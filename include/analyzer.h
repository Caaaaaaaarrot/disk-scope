#pragma once
#include "file_node.h"
#include <filesystem>
#include <memory>
#include <vector>
#include <unordered_map>

class Analyzer {
public:
    void set_precision(int precision);

    std::shared_ptr<FileNode> analyze(const std::filesystem::path& path);
    void list_all_files(const std::shared_ptr<FileNode>& root);
    void get_top_k_files(const std::shared_ptr<FileNode>& root, int k);
    void get_file_type_stats(const std::shared_ptr<FileNode>& root);

private:
    int precision_ = 2;

    std::shared_ptr<FileNode> build_tree_parallel(const std::filesystem::path& path);

    void scan_node(const std::shared_ptr<FileNode>& node,
                   const std::filesystem::path& path);

    void finish_node(const std::shared_ptr<FileNode>& node);

    void collect_files(const std::shared_ptr<FileNode>& node,
                       std::vector<std::shared_ptr<FileNode>>& files);

    void collect_types(const std::shared_ptr<FileNode>& node,
                       std::unordered_map<std::string, uintmax_t>& stats);
};