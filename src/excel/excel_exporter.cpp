#include "excel/excel_exporter.h"

namespace excel {

ExcelExporter::ExcelExporter(const std::string& templatePath) {
    doc_.open(templatePath);
}

ExcelExporter::~ExcelExporter() {
    doc_.close();
}

OpenXLSX::XLWorksheet ExcelExporter::worksheet(uint32_t index) {
    return doc_.workbook().worksheet(index);
}

void ExcelExporter::setHeaderRow(uint32_t row, const std::vector<std::string>& values) {
    auto wks = worksheet(1);
    wks.row(row).values() = values;
}

void ExcelExporter::fillDetailBlock(const ExcelDetailBlock& layout,
                                    const std::vector<std::vector<std::string>>& detailRows) {
    auto wks = worksheet(1);
    excel::fillDetailBlock(wks, layout, detailRows);
}

void ExcelExporter::setCell(uint32_t row, uint16_t col, const std::string& value) {
    auto wks = worksheet(1);
    wks.cell(row, col).value() = value;
}

bool ExcelExporter::saveAs(const std::string& path) {
    try {
        doc_.saveAs(path, OpenXLSX::XLForceOverwrite);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

void ExcelExporter::close() {
    doc_.close();
}

} // namespace excel
