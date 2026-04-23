#pragma once
#include <string>
#include <vector>
#include <memory>
#include <mutex>

struct FileNode {
    std::string name;
    uintmax_t size;
    bool is_directory;

    std::weak_ptr<FileNode> parent;
    std::vector<std::shared_ptr<FileNode>> children;

    std::size_t pending;
    mutable std::mutex mutex;

    FileNode(const std::string& n, bool is_dir)
        : name(n), size(0), is_directory(is_dir), pending(1) {}
};