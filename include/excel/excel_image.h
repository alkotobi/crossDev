#ifndef EXCEL_IMAGE_H
#define EXCEL_IMAGE_H

#include <string>

namespace excel {

/**
 * Inject a logo/image at the top of the first worksheet in an existing .xlsx file.
 * Call this AFTER the Excel file has been saved.
 *
 * @param xlsxPath Path to the .xlsx file (must exist and be a valid xlsx)
 * @param imagePath Path to the PNG image (e.g. logo.png)
 * @param widthInches Display width in inches (default 7.0, ~full letter width)
 * @param heightInches Display height in inches (default 1.5)
 * @param stretchToFill If true, stretch image to fill width x height; if false, preserve aspect ratio
 * @return true on success, false on failure (file not found, invalid xlsx, etc.)
 */
bool injectLetterhead(const std::string& xlsxPath, const std::string& imagePath,
                      double widthInches = 7.0, double heightInches = 1.5,
                      bool stretchToFill = false);

/**
 * Resolve path to letter_head.png (same candidates as template).
 */
std::string resolveLetterheadPath(const std::string& filename = "letter_head.png");

/**
 * Resolve logo path: logo.png in assets/cross_dev/assets.
 * Same path candidates as resolveLetterheadPath.
 */
std::string resolveLogoPath();

/**
 * Read PNG dimensions from IHDR chunk (width, height in pixels).
 * @param path Path to the PNG file
 * @param outWidth Output: image width
 * @param outHeight Output: image height
 * @return true if dimensions were read successfully
 */
bool getPngDimensions(const std::string& path, int& outWidth, int& outHeight);

/**
 * Inject print setup: fit to one page, A4 portrait.
 * Call after the Excel file has been saved.
 * @param xlsxPath Path to the .xlsx file
 * @return true on success
 */
bool injectPrintFitToPage(const std::string& xlsxPath);

} // namespace excel

#endif // EXCEL_IMAGE_H
