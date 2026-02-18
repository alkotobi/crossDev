/**
 * Reusable invoice generation: creates a styled Excel invoice from data.
 */
#include "excel/excel_invoice.h"
#include "excel/excel_image.h"
#include <OpenXLSX.hpp>
#include <stdexcept>

namespace excel {

namespace {

using namespace OpenXLSX;

// Layout: logo left (col A), company block right (col B) — 3 lines, logo height = 3-line block
constexpr uint32_t ROW_COMPANY_NAME = 1;      // Line 1: company name (big font)
constexpr uint32_t ROW_COMPANY_ADDRESS = 2;   // Line 2: address (small font)
constexpr uint32_t ROW_COMPANY_CONTACT = 3;   // Line 3: mobile | email | website (small font)
constexpr uint32_t ROW_INVOICE_TITLE = 6;     // "INVOICE" on its own line
constexpr uint32_t ROW_BILL_TO = 7;          // Bill To and client data start here
constexpr uint32_t ROW_TABLE_HEADER = 12;     // Details table header
constexpr uint32_t COL_COMPANY = 2;          // Company block to the right of logo (col B)
constexpr uint32_t COL_NO = 1;
constexpr uint32_t COL_DESC = 2;
constexpr uint32_t COL_DESC_END = 3;        // Description spans B–C
constexpr uint32_t COL_CLIENT = 4;
constexpr uint32_t COL_PORT = 5;
constexpr uint32_t COL_QTY = 6;
constexpr uint32_t COL_UNIT = 7;
constexpr uint32_t COL_AMOUNT = 8;

static std::string joinContactLine(const InvoiceCompany& co) {
    std::string s;
    if (!co.phone.empty())   s += co.phone;
    if (!co.email.empty())   { if (!s.empty()) s += "  |  "; s += co.email; }
    if (!co.website.empty()) { if (!s.empty()) s += "  |  "; s += co.website; }
    return s;
}

void writeCompany(XLWorksheet& wks, OpenXLSX::XLStyles& styles,
                  const InvoiceCompany& co,
                  XLStyleIndex companyNameFmt, XLStyleIndex companyDetailsFmt) {
    // Line 1: company name — big font (right of logo)
    wks.cell(ROW_COMPANY_NAME, COL_COMPANY).value() = co.name;
    wks.cell(ROW_COMPANY_NAME, COL_COMPANY).setCellFormat(companyNameFmt);
    // Line 2: company address — small font
    wks.cell(ROW_COMPANY_ADDRESS, COL_COMPANY).value() = co.address;
    wks.cell(ROW_COMPANY_ADDRESS, COL_COMPANY).setCellFormat(companyDetailsFmt);
    // Line 3: mobile | email | website — small font
    std::string contactLine = joinContactLine(co);
    wks.cell(ROW_COMPANY_CONTACT, COL_COMPANY).value() = contactLine;
    wks.cell(ROW_COMPANY_CONTACT, COL_COMPANY).setCellFormat(companyDetailsFmt);
}

void writeInvoiceMeta(XLWorksheet& wks, const InvoiceData& data, XLStyleIndex metaFmt) {
    wks.cell(ROW_BILL_TO, 4).value() = "Invoice #: " + data.invoiceNumber;
    wks.cell(ROW_BILL_TO, 4).setCellFormat(metaFmt);
    wks.cell(ROW_BILL_TO + 1, 4).value() = "Date: " + data.invoiceDate;
    wks.cell(ROW_BILL_TO + 1, 4).setCellFormat(metaFmt);
}

void writeBillTo(XLWorksheet& wks, const InvoiceClient& client, XLStyleIndex clientFmt) {
    // Bill To and name on same line
    std::string billToLine = "Bill To: " + client.name;
    wks.cell(ROW_BILL_TO, 1).value() = billToLine;
    wks.cell(ROW_BILL_TO, 1).setCellFormat(clientFmt);
    uint32_t r = ROW_BILL_TO + 1;
    if (!client.address.empty()) { wks.cell(r, 1).value() = client.address; wks.cell(r, 1).setCellFormat(clientFmt); r++; }
    if (!client.phone.empty())  { wks.cell(r, 1).value() = client.phone;  wks.cell(r, 1).setCellFormat(clientFmt); r++; }
    if (!client.email.empty()) { wks.cell(r, 1).value() = client.email;  wks.cell(r, 1).setCellFormat(clientFmt); }
}

static std::string rangeStr(uint32_t row, uint16_t col1, uint16_t col2) {
    auto colLetter = [](uint16_t c) {
        std::string s;
        while (c > 0) { s = char('A' + (c - 1) % 26) + s; c = (c - 1) / 26; }
        return s;
    };
    return colLetter(col1) + std::to_string(row) + ":" + colLetter(col2) + std::to_string(row);
}

void writeLineItems(XLWorksheet& wks, OpenXLSX::XLStyles& styles,
                    const std::vector<InvoiceLineItem>& items,
                    XLStyleIndex headerFmt, XLStyleIndex rowFmt) {
    // Header: No | Description | Client | Port | Qty | Unit Price | Amount
    wks.cell(ROW_TABLE_HEADER, COL_NO).value() = "No";
    wks.cell(ROW_TABLE_HEADER, COL_DESC).value() = "Description";
    wks.cell(ROW_TABLE_HEADER, COL_CLIENT).value() = "Client";
    wks.cell(ROW_TABLE_HEADER, COL_PORT).value() = "Port";
    wks.cell(ROW_TABLE_HEADER, COL_QTY).value() = "Qty";
    wks.cell(ROW_TABLE_HEADER, COL_UNIT).value() = "Unit Price";
    wks.cell(ROW_TABLE_HEADER, COL_AMOUNT).value() = "Amount";
    wks.range(rangeStr(ROW_TABLE_HEADER, COL_NO, COL_AMOUNT)).setFormat(headerFmt);
    wks.mergeCells(rangeStr(ROW_TABLE_HEADER, COL_DESC, COL_DESC_END), true);

    for (size_t i = 0; i < items.size(); ++i) {
        uint32_t r = ROW_TABLE_HEADER + 1 + static_cast<uint32_t>(i);
        const auto& it = items[i];
        wks.cell(r, COL_NO).value() = static_cast<int>(i + 1);
        wks.cell(r, COL_DESC).value() = it.description;
        wks.cell(r, COL_CLIENT).value() = it.clientName;
        wks.cell(r, COL_PORT).value() = it.port;
        wks.cell(r, COL_QTY).value() = it.quantity;
        wks.cell(r, COL_UNIT).value() = it.unitPrice;
        wks.cell(r, COL_AMOUNT).value() = it.amount;
        wks.range(rangeStr(r, COL_NO, COL_AMOUNT)).setFormat(rowFmt);
        wks.mergeCells(rangeStr(r, COL_DESC, COL_DESC_END), true);
    }
}

void writeTotals(XLWorksheet& wks, const InvoiceData& data,
                 uint32_t startRow, XLStyleIndex labelFmt, XLStyleIndex totalFmt) {
    uint32_t r = startRow;
    wks.cell(r, COL_UNIT).value() = "Subtotal:";
    wks.cell(r, COL_UNIT).setCellFormat(labelFmt);
    wks.cell(r, COL_AMOUNT).value() = data.subtotal;
    wks.cell(r, COL_AMOUNT).setCellFormat(labelFmt);
    r++;
    if (!data.taxLabel.empty()) {
        wks.cell(r, COL_UNIT).value() = data.taxLabel + ":";
        wks.cell(r, COL_UNIT).setCellFormat(labelFmt);
        wks.cell(r, COL_AMOUNT).value() = data.taxAmount;
        wks.cell(r, COL_AMOUNT).setCellFormat(labelFmt);
        r++;
    }
    wks.cell(r, COL_UNIT).value() = "Total:";
    wks.cell(r, COL_UNIT).setCellFormat(totalFmt);
    wks.cell(r, COL_AMOUNT).value() = data.total;
    wks.cell(r, COL_AMOUNT).setCellFormat(totalFmt);
}

} // namespace

bool createInvoice(const std::string& path, const InvoiceData& data,
                   const std::string& logoPath) {
    try {
        OpenXLSX::XLDocument doc;
        doc.create(path, OpenXLSX::XLForceOverwrite);

        OpenXLSX::XLWorksheet wks = doc.workbook().worksheet(1);
        wks.setName("Invoice");

        auto& styles = doc.styles();
        auto& fonts = styles.fonts();
        auto& fills = styles.fills();
        auto& borders = styles.borders();
        auto& cellFormats = styles.cellFormats();

        // --- Create styles ---
        // Title: large bold dark blue
        OpenXLSX::XLStyleIndex fontTitleIdx = fonts.create();
        fonts[fontTitleIdx].setFontName("Arial");
        fonts[fontTitleIdx].setFontSize(18);
        fonts[fontTitleIdx].setBold();
        fonts[fontTitleIdx].setFontColor(OpenXLSX::XLColor("ff1a5276"));

        OpenXLSX::XLStyleIndex titleFmt = cellFormats.create();
        cellFormats[titleFmt].setFontIndex(fontTitleIdx);
        cellFormats[titleFmt].setApplyFont(true);

        // "INVOICE" on its own line
        wks.cell(ROW_INVOICE_TITLE, 4).value() = "INVOICE";
        wks.cell(ROW_INVOICE_TITLE, 4).setCellFormat(titleFmt);

        // Company name: big font (20pt bold, dark slate) — line 1 right of logo
        OpenXLSX::XLStyleIndex fontCompanyNameIdx = fonts.create();
        fonts[fontCompanyNameIdx].setFontName("Arial");
        fonts[fontCompanyNameIdx].setFontSize(20);
        fonts[fontCompanyNameIdx].setBold();
        fonts[fontCompanyNameIdx].setFontColor(OpenXLSX::XLColor("ff2c3e50"));

        OpenXLSX::XLStyleIndex companyNameFmt = cellFormats.create();
        cellFormats[companyNameFmt].setFontIndex(fontCompanyNameIdx);
        cellFormats[companyNameFmt].setApplyFont(true);

        // Company details: small font (9pt, gray) — line 2 address, line 3 contact
        OpenXLSX::XLStyleIndex fontCompanyDetailsIdx = fonts.create();
        fonts[fontCompanyDetailsIdx].setFontName("Arial");
        fonts[fontCompanyDetailsIdx].setFontSize(9);
        fonts[fontCompanyDetailsIdx].setFontColor(OpenXLSX::XLColor("ff5d6d7e"));

        OpenXLSX::XLStyleIndex companyDetailsFmt = cellFormats.create();
        cellFormats[companyDetailsFmt].setFontIndex(fontCompanyDetailsIdx);
        cellFormats[companyDetailsFmt].setApplyFont(true);
        cellFormats[companyDetailsFmt].setApplyAlignment(true);
        cellFormats[companyDetailsFmt].alignment(OpenXLSX::XLCreateIfMissing).setWrapText(true);

        // Meta (invoice #, date): right-aligned
        OpenXLSX::XLStyleIndex fontMetaIdx = fonts.create();
        fonts[fontMetaIdx].setFontName("Arial");
        fonts[fontMetaIdx].setFontSize(10);

        OpenXLSX::XLStyleIndex metaFmt = cellFormats.create();
        cellFormats[metaFmt].setFontIndex(fontMetaIdx);
        cellFormats[metaFmt].setApplyFont(true);
        cellFormats[metaFmt].alignment().setHorizontal(OpenXLSX::XLAlignRight);

        // Bill To label
        OpenXLSX::XLStyleIndex clientFmt = cellFormats.create();
        cellFormats[clientFmt].setFontIndex(fontMetaIdx);
        cellFormats[clientFmt].setApplyFont(true);

        // Table header: gray fill, bold, border (9pt for printing)
        OpenXLSX::XLStyleIndex fontHeaderIdx = fonts.create();
        fonts[fontHeaderIdx].setFontName("Arial");
        fonts[fontHeaderIdx].setFontSize(9);
        fonts[fontHeaderIdx].setBold();
        fonts[fontHeaderIdx].setFontColor(OpenXLSX::XLColor("ffffffff"));

        OpenXLSX::XLStyleIndex fillHeaderIdx = fills.create();
        fills[fillHeaderIdx].setFillType(OpenXLSX::XLPatternFill);
        fills[fillHeaderIdx].setPatternType(OpenXLSX::XLPatternSolid);
        fills[fillHeaderIdx].setColor(OpenXLSX::XLColor("ff3498db"));

        OpenXLSX::XLStyleIndex borderHeaderIdx = borders.create();
        borders[borderHeaderIdx].setOutline(true);
        borders[borderHeaderIdx].setLeft(OpenXLSX::XLLineStyleThin, OpenXLSX::XLColor("ff2c3e50"));
        borders[borderHeaderIdx].setRight(OpenXLSX::XLLineStyleThin, OpenXLSX::XLColor("ff2c3e50"));
        borders[borderHeaderIdx].setTop(OpenXLSX::XLLineStyleThin, OpenXLSX::XLColor("ff2c3e50"));
        borders[borderHeaderIdx].setBottom(OpenXLSX::XLLineStyleThin, OpenXLSX::XLColor("ff2c3e50"));

        OpenXLSX::XLStyleIndex headerFmt = cellFormats.create();
        cellFormats[headerFmt].setFontIndex(fontHeaderIdx);
        cellFormats[headerFmt].setFillIndex(fillHeaderIdx);
        cellFormats[headerFmt].setBorderIndex(borderHeaderIdx);
        cellFormats[headerFmt].setApplyFont(true);
        cellFormats[headerFmt].setApplyFill(true);
        cellFormats[headerFmt].setApplyBorder(true);
        cellFormats[headerFmt].alignment().setHorizontal(OpenXLSX::XLAlignCenter);

        // Data rows: light border (9pt for printing)
        OpenXLSX::XLStyleIndex fontRowIdx = fonts.create();
        fonts[fontRowIdx].setFontName("Arial");
        fonts[fontRowIdx].setFontSize(9);

        OpenXLSX::XLStyleIndex borderRowIdx = borders.create();
        borders[borderRowIdx].setOutline(true);
        borders[borderRowIdx].setLeft(OpenXLSX::XLLineStyleHair, OpenXLSX::XLColor("ffbdc3c7"));
        borders[borderRowIdx].setRight(OpenXLSX::XLLineStyleHair, OpenXLSX::XLColor("ffbdc3c7"));
        borders[borderRowIdx].setTop(OpenXLSX::XLLineStyleHair, OpenXLSX::XLColor("ffbdc3c7"));
        borders[borderRowIdx].setBottom(OpenXLSX::XLLineStyleHair, OpenXLSX::XLColor("ffbdc3c7"));

        OpenXLSX::XLStyleIndex rowFmt = cellFormats.create();
        cellFormats[rowFmt].setFontIndex(fontRowIdx);
        cellFormats[rowFmt].setBorderIndex(borderRowIdx);
        cellFormats[rowFmt].setApplyFont(true);
        cellFormats[rowFmt].setApplyBorder(true);
        cellFormats[rowFmt].setApplyAlignment(true);
        cellFormats[rowFmt].alignment(OpenXLSX::XLCreateIfMissing).setWrapText(true);

        // Totals: bold (10pt, slightly larger than table)
        OpenXLSX::XLStyleIndex fontTotalIdx = fonts.create();
        fonts[fontTotalIdx].setFontName("Arial");
        fonts[fontTotalIdx].setFontSize(10);
        fonts[fontTotalIdx].setBold();

        OpenXLSX::XLStyleIndex totalFmt = cellFormats.create();
        cellFormats[totalFmt].setFontIndex(fontTotalIdx);
        cellFormats[totalFmt].setApplyFont(true);
        cellFormats[totalFmt].alignment().setHorizontal(OpenXLSX::XLAlignRight);

        OpenXLSX::XLStyleIndex labelFmt = cellFormats.create();
        cellFormats[labelFmt].setFontIndex(fontRowIdx);
        cellFormats[labelFmt].setApplyFont(true);
        cellFormats[labelFmt].alignment().setHorizontal(OpenXLSX::XLAlignRight);

        // --- Write content ---
        writeCompany(wks, styles, data.company, companyNameFmt, companyDetailsFmt);
        writeInvoiceMeta(wks, data, metaFmt);
        writeBillTo(wks, data.client, clientFmt);
        writeLineItems(wks, styles, data.items, headerFmt, rowFmt);

        uint32_t totalsStart = ROW_TABLE_HEADER + 2 + static_cast<uint32_t>(data.items.size());
        writeTotals(wks, data, totalsStart, labelFmt, totalFmt);

        // Column widths: col A = logo + table "No", B = company + description, etc.
        wks.column(1).setWidth(14);   // Logo (rows 1–3) + table No
        wks.column(2).setWidth(16);   // Company block (rows 1–3) + description (spans B–C)
        wks.column(3).setWidth(10);
        wks.column(4).setWidth(12);   // Client
        wks.column(5).setWidth(10);   // Port
        wks.column(6).setWidth(5);    // Qty
        wks.column(7).setWidth(9);    // Unit Price
        wks.column(8).setWidth(10);   // Amount

        doc.saveAs(path, OpenXLSX::XLForceOverwrite);
        doc.close();

        if (!logoPath.empty()) {
            // Logo at top-left: height = height of 3-line company block (~0.7"), width 1.0"
            injectLetterhead(path, logoPath, 1.0, 0.7, true);  // includes pageSetup
        } else {
            injectPrintFitToPage(path);  // pageSetup only
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace excel
