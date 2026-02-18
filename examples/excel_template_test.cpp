/**
 * Excel template test — uses reusable excel::ExcelExporter.
 * See docs/EXCEL_EXPORT_DESIGN.md for architecture.
 *
 * Run from cross_dev/ or with working dir = cross_dev (e.g. ./assets/...).
 * After saving, opens the output file in the default application.
 */
#include "excel/excel_exporter.h"
#include "excel/excel_image.h"
#include "excel/excel_layout.h"
#include "excel/excel_paths.h"
#include <chrono>
#include <cstdlib>
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
#include <sys/wait.h>
#include <unistd.h>
#endif

/** Resolve path to absolute for ShellExecute/xdg-open. Returns path unchanged if resolution fails. */
static std::string toAbsolutePath(const std::string& path) {
#ifdef _WIN32
    char buf[MAX_PATH];
    if (_fullpath(buf, path.c_str(), MAX_PATH)) return buf;
#else
    char* rp = realpath(path.c_str(), nullptr);
    if (rp) { std::string s(rp); free(rp); return s; }
#endif
    return path;
}

/** Generate a unique temporary filename for Excel output (excel_YYYYMMDD_HHMMSS_XXXX.xlsx). */
static std::string makeUniqueExcelOutputPath() {
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
        os << "excel_"
           << std::put_time(tm, "%Y%m%d_%H%M%S")
           << "_" << std::setfill('0') << std::setw(3) << ms.count();
    } else {
        os << "excel_" << t << "_" << ms.count();
    }
    os << "_" << std::hex << std::setfill('0') << std::setw(4) << dist(gen);
    return os.str() + ".xlsx";
}

/** On macOS: trigger Excel print dialog (Cmd+P) after opening. Does nothing on other platforms. */
static void printExcelAfterOpen() {
#ifdef __APPLE__
    // Give Excel time to load the workbook
    sleep(2);
    int r = system("osascript -e 'tell application \"Microsoft Excel\" to activate' "
                  "-e 'tell application \"System Events\" to tell process \"Microsoft Excel\" to keystroke \"p\" using command down' 2>/dev/null");
    (void)r;
#endif
}

static bool openInDefaultApp(const std::string& path) {
    std::string absPath = toAbsolutePath(path);
#ifdef __APPLE__
    std::string cmd = "open \"" + absPath + "\"";
    int r = system(cmd.c_str());
    return (r == 0);
#elif defined(_WIN32)
    int r = (int)(INT_PTR)ShellExecuteA(NULL, "open", absPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return (r > 32);  /* >32 = success, 31 = no associated app, 2 = file not found */
#else
    std::string cmd = "xdg-open \"" + absPath + "\" 2>/dev/null";
    int r = system(cmd.c_str());
    if (r == 0) return true;
    if (WIFEXITED(r) && (WEXITSTATUS(r) == 2 || WEXITSTATUS(r) == 4))
        return false;  /* xdg-open: 2=no handler, 4=action cancelled */
    return false;
#endif
}

int main(int argc, char* argv[]) {
    std::string templatePath = argc > 1 ? argv[1] : excel::resolveTemplatePath("template_sell_1.xlsx");
    if (templatePath.empty()) {
        std::cerr << "Excel template test: template not found. Tried assets/template_sell_1.xlsx, ../assets/...\n";
        std::cerr << "Usage: excel_template_test [path-to-template.xlsx]\n";
        return 1;
    }

    std::string outputPath = makeUniqueExcelOutputPath();
    std::cout << "Opening template: " << templatePath << "\n";

    excel::ExcelExporter exporter(templatePath);
    auto layout = ExcelLayouts::sellBill();

    std::cout << "Using sheet: " << exporter.worksheet(1).name() << "\n";

    exporter.setHeaderRow(layout.headerRow, {"ColA", "ColB", "ColC", "ColD", "ColE", "ColF", "ColG"});

    const std::vector<std::vector<std::string>> details = {
        {"Item1", "100", "200", "300", "400", "500", "600"},
        {"Item2", "101", "201", "301", "401", "501", "601"},
        {"Item3", "102", "202", "302", "402", "502", "602"},
    };
    exporter.fillDetailBlock(layout, details);

    if (!exporter.saveAs(outputPath)) {
        std::cerr << "Failed to save: " << outputPath << "\n";
        return 1;
    }
    std::cout << "Saved: " << outputPath << "\n";

    // Inject logo at top of worksheet
    std::string logoPath = excel::resolveLogoPath();
    if (!logoPath.empty()) {
        if (excel::injectLetterhead(outputPath, logoPath, 1.0, 1.0, true)) {
            std::cout << "Logo injected: " << logoPath << "\n";
        } else {
            std::cerr << "Warning: could not inject logo from " << logoPath << "\n";
        }
    } else {
        std::cerr << "Warning: logo.png not found (tried assets/...)\n";
    }

    if (!openInDefaultApp(outputPath)) {
        std::cerr << "Error: No default application to open .xlsx files.\n";
        std::cerr << "  Install Excel, LibreOffice Calc, or another spreadsheet app.\n";
        return 1;
    }
    std::cout << "Opened in default application.\n";
    printExcelAfterOpen();
    std::cout << "Done.\n";
    return 0;
}
