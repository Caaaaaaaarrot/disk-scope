#include "utils.h"
#include <sstream>
#include <iomanip>

SizeParts format_size(uintmax_t bytes, int precision) {
    const double KB = 1024.0;
    const double MB = KB * 1024;
    const double GB = MB * 1024;

    SizeParts out;

    if (bytes < KB) {
        out.value = std::to_string(bytes);
        out.unit = "B";
    } else if (bytes < MB) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << bytes / KB;
        out.value = oss.str();
        out.unit = "KB";
    } else if (bytes < GB) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << bytes / MB;
        out.value = oss.str();
        out.unit = "MB";
    } else {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(precision) << bytes / GB;
        out.value = oss.str();
        out.unit = "GB";
    }

    return out;
}