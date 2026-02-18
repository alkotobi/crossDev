#ifndef EXCEL_INVOICE_H
#define EXCEL_INVOICE_H

#include <string>
#include <vector>

namespace excel {

/** Company (issuer) information for invoice. */
struct InvoiceCompany {
    std::string name;
    std::string address;
    std::string phone;
    std::string email;
    std::string website;
};

/** Client (bill-to) information for invoice. */
struct InvoiceClient {
    std::string name;
    std::string address;
    std::string phone;
    std::string email;
};

/** A single line item on the invoice. */
struct InvoiceLineItem {
    std::string description;
    std::string clientName;
    std::string port;
    std::string quantity;
    std::string unitPrice;
    std::string amount;
};

/** Complete invoice data. */
struct InvoiceData {
    InvoiceCompany company;
    InvoiceClient client;
    std::string invoiceNumber;
    std::string invoiceDate;
    std::vector<InvoiceLineItem> items;
    std::string subtotal;
    std::string taxLabel;
    std::string taxAmount;
    std::string total;
    std::string currency;
};

/**
 * Create a styled invoice workbook and save to path.
 * Uses logo and company name: logo at top-left (stretched to design size), company name below.
 * Injects logo at top if logoPath is non-empty.
 *
 * @param path Output .xlsx path
 * @param data Invoice content
 * @param logoPath Path to logo PNG, or empty to skip; use resolveLogoPath() for auto-resolve
 * @return true on success
 */
bool createInvoice(const std::string& path, const InvoiceData& data,
                   const std::string& logoPath = "");

/** Resolve logo path: logo.png in assets. Returns empty if not found. */
std::string resolveLogoPath();

} // namespace excel

#endif // EXCEL_INVOICE_H
