## 关于项目 | About
C++ 练手项目，写了个磁盘分析工具，尝试了一下。

This is a hands-on C++ practice project built by a sophomore university student. After learning Operating Systems, Concurrent Programming and Data Structures, I built this tool to turn theoretical knowledge into real-world code, and sharpen my C++ development skills.
The code is far from perfect. Any suggestions on code specification, performance optimization, or feature design are highly welcome. Thanks a lot in advance!

---

## 功能

- **多线程扫描** — 使用线程池并行遍历目录，大幅加快扫描速度
- **多种输出模式** — 扁平文件列表、树形视图、JSON 导出、交互式浏览器
- **排序与筛选** — 支持按名称、大小、路径排序，可反转顺序
- **Top-K** — 快速查看最大的 N 个文件
- **文件类型统计** — 按扩展名聚合磁盘占用
- **跨平台** — Windows 原生支持 · Linux / macOS 需要 pthread

## 编译

环境要求：CMake 3.10 以上，C++17 编译器。

```bash
mkdir build && cd build
cmake ..
cmake --build .
```


## 用法

```
disk_scope <路径> [选项]
```

### 选项

| 选项 | 说明 |
|------|------|
| `--all` | 列出所有文件，按当前排序键排序 |
| `--top K` | 显示前 K 个最大的文件 |
| `--tree` | 以树形结构打印目录及大小 |
| `--json` | 将目录树导出为 JSON（可用于火焰图等） |
| `--sort name\|size\|path` | 设置排序键（默认按 size） |
| `--reverse` | 反转排序顺序 |
| `--interactive` | 启动交互式 TUI 浏览器 |
| `--help` | 打印帮助信息 |

不指定任何输出选项时，默认显示最大的前 10 个文件。

### 示例

```bash
# 查看当前目录下最大的 5 个文件
disk_scope . --top 5

# 树形视图
disk_scope . --tree

# 交互式浏览
disk_scope C:\ --interactive

# 按名称正序排列所有文件
disk_scope . --all --sort name --reverse

# JSON 导出到文件
disk_scope . --json > tree.json
```

## 交互模式

方向键上下选择，Enter 进入目录，`b` 返回上级，`q` 退出。目录始终按大小降序排列——占用最多的项会浮到最上方，方便快速定位磁盘空间杀手。
