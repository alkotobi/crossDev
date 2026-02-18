#ifndef EXCEL_PATHS_H
#define EXCEL_PATHS_H

#include <string>

namespace excel {

/**
 * Resolve template path. Tries common locations relative to CWD.
 * Returns empty string if not found.
 */
std::string resolveTemplatePath(const std::string& filename);

} // namespace excel

#endif // EXCEL_PATHS_H
