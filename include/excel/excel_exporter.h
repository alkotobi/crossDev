#ifndef EXCEL_EXPORTER_H
#define EXCEL_EXPORTER_H

#include "excel_layout.h"
#include "excel_paths.h"
#include "excel_sheet_ops.h"
#include <OpenXLSX.hpp>
#include <string>
#include <vector>

namespace excel {

/**
 * Reusable Excel template exporter.
 * Open template, fill headers/details, save.
 */
class ExcelExporter {
public:
    explicit ExcelExporter(const std::string& templatePath);
    ~ExcelExporter();

    ExcelExporter(const ExcelExporter&) = delete;
    ExcelExporter& operator=(const ExcelExporter&) = delete;

    OpenXLSX::XLWorksheet worksheet(uint32_t index = 1);

    void setHeaderRow(uint32_t row, const std::vector<std::string>& values);

    void fillDetailBlock(const ExcelDetailBlock& layout,
                        const std::vector<std::vector<std::string>>& detailRows);

    void setCell(uint32_t row, uint16_t col, const std::string& value);

    bool saveAs(const std::string& path);
    void close();

private:
    OpenXLSX::XLDocument doc_;
};

} // namespace excel

#endif // EXCEL_EXPORTER_H
