#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>

struct FileNode {
    std::string name;
    uintmax_t size;
    bool is_directory;
    std::vector<std::shared_ptr<FileNode>> children;

    FileNode(const std::string& n, bool is_dir)
        : name(n), size(0), is_directory(is_dir) {}
};