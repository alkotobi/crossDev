#include "excel/excel_paths.h"
#include <fstream>

namespace excel {

std::string resolveTemplatePath(const std::string& filename) {
    std::string candidates[] = {
        filename,
        "assets/" + filename,
        "../assets/" + filename,
        "../../assets/" + filename,
        "cross_dev/assets/" + filename,
    };
    for (const auto& p : candidates) {
        std::ifstream f(p);
        if (f.good()) return p;
    }
    return "";
}

} // namespace excel
