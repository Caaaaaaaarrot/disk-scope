#include "analyzer.h"
#include <filesystem>
#include <iostream>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

struct SizeParts {
    std::string value;
    std::string unit;
};

// 单位自适应
SizeParts format_size(uintmax_t bytes, int precision) {
    const double KB = 1024.0;
    const double MB = KB * 1024;
    const double GB = MB * 1024;

    SizeParts out;

    if (bytes < KB) {
        out.value = std::to_string(bytes);
        out.unit = "B";
    } else if (bytes < MB) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << bytes / KB;
        out.value = oss.str();
        out.unit = "KB";
    } else if (bytes < GB) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << bytes / MB;
        out.value = oss.str();
        out.unit = "MB";
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << bytes / GB;
        out.value = oss.str();
        out.unit = "GB";
    }

    return out;
}

void Analyzer::set_precision(int precision) {
    if (precision >= 0) {
        precision_ = precision;
    }
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

    std::vector<std::string> names;
    std::vector<SizeParts> sizes;
    names.reserve(result.size());
    sizes.reserve(result.size());

    std::size_t value_width = 0;
    std::size_t unit_width = 0;
    for (const auto& item : result) {
        std::string name = item->name;
        if (name.length() > 28) {
            name = name.substr(0, 25) + "...";
        }
        names.push_back(name);

        SizeParts parts = format_size(item->size, precision_);
        value_width = std::max(value_width, parts.value.length());
        unit_width = std::max(unit_width, parts.unit.length());
        sizes.push_back(parts);
    }

    for (int i = 0; i < result.size(); ++i) {
        std::cout << std::right << std::setw(3) << (i + 1) << ". "
                  << std::setw(30) << std::left << names[i]
                  << std::setw(static_cast<int>(value_width) + 1) << std::right << sizes[i].value
                  << " "
                  << std::setw(static_cast<int>(unit_width)) << std::left << sizes[i].unit
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

    int limit = std::min<int>(10, vec.size());
    std::vector<SizeParts> sizes;
    sizes.reserve(limit);
    std::size_t value_width = 0;
    std::size_t unit_width = 0;
    for (int i = 0; i < limit; ++i) {
        SizeParts parts = format_size(vec[i].second, precision_);
        value_width = std::max(value_width, parts.value.length());
        unit_width = std::max(unit_width, parts.unit.length());
        sizes.push_back(parts);
    }

    std::cout << "\nFile type statistics:\n\n";

    int count = 0;
    for (const auto& entry : vec) {
        if (count++ >= 10) break;

        std::cout << std::setw(10) << std::left << entry.first
                  << std::setw(static_cast<int>(value_width) + 1) << std::right << sizes[count - 1].value
                  << " "
                  << std::setw(static_cast<int>(unit_width)) << std::left << sizes[count - 1].unit
                  << "\n";
    }
}
