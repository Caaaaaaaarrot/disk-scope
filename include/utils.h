#pragma once
#include <string>
#include <cstdint>
#include <cstddef>

struct SizeParts {
    std::string value;
    std::string unit;
};

SizeParts format_size(uintmax_t bytes, int precision);

// 终端控制
void clear_screen();
void set_color(int color);
void reset_color();