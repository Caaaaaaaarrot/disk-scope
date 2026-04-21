#include "analyzer.h"
#include <iostream>
#include <iomanip>

void print_tree(const std::shared_ptr<FileNode>& node, int depth = 0) {
    for (int i = 0; i < depth; ++i) std::cout << "  ";

    std::cout << (node->is_directory ? "[D] " : "[F] ")
              << node->name
              << " (" << std::fixed << std::setprecision(2)
              << node->size / 1024.0 / 1024.0 << " MB)\n";

    for (const auto& child : node->children) {
        print_tree(child, depth + 1);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./disk_scope <path>\n";
        return 1;
    }

    Analyzer analyzer;
    auto root = analyzer.analyze(argv[1]);

    std::cout << "\n===== Disk Usage Analysis =====\n";
    print_tree(root);

    std::cout << "\nTotal Size: "
              << root->size / 1024.0 / 1024.0
              << " MB\n";

    return 0;
}