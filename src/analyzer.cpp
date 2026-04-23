#include "analyzer.h"
#include "thread_pool.h"
#include "utils.h"
#include <filesystem>
#include <iostream>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <atomic>

namespace fs = std::filesystem;

static ThreadPool pool(std::thread::hardware_concurrency());
static std::atomic<int> active_tasks{0};

void Analyzer::set_precision(int precision) {
    if (precision >= 0) precision_ = precision;
}

std::shared_ptr<FileNode> Analyzer::analyze(const fs::path& path) {
    return build_tree_parallel(path);
}

std::shared_ptr<FileNode> Analyzer::build_tree_parallel(const fs::path& path) {
    auto root = std::make_shared<FileNode>(path.string(), fs::is_directory(path));

    active_tasks = 1;

    pool.enqueue([this, root, path]() {
        scan_node(root, path);
        active_tasks--;
    });

    while (active_tasks > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return root;
}

void Analyzer::scan_node(const std::shared_ptr<FileNode>& node,
                         const fs::path& path) {
    if (node->is_directory) {
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                bool is_dir = fs::is_directory(entry.path());

                auto child = std::make_shared<FileNode>(
                    entry.path().filename().string(), is_dir);

                child->parent = node;

                {
                    std::lock_guard<std::mutex> lock(node->mutex);
                    node->children.push_back(child);
                    node->pending++;
                }

                active_tasks++;

                pool.enqueue([this, child, entry]() {
                    scan_node(child, entry.path());
                    active_tasks--;
                });
            }
        } catch (...) {}
    } else {
        try {
            node->size = fs::file_size(path);
        } catch (...) {}
    }

    finish_node(node);
}

void Analyzer::finish_node(const std::shared_ptr<FileNode>& node) {
    std::shared_ptr<FileNode> parent;

    {
        std::lock_guard<std::mutex> lock(node->mutex);
        node->pending--;
        if (node->pending != 0) return;

        parent = node->parent.lock();
    }

    if (parent) {
        {
            std::lock_guard<std::mutex> lock(parent->mutex);
            parent->size += node->size;
        }
        finish_node(parent);
    }
}

void Analyzer::collect_files(const std::shared_ptr<FileNode>& node,
                             std::vector<std::shared_ptr<FileNode>>& files) {
    if (!node->is_directory) files.push_back(node);
    for (auto& c : node->children) collect_files(c, files);
}

void Analyzer::get_top_k_files(const std::shared_ptr<FileNode>& root, int k) {
    std::vector<std::shared_ptr<FileNode>> files;
    collect_files(root, files);

    auto cmp = [](auto& a, auto& b) { return a->size > b->size; };

    std::priority_queue<
        std::shared_ptr<FileNode>,
        std::vector<std::shared_ptr<FileNode>>,
        decltype(cmp)
    > heap(cmp);

    for (auto& f : files) {
        if (heap.size() < k) heap.push(f);
        else if (f->size > heap.top()->size) {
            heap.pop();
            heap.push(f);
        }
    }

    std::vector<std::shared_ptr<FileNode>> res;
    while (!heap.empty()) {
        res.push_back(heap.top());
        heap.pop();
    }
    std::reverse(res.begin(), res.end());

    std::cout << "\nTop " << k << " largest files:\n\n";

    std::size_t index_width = std::to_string(res.size()).length();
    std::size_t name_width = 0;
    std::size_t value_width = 0;
    std::size_t unit_width = 0;
    std::vector<SizeParts> sizes;
    sizes.reserve(res.size());

    for (const auto& file : res) {
        name_width = std::max(name_width, file->name.length());
        auto parts = format_size(file->size, precision_);
        value_width = std::max(value_width, parts.value.length());
        unit_width = std::max(unit_width, parts.unit.length());
        sizes.push_back(parts);
    }

    name_width = std::min<std::size_t>(40, std::max<std::size_t>(name_width, 16));

    for (int i = 0; i < res.size(); ++i) {
        std::string name = res[i]->name;
        if (name.length() > name_width) {
            name = name.substr(0, name_width - 3) + "...";
        }

        std::cout << std::right << std::setw(static_cast<int>(index_width)) << i + 1 << ". "
                  << std::left << std::setw(static_cast<int>(name_width)) << name
                  << "  "
                  << std::right << std::setw(static_cast<int>(value_width)) << sizes[i].value
                  << " "
                  << std::left << std::setw(static_cast<int>(unit_width)) << sizes[i].unit
                  << "\n";
    }
}

