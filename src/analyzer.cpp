#include "analyzer.h"
#include <filesystem>
#include <iostream>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// ✅ 单位自适应
std::string format_size(uintmax_t bytes) {
    const double KB = 1024.0;
    const double MB = KB * 1024;
    const double GB = MB * 1024;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    if (bytes < KB) {
        oss << bytes << " B";
    } else if (bytes < MB) {
        oss << bytes / KB << " KB";
    } else if (bytes < GB) {
        oss << bytes / MB << " MB";
    } else {
        oss << bytes / GB << " GB";
    }

    return oss.str();
}

std::shared_ptr<FileNode> Analyzer::analyze(const fs::path& path) {
    return build_tree(path);
}

std::shared_ptr<FileNode> Analyzer::build_tree(const fs::path& path) {
    auto node = std::make_shared<FileNode>(
        path.filename().string().empty() ? path.string() : path.filename().string(),
        fs::is_directory(path)
    );

    try {
        if (node->is_directory) {
            for (const auto& entry : fs::directory_iterator(path)) {
                auto child = build_tree(entry.path());
                node->size += child->size;
                node->children.push_back(child);
            }
        } else {
            node->size = fs::file_size(path);
        }
    } catch (...) {}

    return node;
}

void Analyzer::collect_files(const std::shared_ptr<FileNode>& node,
                             std::vector<std::shared_ptr<FileNode>>& files) {
    if (!node->is_directory) {
        files.push_back(node);
    }
    for (const auto& child : node->children) {
        collect_files(child, files);
    }
}

// ✅ Top-K（小根堆）
void Analyzer::get_top_k_files(const std::shared_ptr<FileNode>& root, int k) {
    std::vector<std::shared_ptr<FileNode>> files;
    collect_files(root, files);

    auto cmp = [](const std::shared_ptr<FileNode>& a,
                  const std::shared_ptr<FileNode>& b) {
        return a->size > b->size;
    };

    std::priority_queue<
        std::shared_ptr<FileNode>,
        std::vector<std::shared_ptr<FileNode>>,
        decltype(cmp)
    > min_heap(cmp);

    for (auto& file : files) {
        if (min_heap.size() < k) {
            min_heap.push(file);
        } else if (file->size > min_heap.top()->size) {
            min_heap.pop();
            min_heap.push(file);
        }
    }

    std::vector<std::shared_ptr<FileNode>> result;
    while (!min_heap.empty()) {
        result.push_back(min_heap.top());
        min_heap.pop();
    }

    std::reverse(result.begin(), result.end());

    std::cout << "\nTop " << k << " largest files:\n\n";

    for (int i = 0; i < result.size(); ++i) {
        std::string name = result[i]->name;
        if (name.length() > 28) {
            name = name.substr(0, 25) + "...";
        }

        std::cout << std::setw(3) << (i + 1) << ". "
                  << std::setw(30) << std::left << name
                  << std::setw(12) << std::right << format_size(result[i]->size)
                  << "\n";
    }
}

// 🔹 类型统计
void Analyzer::collect_types(const std::shared_ptr<FileNode>& node,
                             std::unordered_map<std::string, uintmax_t>& stats) {
    if (!node->is_directory) {
        auto pos = node->name.find_last_of('.');
        std::string ext = (pos == std::string::npos) ? "unknown" : node->name.substr(pos);

        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        stats[ext] += node->size;
    }

    for (const auto& child : node->children) {
        collect_types(child, stats);
    }
}

void Analyzer::get_file_type_stats(const std::shared_ptr<FileNode>& root) {
    std::unordered_map<std::string, uintmax_t> stats;
    collect_types(root, stats);

    std::vector<std::pair<std::string, uintmax_t>> vec(stats.begin(), stats.end());

    std::sort(vec.begin(), vec.end(),
              [](auto& a, auto& b) {
                  return a.second > b.second;
              });

    std::cout << "\nFile type statistics:\n\n";

    int count = 0;
    for (auto& [type, size] : vec) {
        if (count++ >= 10) break;

        std::cout << std::setw(10) << std::left << type
                  << std::setw(12) << std::right << format_size(size)
                  << "\n";
    }
}