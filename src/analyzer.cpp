#include "analyzer.h"
#include "thread_pool.h"
#include "utils.h"

#include <filesystem>
#include <iostream>
#include <algorithm>
#include <atomic>
#include <thread>

#ifdef _WIN32
#include <conio.h>
#endif

namespace fs = std::filesystem;

static ThreadPool pool(std::thread::hardware_concurrency());
static std::atomic<int> tasks{0};

void Analyzer::set_precision(int precision) {
    if (precision >= 0) precision_ = precision;
}

void Analyzer::set_sort(SortKey key, bool reverse) {
    sort_key_ = key;
    reverse_ = reverse;
}

std::shared_ptr<FileNode> Analyzer::analyze(const fs::path& path) {
    auto root = std::make_shared<FileNode>(path.string(), fs::is_directory(path));

    tasks = 1;

    pool.enqueue([this, root, path]() {
        scan(root, path);
        tasks--;
    });

    while (tasks > 0) {
        std::cout << "\rScanning...";
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\rScanning done.       \n";

    build_sizes(root);

    return root;
}

void Analyzer::scan(const std::shared_ptr<FileNode>& node,
                    const fs::path& path) {

    if (node->is_directory) {
        try {
            for (auto& entry : fs::directory_iterator(path)) {
                bool is_dir = fs::is_directory(entry);

                auto child = std::make_shared<FileNode>(
                    entry.path().filename().string(), is_dir);

                child->parent = node;

                {
                    std::lock_guard<std::mutex> lock(node->mutex);
                    node->children.push_back(child);
                }

                tasks++;

                pool.enqueue([this, child, entry]() {
                    scan(child, entry.path());
                    tasks--;
                });
            }
        } catch (...) {}
    } else {
        try {
            node->size = fs::file_size(path);
        } catch (...) {}
    }
}

void Analyzer::build_sizes(const std::shared_ptr<FileNode>& node) {
    for (auto& c : node->children) {
        build_sizes(c);
        node->size += c->size;
    }
}

void Analyzer::collect_files(const std::shared_ptr<FileNode>& node,
                             std::vector<std::shared_ptr<FileNode>>& files) {
    if (!node->is_directory)
        files.push_back(node);

    for (auto& c : node->children)
        collect_files(c, files);
}

// ======================= 功能 =======================

void Analyzer::list_all_files(const std::shared_ptr<FileNode>& root) {
    std::vector<std::shared_ptr<FileNode>> files;
    collect_files(root, files);

    std::sort(files.begin(), files.end(), [this](auto& a, auto& b) {
        if (sort_key_ == SortKey::SIZE)
            return reverse_ ? a->size < b->size : a->size > b->size;
        return reverse_ ? a->name > b->name : a->name < b->name;
    });

    for (auto& f : files) {
        auto p = format_size(f->size, precision_);
        std::cout << f->name << "  " << p.value << " " << p.unit << "\n";
    }
}

void Analyzer::get_top_k_files(const std::shared_ptr<FileNode>& root, int k) {
    std::vector<std::shared_ptr<FileNode>> files;
    collect_files(root, files);

    std::sort(files.begin(), files.end(),
              [](auto& a, auto& b) { return a->size > b->size; });

    if (k < files.size()) files.resize(k);

    for (auto& f : files) {
        auto p = format_size(f->size, precision_);
        std::cout << f->name << "  " << p.value << " " << p.unit << "\n";
    }
}

void Analyzer::collect_types(const std::shared_ptr<FileNode>& node,
                             std::unordered_map<std::string, uintmax_t>& stats) {
    if (!node->is_directory) {
        auto pos = node->name.find_last_of('.');
        std::string ext = (pos == std::string::npos) ? "unknown" : node->name.substr(pos);
        stats[ext] += node->size;
    }

    for (auto& c : node->children)
        collect_types(c, stats);
}

void Analyzer::get_file_type_stats(const std::shared_ptr<FileNode>& root) {
    std::unordered_map<std::string, uintmax_t> stats;
    collect_types(root, stats);

    for (auto& [k, v] : stats) {
        auto p = format_size(v, precision_);
        std::cout << k << "  " << p.value << " " << p.unit << "\n";
    }
}

// ======================= TREE =======================

void print_tree_impl(const std::shared_ptr<FileNode>& node, int depth) {
    for (int i = 0; i < depth; ++i) std::cout << "  ";

    auto p = format_size(node->size, 2);
    std::cout << node->name << " (" << p.value << " " << p.unit << ")\n";

    for (auto& c : node->children)
        print_tree_impl(c, depth + 1);
}

void Analyzer::print_tree(const std::shared_ptr<FileNode>& root) {
    print_tree_impl(root, 0);
}

// ======================= JSON =======================

void Analyzer::export_json(const std::shared_ptr<FileNode>& node) {
    std::cout << "{ \"name\": \"" << node->name << "\", \"size\": " << node->size;

    if (!node->children.empty()) {
        std::cout << ", \"children\": [";
        for (size_t i = 0; i < node->children.size(); ++i) {
            export_json(node->children[i]);
            if (i + 1 != node->children.size()) std::cout << ",";
        }
        std::cout << "]";
    }

    std::cout << "}";
}

// ======================= 交互 =======================

void Analyzer::interactive_view(const std::shared_ptr<FileNode>& root) {
    auto cur = root;
    int selected = 0;

    while (true) {
        clear_screen();

        std::cout << "Dir: " << cur->name << "\n\n";

        auto& children = cur->children;

        std::sort(children.begin(), children.end(),
                  [](auto& a, auto& b) {
                      return a->size > b->size;
                  });

        for (int i = 0; i < children.size(); ++i) {
            auto& c = children[i];
            auto p = format_size(c->size, 2);

            if (i == selected) {
                set_color(10);
                std::cout << "> ";
            } else {
                std::cout << "  ";
            }

            std::cout << c->name << " (" << p.value << " " << p.unit << ")\n";
            reset_color();
        }

        std::cout << "\n[q] quit  [Enter] open  [b] back\n";

        int ch = _getch();

        if (ch == 'q') break;
        else if (ch == 'b') {
            if (auto p = cur->parent.lock()) cur = p;
            selected = 0;
        }
        else if (ch == 13) {
            if (selected < children.size() && children[selected]->is_directory) {
                cur = children[selected];
                selected = 0;
            }
        }
        else if (ch == 224) {
            ch = _getch();
            if (ch == 72 && selected > 0) selected--;
            else if (ch == 80 && selected < children.size()-1) selected++;
        }
    }
}