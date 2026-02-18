#include "excel/excel_sheet_ops.h"

namespace excel {

void shiftRowsDown(OpenXLSX::XLWorksheet& wks,
                  uint32_t firstRow, uint32_t lastRow,
                  int rowCount, int numCols) {
    if (rowCount <= 0) return;
    for (uint32_t r = lastRow; r >= firstRow; --r) {
        uint32_t destRow = r + static_cast<uint32_t>(rowCount);
        for (int col = 1; col <= numCols; ++col) {
            auto src = wks.findCell(r, col);
            if (!src.empty()) {
                wks.cell(destRow, col) = src;
            }
        }
    }
}

void copyRowFormatToRow(OpenXLSX::XLWorksheet& wks,
                        uint32_t srcRow, uint32_t destRow,
                        int numCols) {
    for (int col = 1; col <= numCols; ++col) {
        wks.cell(destRow, col) = wks.cell(srcRow, col);
    }
}

void fillDetailBlock(OpenXLSX::XLWorksheet& wks,
                     const ExcelDetailBlock& layout,
                     const std::vector<std::vector<std::string>>& detailRows) {
    const int insertCount = static_cast<int>(detailRows.size()) - 1;

    if (insertCount > 0) {
        uint32_t lastRow = wks.rowCount();
        shiftRowsDown(wks, layout.detailStartRow + 1, lastRow,
                     insertCount, static_cast<int>(layout.shiftNumCols));
    }

    const int numCols = static_cast<int>(layout.detailNumCols);
    for (size_t i = 0; i < detailRows.size(); ++i) {
        uint32_t rowNum = layout.detailStartRow + static_cast<uint32_t>(i);
        if (rowNum > layout.detailStartRow) {
            copyRowFormatToRow(wks, layout.detailStartRow, rowNum, numCols);
        }
        const auto& row = detailRows[i];
        for (int col = 0; col < numCols && col < static_cast<int>(row.size()); ++col) {
            wks.cell(rowNum, col + 1).value() = row[col];
        }
    }
}

} // namespace excel
