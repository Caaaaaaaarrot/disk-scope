#include "utils.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstdint>
#ifdef _WIN32
#include <windows.h>
#endif

SizeParts format_size(uintmax_t bytes, int precision) {
    const double KB = 1024.0;
    const double MB = KB * 1024;
    const double GB = MB * 1024;

    SizeParts out;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision);

    if (bytes < KB) {
        out.value = std::to_string(bytes);
        out.unit = "B";
    } else if (bytes < MB) {
        oss << bytes / KB;
        out.value = oss.str();
        out.unit = "KB";
    } else if (bytes < GB) {
        oss << bytes / MB;
        out.value = oss.str();
        out.unit = "MB";
    } else {
        oss << bytes / GB;
        out.value = oss.str();
        out.unit = "GB";
    }

    return out;
}

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void set_color(int color) {
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
#else
    std::cout << "\033[" << color << "m";
#endif
}

void reset_color() {
#ifdef _WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
#else
    std::cout << "\033[0m";
#endif
}