#ifndef EXCEL_SHEET_OPS_H
#define EXCEL_SHEET_OPS_H

#include "excel_layout.h"
#include <OpenXLSX.hpp>
#include <string>
#include <vector>

namespace excel {

/**
 * Shift rows [firstRow..lastRow] down by rowCount.
 * Copies cell content (and format) so content below details is not overwritten.
 * Process from lastRow down to firstRow to avoid overwriting source.
 */
void shiftRowsDown(OpenXLSX::XLWorksheet& wks,
                  uint32_t firstRow, uint32_t lastRow,
                  int rowCount, int numCols);

/**
 * Copy format (and value) from srcRow to destRow for columns 1..numCols.
 * Like Excel "Insert row below" - new row inherits source formatting.
 */
void copyRowFormatToRow(OpenXLSX::XLWorksheet& wks,
                        uint32_t srcRow, uint32_t destRow,
                        int numCols);

/**
 * Fill detail block: shift existing rows if needed, then write detail rows
 * with format copied from detailStartRow.
 */
void fillDetailBlock(OpenXLSX::XLWorksheet& wks,
                     const ExcelDetailBlock& layout,
                     const std::vector<std::vector<std::string>>& detailRows);

} // namespace excel

#endif // EXCEL_SHEET_OPS_H
