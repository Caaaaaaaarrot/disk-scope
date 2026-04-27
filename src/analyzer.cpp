#include "analyzer.h"
#include "thread_pool.h"
#include "utils.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <atomic>
#include <thread>

namespace fs = std::filesystem;

static ThreadPool pool(std::thread::hardware_concurrency());
static std::atomic<int> active_tasks{0};
static std::atomic<int> scanned{0};

static bool cmp(const std::shared_ptr<FileNode>& a,
                const std::shared_ptr<FileNode>& b,
                Analyzer::SortKey key) {
    if (key == Analyzer::SortKey::SIZE) return a->size < b->size;
    if (key == Analyzer::SortKey::NAME) return a->name < b->name;
    return a->name < b->name;
}

void Analyzer::set_sort(SortKey key, bool reverse) {
    sort_key_ = key;
    reverse_ = reverse;
}

void Analyzer::set_precision(int p) {
    if (p >= 0) precision_ = p;
}

std::shared_ptr<FileNode> Analyzer::analyze(const fs::path& path) {
    return build_tree_parallel(path);
}

std::shared_ptr<FileNode> Analyzer::build_tree_parallel(const fs::path& path) {
    auto root = std::make_shared<FileNode>(path.string(), fs::is_directory(path));

    active_tasks = 1;
    scanned = 0;

    std::thread progress([&]() {
        while (active_tasks > 0) {
            std::cout << "\rScanning... " << scanned << " files";
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "\rScanning... Done!        \n";
    });

    pool.enqueue([this, root, path]() {
        scan_node(root, path);
        active_tasks--;
    });

    while (active_tasks > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    progress.join();
    return root;
}

void Analyzer::scan_node(const std::shared_ptr<FileNode>& node,
                         const fs::path& path) {
    if (node->is_directory) {
        try {
            for (auto& e : fs::directory_iterator(path)) {
                auto child = std::make_shared<FileNode>(
                    e.path().filename().string(),
                    fs::is_directory(e));

                child->parent = node;

                {
                    std::lock_guard<std::mutex> lock(node->mutex);
                    node->children.push_back(child);
                    node->pending++;
                }

                active_tasks++;
                pool.enqueue([this, child, e]() {
                    scan_node(child, e.path());
                    active_tasks--;
                });
            }
        } catch (...) {}
    } else {
        try {
            node->size = fs::file_size(path);
            scanned++;
        } catch (...) {}
    }

    finish_node(node);
}

void Analyzer::finish_node(const std::shared_ptr<FileNode>& node) {
    auto parent = node->parent.lock();

    {
        std::lock_guard<std::mutex> lock(node->mutex);
        node->pending--;
        if (node->pending != 0) return;
    }

    if (parent) {
        {
            std::lock_guard<std::mutex> lock(parent->mutex);
            parent->size += node->size;
        }
        finish_node(parent);
    }
}

// ================= tree（重点🔥）=================

void print_tree_impl(const std::shared_ptr<FileNode>& node,
                     const std::string& prefix,
                     bool is_last,
                     int precision) {

    auto size = format_size(node->size, precision);

    std::cout << prefix
              << (is_last ? "└── " : "├── ")
              << node->name << " (" << size.value << " " << size.unit << ")\n";

    std::string new_prefix = prefix + (is_last ? "    " : "│   ");

    for (size_t i = 0; i < node->children.size(); ++i) {
        print_tree_impl(node->children[i],
                        new_prefix,
                        i == node->children.size() - 1,
                        precision);
    }
}

void Analyzer::print_tree(const std::shared_ptr<FileNode>& root) {
    print_tree_impl(root, "", true, precision_);
}

// ================= list =================

void Analyzer::collect_files(const std::shared_ptr<FileNode>& node,
                             std::vector<std::shared_ptr<FileNode>>& files) {
    if (!node->is_directory) files.push_back(node);
    for (auto& c : node->children) collect_files(c, files);
}

void Analyzer::list_all_files(const std::shared_ptr<FileNode>& root) {
    std::vector<std::shared_ptr<FileNode>> files;
    collect_files(root, files);

    std::sort(files.begin(), files.end(),
        [this](auto& a, auto& b) {
            bool r = cmp(a, b, sort_key_);
            return reverse_ ? !r : r;
        });

    for (auto& f : files) {
        auto p = format_size(f->size, precision_);
        std::cout << f->name << "  " << p.value << " " << p.unit << "\n";
    }
}

// ================= JSON =================

void Analyzer::export_json(const std::shared_ptr<FileNode>& node) {
    std::cout << "{ \"name\":\"" << node->name << "\", \"size\":" << node->size;

    if (!node->children.empty()) {
        std::cout << ",\"children\":[";
        for (size_t i = 0; i < node->children.size(); ++i) {
            export_json(node->children[i]);
            if (i != node->children.size() - 1) std::cout << ",";
        }
        std::cout << "]";
    }

    std::cout << "}";
}