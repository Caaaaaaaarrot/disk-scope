#pragma once
#include "file_node.h"
#include <filesystem>
#include <memory>
#include <vector>
#include <unordered_map>

class Analyzer {
public:
    enum class SortKey {
        SIZE,
        NAME,
        PATH
    };

    void set_precision(int precision);
    void set_sort(SortKey key, bool reverse);

    std::shared_ptr<FileNode> analyze(const std::filesystem::path& path);

    // 功能
    void list_all_files(const std::shared_ptr<FileNode>& root);
    void get_top_k_files(const std::shared_ptr<FileNode>& root, int k);
    void get_file_type_stats(const std::shared_ptr<FileNode>& root);

    void print_tree(const std::shared_ptr<FileNode>& root);
    void export_json(const std::shared_ptr<FileNode>& root);
    void interactive_view(const std::shared_ptr<FileNode>& root);

private:
    int precision_ = 2;
    SortKey sort_key_ = SortKey::SIZE;
    bool reverse_ = false;

    std::shared_ptr<FileNode> build(const std::filesystem::path& path);
    void scan(const std::shared_ptr<FileNode>& node,
              const std::filesystem::path& path);

    void build_sizes(const std::shared_ptr<FileNode>& node);
    void collect_files(const std::shared_ptr<FileNode>& node,
                       std::vector<std::shared_ptr<FileNode>>& files);

    void collect_types(const std::shared_ptr<FileNode>& node,
                       std::unordered_map<std::string, uintmax_t>& stats);
};