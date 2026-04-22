#pragma once
#include "file_node.h"
#include <filesystem>
#include <memory>
#include <vector>
#include <unordered_map>

class Analyzer {
public:
    std::shared_ptr<FileNode> analyze(const std::filesystem::path& path);

    void get_top_k_files(const std::shared_ptr<FileNode>& root, int k);
    void get_file_type_stats(const std::shared_ptr<FileNode>& root);

private:
    std::shared_ptr<FileNode> build_tree(const std::filesystem::path& path);

    void collect_files(const std::shared_ptr<FileNode>& node,
                       std::vector<std::shared_ptr<FileNode>>& files);

    void collect_types(const std::shared_ptr<FileNode>& node,
                       std::unordered_map<std::string, uintmax_t>& stats);
};