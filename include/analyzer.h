#pragma once
#include "file_node.h"
#include <filesystem>
#include <memory>

class Analyzer {
public:
    std::shared_ptr<FileNode> analyze(const std::filesystem::path& path);

private:
    std::shared_ptr<FileNode> build_tree(const std::filesystem::path& path);
};