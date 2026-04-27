#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mutex>

struct FileNode {
    std::string name;
    uintmax_t size = 0;
    bool is_directory;

    std::weak_ptr<FileNode> parent;
    std::vector<std::shared_ptr<FileNode>> children;

    std::mutex mutex;

    FileNode(const std::string& n, bool is_dir)
        : name(n), is_directory(is_dir) {}
};