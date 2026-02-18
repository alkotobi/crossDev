#ifndef EXCEL_LAYOUT_H
#define EXCEL_LAYOUT_H

#include <cstdint>

/**
 * Layout configuration for a template's detail block.
 * Use presets from ExcelLayouts for known templates.
 */
struct ExcelDetailBlock {
    uint32_t headerRow;       // Row with column headers (e.g. 6)
    uint32_t detailStartRow;   // First detail row, has formatting in template (e.g. 7)
    uint32_t detailNumCols;    // Columns for details (e.g. 7 for A..G)
    uint32_t shiftNumCols;     // Columns to shift when inserting rows (e.g. 16 for A..P)
};

namespace ExcelLayouts {

/** Sell bill template: headers row 6, details start row 7, A..G details, A..P shift */
inline ExcelDetailBlock sellBill() {
    return {6, 7, 7, 16};
}

} // namespace ExcelLayouts

#endif // EXCEL_LAYOUT_H
