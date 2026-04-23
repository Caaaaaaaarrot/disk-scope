#pragma once
#include <string>
#include <cstdint>

struct SizeParts {
    std::string value;
    std::string unit;
};

SizeParts format_size(uintmax_t bytes, int precision);