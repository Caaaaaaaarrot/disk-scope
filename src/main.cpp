#include "analyzer.h"
#include <iostream>
#include <cstring>

void help() {
    std::cout << R"(DiskScope v2

Usage:
  disk_scope <path> [options]

Options:
  --all
  --top K
  --tree
  --json
  --sort name|size|path
  --reverse
  --interactive
  --help
)";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        help();
        return 1;
    }

    Analyzer analyzer;

    bool show_all=false, show_tree=false, show_json=false, interactive=false;
    int top=-1;
    bool reverse=false;
    Analyzer::SortKey sort=Analyzer::SortKey::SIZE;

    std::string path = argv[1];

    for (int i=2;i<argc;i++) {
        if (!strcmp(argv[i],"--all")) show_all=true;
        else if (!strcmp(argv[i],"--tree")) show_tree=true;
        else if (!strcmp(argv[i],"--json")) show_json=true;
        else if (!strcmp(argv[i],"--reverse")) reverse=true;
        else if (!strcmp(argv[i],"--interactive")) interactive=true;
        else if (!strcmp(argv[i],"--top") && i+1<argc)
            top = std::stoi(argv[++i]);
        else if (!strcmp(argv[i],"--sort") && i+1<argc) {
            std::string s=argv[++i];
            if (s=="name") sort=Analyzer::SortKey::NAME;
            else if (s=="size") sort=Analyzer::SortKey::SIZE;
            else sort=Analyzer::SortKey::PATH;
        }
        else if (!strcmp(argv[i],"--help")) {
            help(); return 0;
        }
    }

    analyzer.set_sort(sort, reverse);

    auto root = analyzer.analyze(path);

    if (show_tree) analyzer.print_tree(root);
    if (show_json) { analyzer.export_json(root); std::cout<<"\n"; }
    if (show_all) analyzer.list_all_files(root);
    if (top>0) analyzer.get_top_k_files(root, top);

    if (interactive) analyzer.interactive_view(root);
    else if (!show_all && !show_tree && !show_json && top<0)
        analyzer.get_top_k_files(root,10);

    return 0;
}