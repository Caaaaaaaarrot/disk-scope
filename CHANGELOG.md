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