void Analyzer::collect_types(const std::shared_ptr<FileNode>& node,
                             std::unordered_map<std::string, uintmax_t>& stats) {
    if (!node->is_directory) {
        auto pos = node->name.find_last_of('.');
        std::string ext = (pos == std::string::npos) ? "unknown" : node->name.substr(pos);
        stats[ext] += node->size;
    }
    for (auto& c : node->children) collect_types(c, stats);
}

void Analyzer::get_file_type_stats(const std::shared_ptr<FileNode>& root) {
    std::unordered_map<std::string, uintmax_t> stats;
    collect_types(root, stats);

    std::vector<std::pair<std::string, uintmax_t>> vec(stats.begin(), stats.end());
    std::sort(vec.begin(), vec.end(), [](auto& a, auto& b) {
        return a.second > b.second;
    });

    std::cout << "\nFile type statistics:\n\n";

    int limit = std::min(10, (int)vec.size());
    std::size_t type_width = 0;
    std::size_t value_width = 0;
    std::size_t unit_width = 0;
    std::vector<SizeParts> sizes;
    sizes.reserve(limit);

    for (int i = 0; i < limit; ++i) {
        type_width = std::max(type_width, vec[i].first.length());
        auto parts = format_size(vec[i].second, precision_);
        value_width = std::max(value_width, parts.value.length());
        unit_width = std::max(unit_width, parts.unit.length());
        sizes.push_back(parts);
    }

    type_width = std::max<std::size_t>(10, type_width);

    for (int i = 0; i < limit; ++i) {
        std::cout << std::left << std::setw(static_cast<int>(type_width)) << vec[i].first
                  << "  "
                  << std::right << std::setw(static_cast<int>(value_width)) << sizes[i].value
                  << " "
                  << std::left << std::setw(static_cast<int>(unit_width)) << sizes[i].unit
                  << "\n";
    }
}

void Analyzer::list_all_files(const std::shared_ptr<FileNode>& root) {
    std::vector<std::shared_ptr<FileNode>> files;
    collect_files(root, files);

    std::cout << "\nAll files:\n\n";

    std::size_t index_width = std::to_string(files.size()).length();
    std::size_t name_width = 0;
    std::size_t value_width = 0;
    std::size_t unit_width = 0;
    std::vector<SizeParts> sizes;
    sizes.reserve(files.size());

    for (const auto& file : files) {
        name_width = std::max(name_width, file->name.length());
        auto parts = format_size(file->size, precision_);
        value_width = std::max(value_width, parts.value.length());
        unit_width = std::max(unit_width, parts.unit.length());
        sizes.push_back(parts);
    }

    name_width = std::min<std::size_t>(48, std::max<std::size_t>(name_width, 16));

    for (int i = 0; i < files.size(); ++i) {
        std::string name = files[i]->name;
        if (name.length() > name_width) {
            name = name.substr(0, name_width - 3) + "...";
        }

        std::cout << std::right << std::setw(static_cast<int>(index_width)) << i + 1 << ". "
                  << std::left << std::setw(static_cast<int>(name_width)) << name
                  << "  "
                  << std::right << std::setw(static_cast<int>(value_width)) << sizes[i].value
                  << " "
                  << std::left << std::setw(static_cast<int>(unit_width)) << sizes[i].unit
                  << "\n";
    }
}
