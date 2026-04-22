#include "analyzer.h"
#include <iostream>
#include <cstring>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./disk_scope <path> [--top K] [--type]\n";
        return 1;
    }

    std::string path = argv[1];

    Analyzer analyzer;
    auto root = analyzer.analyze(path);

    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--top") == 0 && i + 1 < argc) {
            int k = std::stoi(argv[++i]);
            analyzer.get_top_k_files(root, k);
        } else if (std::strcmp(argv[i], "--type") == 0) {
            analyzer.get_file_type_stats(root);
        }
    }

    return 0;
}