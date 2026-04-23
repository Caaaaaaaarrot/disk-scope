#include "analyzer.h"
#include <iostream>
#include <cstring>

struct Options {
    std::string path;

    int top_k = -1;
    bool show_type = false;
    bool show_all = false;
    int precision = 2;
    bool show_help = false;
};

// 打印帮助
void print_help() {
    std::cout <<
    R"(DiskScope - Disk Usage Analyzer

    Usage:
    disk_scope <path> [options]

    Options:
    --top K           Show top K largest files
    --type            Show file type statistics
    --all             List all files
    --precision N     Set decimal precision (default: 2)
    --help            Show this help message

    Examples:
    disk_scope . --top 10
    disk_scope . --type
    disk_scope . --all --precision 4
    )";
}

// 参数解析
bool parse_args(int argc, char* argv[], Options& opt) {
    if (argc < 2) {
        return false;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            opt.show_help = true;
        } else if (strcmp(argv[i], "--top") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --top\n";
                return false;
            }
            opt.top_k = std::stoi(argv[++i]);
        } else if (strcmp(argv[i], "--type") == 0) {
            opt.show_type = true;
        } else if (strcmp(argv[i], "--all") == 0) {
            opt.show_all = true;
        } else if (strcmp(argv[i], "--precision") == 0) {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for --precision\n";
                return false;
            }
            opt.precision = std::stoi(argv[++i]);
        } else if (strncmp(argv[i], "--", 2) == 0) {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            return false;
        } else if (opt.path.empty()) {
            opt.path = argv[i];
        } else {
            std::cerr << "Unexpected argument: " << argv[i] << "\n";
            return false;
        }
    }

    if (opt.show_help) {
        return true;
    }

    if (opt.path.empty()) {
        std::cerr << "Missing path argument.\n";
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    Options opt;

    if (!parse_args(argc, argv, opt)) {
        print_help();
        return 1;
    }

    if (opt.show_help) {
        print_help();
        return 0;
    }

    Analyzer analyzer;
    analyzer.set_precision(opt.precision);

    auto root = analyzer.analyze(opt.path);

    bool executed = false;

    if (opt.top_k > 0) {
        analyzer.get_top_k_files(root, opt.top_k);
        executed = true;
    }

    if (opt.show_type) {
        analyzer.get_file_type_stats(root);
        executed = true;
    }

    if (opt.show_all) {
        analyzer.list_all_files(root);
        executed = true;
    }

    // 默认行为（非常重要）
    if (!executed) {
        analyzer.get_top_k_files(root, 10);
    }

    return 0;
}
