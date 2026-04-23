##first commit

disk-scope/
│
├── src/
│   ├── main.cpp
│   └── analyzer.cpp
│
├── include/
│   ├── analyzer.h
│   └── file_node.h
│
├── CMakeLists.txt
└── README.md

实现：
    能扫描一个目录
    不崩溃（权限错误处理）
    能输出树结构


##second commit

在第一版上进行改进

实现：
    文件系统扫描
    树结构建模
    Top-K（排序 / 算法）
    分类统计（哈希表）
    单位自动适配，自动切换 KB/MB/GB
    输出格式对齐，数字列对齐

坑：
    Windows 路径带空格 → 用引号
    扫描太大目录 → 会慢，准备引入多线程


##third commit

改了一下结构
    模块化
    - analyzer：负责核心扫描逻辑
    - thread_pool：独立并发组件
    - utils：负责格式化和输出

实现：
    线程池，多线程加快扫描

disk-scope/
│
├── include/
│   ├── analyzer.h
│   ├── file_node.h
│   ├── thread_pool.h
│   └── utils.h
│
├── src/
│   ├── main.cpp
│   ├── analyzer.cpp
│   └── utils.cpp
│
├── CMakeLists.txt



加入了help

    .\disk_scope.exe --help

    输出：
    DiskScope - Disk Usage Analyzer

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

    感觉有点混乱，明天改一下逻辑