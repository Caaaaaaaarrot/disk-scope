#include "analyzer.h"
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

std::shared_ptr<FileNode> Analyzer::analyze(const fs::path& path) {
    return build_tree(path);
}

std::shared_ptr<FileNode> Analyzer::build_tree(const fs::path& path) {
    auto node = std::make_shared<FileNode>(
        path.filename().string().empty() ? path.string() : path.filename().string(),
        fs::is_directory(path)
    );

    try {
        if (node->is_directory) {
            for (const auto& entry : fs::directory_iterator(path)) {
                auto child = build_tree(entry.path());
                node->size += child->size;
                node->children.push_back(child);
            }
        } else {
            node->size = fs::file_size(path);
        }
    } catch (const std::exception& e) {
        // 权限问题 / 无法访问
        std::cerr << "Warning: " << e.what() << "\n";
    }

    return node;
}