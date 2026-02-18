/**
 * Beautiful invoice test — creates a styled invoice with logo.
 * Uses excel::createInvoice() for reusable invoice generation.
 *
 * Run from cross_dev/ (e.g. ./build/examples/excel_invoice_test).
 */
#include "excel/excel_image.h"
#include "excel/excel_invoice.h"
#include <chrono>
#include <iomanip>
#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

static std::string makeUniquePath() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&t);
    std::ostringstream os;
    if (tm) {
        os << "invoice_";
        os << (tm->tm_year + 1900);
        os << std::setfill('0') << std::setw(2) << (tm->tm_mon + 1);
        os << std::setw(2) << tm->tm_mday << "_";
        os << std::setw(2) << tm->tm_hour << std::setw(2) << tm->tm_min;
        os << std::setw(2) << tm->tm_sec;
    } else {
        os << "invoice_" << t;
    }
    std::uniform_int_distribution<int> dist(0, 0xFFF);
    std::random_device rd;
    std::mt19937 gen(rd());
    os << "_" << std::hex << dist(gen);
    return os.str() + ".xlsx";
}

static bool openInDefaultApp(const std::string& path) {
#ifdef __APPLE__
    std::string cmd = "open \"" + path + "\"";
    return system(cmd.c_str()) == 0;
#elif defined(_WIN32)
    int r = (int)(INT_PTR)ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return r > 32;
#else
    std::string cmd = "xdg-open \"" + path + "\" 2>/dev/null";
    return system(cmd.c_str()) == 0;
#endif
}

int main() {
    excel::InvoiceData data;

    // Company
    data.company.name = "Nordic Motors Inc.";
    data.company.address = "123 Commerce Street, Suite 400\nStockholm, SE-111 52";
    data.company.phone = "+46 8 123 45 67";
    data.company.email = "invoices@nordicmotors.se";
    data.company.website = "www.nordicmotors.se";

    // Client
    data.client.name = "Algerian Auto Imports SAS";
    data.client.address = "42 Boulevard Mohamed Khemisti\nAlgiers 16000, Algeria";
    data.client.phone = "+213 21 45 67 89";
    data.client.email = "procurement@algerianauto.dz";

    // Invoice meta
    data.invoiceNumber = "INV-2025-0042";
    data.invoiceDate = "February 14, 2025";
    data.currency = "USD";

    // Line items: {description, clientName, port, quantity, unitPrice, amount}
    data.items = {
        {"2024 Toyota Camry SE — White exterior, Black interior", "Client A", "Algiers", "2", "28,500.00", "57,000.00"},
        {"2024 Honda CR-V EX — Pearl White, Leather seats", "Client B", "Oran", "1", "34,200.00", "34,200.00"},
        {"Freight & handling (Port of Alger to Algiers)", "", "", "—", "1,200.00", "1,200.00"},
        {"Documentation & customs clearance", "", "", "—", "450.00", "450.00"},
    };

    data.subtotal = "92,850.00";
    data.taxLabel = "VAT (19%)";
    data.taxAmount = "17,641.50";
    data.total = "110,491.50";

    std::string outputPath = makeUniquePath();
    std::cout << "Creating invoice: " << outputPath << "\n";

    std::string logoPath = excel::resolveLogoPath();
    if (!logoPath.empty()) {
        std::cout << "Using logo: " << logoPath << "\n";
    }

    if (!excel::createInvoice(outputPath, data, logoPath)) {
        std::cerr << "Failed to create invoice.\n";
        return 1;
    }
    std::cout << "Saved: " << outputPath << "\n";

    if (!openInDefaultApp(outputPath)) {
        std::cerr << "Could not open file in default app.\n";
        return 1;
    }
    std::cout << "Done.\n";
    return 0;
}
