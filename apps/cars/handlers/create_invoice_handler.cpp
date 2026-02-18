/**
 * CreateInvoiceHandler — parses createInvoice JSON payload, maps to excel::InvoiceData,
 * creates styled invoice via excel::createInvoice(), optionally opens file.
 */
#include "create_invoice_handler.h"
#include "../../../include/excel/excel_image.h"
#include "../../../include/excel/excel_invoice.h"
#include "../../../include/message_handler.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

/** Get user's Downloads directory for saving invoices. Returns empty on failure. */
std::string getDownloadsDir() {
#ifdef _WIN32
    const char* u = getenv("USERPROFILE");
    if (u) return std::string(u) + "\\Downloads";
    return "";
#else
    const char* home = getenv("HOME");
    if (!home) return "";
    std::string downloads = std::string(home) + "/Downloads";
#ifdef __APPLE__
    return downloads;
#else
    struct stat st;
    if (stat(downloads.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        return downloads;
    return std::string(home) + "/Documents";
#endif
#endif
}

std::string toAbsolutePath(const std::string& path) {
#ifdef _WIN32
    char buf[MAX_PATH];
    if (_fullpath(buf, path.c_str(), MAX_PATH)) return buf;
#else
    char* rp = realpath(path.c_str(), nullptr);
    if (rp) { std::string s(rp); free(rp); return s; }
#endif
    return path;
}

/** Generate full path for invoice: Downloads/invoice_YYYYMMDD_HHMMSS_xxx.xlsx */
std::string makeUniqueInvoicePath() {
    std::string dir = getDownloadsDir();
    if (!dir.empty()) {
#ifdef _WIN32
        if (dir.back() != '\\') dir += '\\';
#else
        if (dir.back() != '/') dir += '/';
#endif
    }
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    std::uniform_int_distribution<int> dist(0, 0xFFFF);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::ostringstream os;
    std::tm* tm = std::localtime(&t);
    if (tm) {
        os << "invoice_"
           << std::put_time(tm, "%Y%m%d_%H%M%S")
           << "_" << std::setfill('0') << std::setw(3) << ms.count();
    } else {
        os << "invoice_" << t << "_" << ms.count();
    }
    os << "_" << std::hex << std::setfill('0') << std::setw(4) << dist(gen);
    return dir + os.str() + ".xlsx";
}

bool openInDefaultApp(const std::string& path) {
    std::string absPath = toAbsolutePath(path);
#ifdef __APPLE__
    std::string cmd = "open \"" + absPath + "\"";
    return system(cmd.c_str()) == 0;
#elif defined(_WIN32)
    int r = (int)(INT_PTR)ShellExecuteA(NULL, "open", absPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return r > 32;
#else
    std::string cmd = "xdg-open \"" + absPath + "\" 2>/dev/null";
    int rc = system(cmd.c_str());
    return rc == 0 || (WIFEXITED(rc) && WEXITSTATUS(rc) != 2 && WEXITSTATUS(rc) != 4);
#endif
}

template<typename T>
T jsonGet(const nlohmann::json& j, const char* key, T def) {
    if (!j.contains(key)) return def;
    try {
        return j[key].get<T>();
    } catch (...) {
        return def;
    }
}

std::string jsonGetStr(const nlohmann::json& j, const char* key, const std::string& def = "") {
    if (!j.contains(key)) return def;
    if (j[key].is_string()) return j[key].get<std::string>();
    if (j[key].is_number()) return std::to_string(static_cast<int>(j[key].get<double>()));
    return def;
}

/** Base64 decode. Returns empty vector on invalid input. */
static std::vector<unsigned char> base64Decode(const std::string& encoded) {
    static const char kTable[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static int table[256];
    static bool init = false;
    if (!init) {
        for (int i = 0; i < 256; ++i) table[i] = -1;
        for (int i = 0; i < 64; ++i) table[static_cast<unsigned char>(kTable[i])] = i;
        init = true;
    }
    std::vector<unsigned char> out;
    int val = 0, bits = 0;
    for (unsigned char c : encoded) {
        if (c == '=') break;
        int v = table[c];
        if (v < 0) continue;
        val = (val << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<unsigned char>((val >> bits) & 0xFF));
        }
    }
    return out;
}

/** Write base64-encoded PNG to temp file. Returns path or empty on failure. */
static std::string writeLogoTempFile(const std::string& base64) {
    std::vector<unsigned char> data = base64Decode(base64);
    if (data.empty() || data.size() < 8) return "";

    std::string tmpPath;
#ifdef _WIN32
    const char* tmp = getenv("TEMP");
    std::string base = tmp ? tmp : ".";
    while (!base.empty() && (base.back() == '\\' || base.back() == '/')) base.pop_back();
    std::uniform_int_distribution<int> dist(100000, 999999);
    std::random_device rd;
    std::mt19937 gen(rd());
    tmpPath = base + "\\invoice_logo_" + std::to_string(dist(gen)) + ".png";
#else
    std::uniform_int_distribution<int> dist(100000, 999999);
    std::random_device rd;
    std::mt19937 gen(rd());
    tmpPath = "/tmp/invoice_logo_" + std::to_string(dist(gen)) + ".png";
#endif

    std::ofstream f(tmpPath, std::ios::binary);
    if (!f || !f.write(reinterpret_cast<const char*>(data.data()), data.size())) return "";
    return tmpPath;
}

excel::InvoiceData payloadToInvoiceData(const nlohmann::json& payload) {
    excel::InvoiceData data;

    data.invoiceNumber = jsonGetStr(payload, "ref");
    data.invoiceDate = jsonGetStr(payload, "date");
    data.subtotal = jsonGetStr(payload, "subtotal");
    data.taxLabel = jsonGetStr(payload, "taxLabel");
    data.taxAmount = jsonGetStr(payload, "taxAmount");
    data.total = jsonGetStr(payload, "total");
    data.currency = jsonGetStr(payload, "currency", "USD");

    if (payload.contains("client") && payload["client"].is_object()) {
        const auto& c = payload["client"];
        data.client.name = jsonGetStr(c, "name");
        data.client.phone = jsonGetStr(c, "mobile");
        data.client.address = jsonGetStr(c, "address");
        data.client.email = jsonGetStr(c, "email");
        std::string id = jsonGetStr(c, "id");
        if (!id.empty() && data.client.address.empty()) {
            data.client.address = id;
        } else if (!id.empty() && !data.client.address.empty()) {
            data.client.address += "\n" + id;
        }
    }

    if (payload.contains("company") && payload["company"].is_object()) {
        const auto& co = payload["company"];
        data.company.name = jsonGetStr(co, "name");
        data.company.address = jsonGetStr(co, "address");
        data.company.phone = jsonGetStr(co, "phone");
        data.company.email = jsonGetStr(co, "email");
        data.company.website = jsonGetStr(co, "website");
    } else {
        // Default company when not provided
        data.company.name = "Nordic Motors Inc.";
        data.company.address = "123 Commerce Street, Suite 400\nStockholm, SE-111 52";
        data.company.phone = "+46 8 123 45 67";
        data.company.email = "invoices@nordicmotors.se";
        data.company.website = "www.nordicmotors.se";
    }

    if (payload.contains("items") && payload["items"].is_array()) {
        for (const auto& it : payload["items"]) {
            excel::InvoiceLineItem item;
            item.description = jsonGetStr(it, "description");
            item.clientName = jsonGetStr(it, "clientName");
            item.port = jsonGetStr(it, "port");
            item.quantity = jsonGetStr(it, "qty", "1");
            item.unitPrice = jsonGetStr(it, "price");
            item.amount = jsonGetStr(it, "amount");
            if (item.amount.empty() && !item.quantity.empty() && !item.unitPrice.empty()) {
                try {
                    double q = std::stod(item.quantity);
                    double p = std::stod(item.unitPrice);
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(2) << (q * p);
                    item.amount = oss.str();
                } catch (...) {
                    item.amount = item.unitPrice;
                }
            }
            data.items.push_back(std::move(item));
        }
    }

    return data;
}

} // namespace

class CreateInvoiceHandler : public MessageHandler {
public:
    bool canHandle(const std::string& messageType) const override {
        return messageType == "createInvoice";
    }

    nlohmann::json handle(const nlohmann::json& payload, const std::string& requestId) override {
        (void)requestId;
        nlohmann::json result;

        if (!payload.is_object()) {
            result["success"] = false;
            result["error"] = "Payload must be an object";
            return result;
        }

        std::string ref = jsonGetStr(payload, "ref");
        std::string total = jsonGetStr(payload, "total");
        if (ref.empty()) {
            result["success"] = false;
            result["error"] = "Missing required field 'ref' (invoice reference)";
            return result;
        }
        if (total.empty()) {
            result["success"] = false;
            result["error"] = "Missing required field 'total'";
            return result;
        }

        excel::InvoiceData data = payloadToInvoiceData(payload);

        std::string outputPath = jsonGetStr(payload, "outputPath");
        if (outputPath.empty()) {
            outputPath = makeUniqueInvoicePath();
        }
        if (outputPath.find('.') == std::string::npos) {
            outputPath += ".xlsx";
        }

        std::string logoPath;
        std::string logoTempPath;
        if (payload.contains("logoBase64") && payload["logoBase64"].is_string()) {
            std::string base64 = payload["logoBase64"].get<std::string>();
            logoTempPath = writeLogoTempFile(base64);
            if (!logoTempPath.empty()) {
                logoPath = logoTempPath;
            }
        }
        if (logoPath.empty()) {
            logoPath = excel::resolveLogoPath();
        }

        if (!excel::createInvoice(outputPath, data, logoPath)) {
            if (!logoTempPath.empty()) {
#ifdef _WIN32
                std::remove(logoTempPath.c_str());
#else
                unlink(logoTempPath.c_str());
#endif
            }
            result["success"] = false;
            result["error"] = "Failed to create invoice file";
            return result;
        }

        if (!logoTempPath.empty()) {
#ifdef _WIN32
            std::remove(logoTempPath.c_str());
#else
            unlink(logoTempPath.c_str());
#endif
        }

        bool openAfter = true;
        if (payload.contains("openAfterCreate") && payload["openAfterCreate"].is_boolean()) {
            openAfter = payload["openAfterCreate"].get<bool>();
        }

        if (openAfter && !openInDefaultApp(outputPath)) {
            // Still success - file was created, just couldn't open
            result["success"] = true;
            result["path"] = outputPath;
            result["openFailed"] = true;
        } else {
            result["success"] = true;
            result["path"] = outputPath;
        }
        return result;
    }

    std::vector<std::string> getSupportedTypes() const override {
        return {"createInvoice"};
    }
};

std::shared_ptr<MessageHandler> createCreateInvoiceHandler() {
    return std::make_shared<CreateInvoiceHandler>();
}
