/**
 * Excel image injection: add logo at top of worksheet.
 * Post-processes .xlsx (zip) using unzip/zip to avoid duplicate symbols with OpenXLSX/Zippy.
 */
#include "excel/excel_image.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <chrono>
#include <random>

#if defined(__APPLE__) || defined(__linux__)
#include <limits.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#endif

namespace excel {

std::string resolveLetterheadPath(const std::string& filename) {
    std::string candidates[] = {
        filename,
        "assets/" + filename,
        "../assets/" + filename,
        "../../assets/" + filename,
        "cross_dev/assets/" + filename,
    };
    for (const auto& p : candidates) {
        std::ifstream f(p);
        if (f.good()) return p;
    }
    return "";
}

std::string resolveLogoPath() {
    return resolveLetterheadPath("logo.png");
}

bool getPngDimensions(const std::string& path, int& outWidth, int& outHeight) {
    std::ifstream f(path, std::ios::binary);
    if (!f || !f.seekg(16)) return false;
    uint8_t buf[8];
    if (!f.read(reinterpret_cast<char*>(buf), 8)) return false;
    outWidth = static_cast<int>((static_cast<uint32_t>(buf[0]) << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
    outHeight = static_cast<int>((static_cast<uint32_t>(buf[4]) << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7]);
    return outWidth > 0 && outHeight > 0;
}

namespace {

// Insert <drawing r:id="rIdN"/> into worksheet XML, before </worksheet>
bool insertDrawingRef(std::string& xml, const std::string& rId) {
    const std::string marker = "</worksheet>";
    auto pos = xml.rfind(marker);
    if (pos == std::string::npos) return false;
    std::string insert = "  <drawing r:id=\"" + rId + "\"/>\n";
    xml.insert(pos, insert);
    return true;
}

// Add Relationship to sheet rels; return new rId (e.g. "rId5")
std::string addDrawingRelationship(std::string& relsXml, const std::string& drawingTarget) {
    int maxId = 0;
    std::string needle = "Id=\"rId";
    size_t p = 0;
    while ((p = relsXml.find(needle, p)) != std::string::npos) {
        p += needle.size();
        int n = 0;
        while (p < relsXml.size() && relsXml[p] >= '0' && relsXml[p] <= '9') {
            n = n * 10 + (relsXml[p] - '0');
            p++;
        }
        if (n > maxId) maxId = n;
    }
    std::string newId = "rId" + std::to_string(maxId + 1);
    const std::string rel = "<Relationship Id=\"" + newId +
                            "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/drawing\" Target=\"" +
                            drawingTarget + "\"/>\n";
    const std::string marker = "</Relationships>";
    auto pos = relsXml.rfind(marker);
    if (pos == std::string::npos) return "";
    relsXml.insert(pos, rel);
    return newId;
}

bool addContentTypeOverrides(std::string& xml,
                             const std::string& partName,
                             const std::string& contentType) {
    const std::string override =
        "<Override PartName=\"" + partName + "\" ContentType=\"" + contentType + "\"/>\n";
    const std::string marker = "</Types>";
    auto pos = xml.rfind(marker);
    if (pos == std::string::npos) return false;
    xml.insert(pos, override);
    return true;
}

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    if (!f) return false;
    f << content;
    return true;
}

bool copyFile(const std::string& src, const std::string& dst) {
    std::ifstream in(src, std::ios::binary);
    std::ofstream out(dst, std::ios::binary);
    if (!in || !out) return false;
    out << in.rdbuf();
    return true;
}

#ifdef _WIN32
/**
 * Zip directory to outPath with [Content_Types].xml first (Excel expects this order).
 * Uses .NET ZipFile since Compress-Archive does not control entry order.
 */
static bool zipWithContentTypesFirst(const std::string& tmpBase, const std::string& outPath) {
    std::string scriptPath = tmpBase + "\\_zip.ps1";
    std::ofstream ps(scriptPath);
    if (!ps) return false;
    ps << R"(
param($tb, $op)
Add-Type -AssemblyName System.IO.Compression.FileSystem
if (Test-Path $op) { Remove-Item $op -Force }
$z = [System.IO.Compression.ZipFile]::Open($op, [System.IO.Compression.ZipArchiveMode]::Create)
try {
  $ct = Join-Path $tb '[Content_Types].xml'
  if (Test-Path $ct) { [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($z, $ct, '[Content_Types].xml') | Out-Null }
  @('_rels','docProps','xl') | ForEach-Object {
    $dir = Join-Path $tb $_
    if (Test-Path $dir) {
      Get-ChildItem -Path $dir -Recurse -File | ForEach-Object {
        $rel = $_.FullName.Substring($tb.Length + 1).Replace('\', '/')
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($z, $_.FullName, $rel) | Out-Null
      }
    }
  }
} finally { $z.Dispose() }
)";
    ps.close();
    std::string cmd = "powershell -NoProfile -ExecutionPolicy Bypass -File \"" + scriptPath + "\" -tb \"" + tmpBase + "\" -op \"" + outPath + "\"";
    int r = system(cmd.c_str());
    std::remove(scriptPath.c_str());
    return (r == 0);
}
#endif

} // namespace

bool injectLetterhead(const std::string& xlsxPath, const std::string& imagePath,
                      double widthInches, double heightInches, bool stretchToFill) {
    if (widthInches <= 0) widthInches = 7.0;
    if (heightInches <= 0) heightInches = 1.5;
    int64_t cx, cy;
    int imgW = 0, imgH = 0;
    if (!stretchToFill && getPngDimensions(imagePath, imgW, imgH) && imgW > 0 && imgH > 0) {
        // Preserve aspect ratio: fit within max width x max height
        double aspect = static_cast<double>(imgW) / static_cast<double>(imgH);
        double dispW = widthInches, dispH = heightInches;
        if (dispW / dispH > aspect) {
            dispW = dispH * aspect;  // height constrains
        } else {
            dispH = dispW / aspect;  // width constrains
        }
        cx = static_cast<int64_t>(dispW * 914400);
        cy = static_cast<int64_t>(dispH * 914400);
    } else {
        // Stretch to fill the given dimensions (or fallback if dimensions unknown)
        cx = static_cast<int64_t>(widthInches * 914400);
        cy = static_cast<int64_t>(heightInches * 914400);
    }
    std::ifstream checkImg(imagePath, std::ios::binary);
    if (!checkImg) return false;
    checkImg.seekg(0, std::ios::end);
    if (checkImg.tellg() <= 0) return false;
    checkImg.close();

    std::ifstream checkXlsx(xlsxPath);
    if (!checkXlsx) return false;
    checkXlsx.close();

    // Use temp dir: unzip xlsx, add files, zip back
    std::string tmpBase;
    {
        unsigned seed = static_cast<unsigned>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> dist(100000, 999999);
#ifdef _WIN32
        std::string base = std::getenv("TEMP") ? std::getenv("TEMP") : ".";
        while (!base.empty() && (base.back() == '\\' || base.back() == '/')) base.pop_back();
        tmpBase = base + "\\excel_letterhead_" + std::to_string(dist(rng));
        if (_mkdir(tmpBase.c_str()) != 0) return false;
#else
        tmpBase = "/tmp/excel_letterhead_" + std::to_string(dist(rng));
        if (mkdir(tmpBase.c_str(), 0755) != 0) return false;
#endif
    }
    bool ok = false;

    auto cleanup = [&]() {
#ifdef _WIN32
        (void)system(("rd /s /q \"" + tmpBase + "\"").c_str());
#else
        (void)system(("rm -rf \"" + tmpBase + "\"").c_str());
#endif
    };

#ifdef _WIN32
    int unzipR = system(("powershell -Command \"Expand-Archive -Path '" + xlsxPath + "' -DestinationPath '" + tmpBase + "'\"").c_str());
#else
    int unzipR = system(("unzip -q -o \"" + xlsxPath + "\" -d \"" + tmpBase + "\"").c_str());
#endif
    if (unzipR != 0) {
        cleanup();
        return false;
    }

    // Copy image to xl/media/
    std::string mediaDir = tmpBase + "/xl/media";
#ifdef _WIN32
    _mkdir((tmpBase + "/xl").c_str());
    _mkdir(mediaDir.c_str());
#else
    mkdir((tmpBase + "/xl").c_str(), 0755);
    mkdir(mediaDir.c_str(), 0755);
#endif
    if (!copyFile(imagePath, mediaDir + "/image1.png")) {
        cleanup();
        return false;
    }

    // Create drawing XML: oneCellAnchor with xdr:ext (required at anchor level) + spPr extent
    std::string drawingXml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<xdr:wsDr xmlns:xdr=\"http://schemas.openxmlformats.org/drawingml/2006/spreadsheetDrawing\" "
        "xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">\n"
        "<xdr:oneCellAnchor>\n"
        "<xdr:from><xdr:col>0</xdr:col><xdr:colOff>0</xdr:colOff><xdr:row>0</xdr:row><xdr:rowOff>0</xdr:rowOff></xdr:from>\n"
        "<xdr:ext cx=\"" + std::to_string(cx) + "\" cy=\"" + std::to_string(cy) + "\"/>\n"
        "<xdr:pic>\n"
        "<xdr:nvPicPr><xdr:cNvPr id=\"2\" name=\"Logo\"/><xdr:cNvPicPr/></xdr:nvPicPr>\n"
        "<xdr:blipFill><a:blip r:embed=\"rId1\"/><a:stretch><a:fillRect/></a:stretch></xdr:blipFill>\n"
        "<xdr:spPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"" +
        std::to_string(cx) + "\" cy=\"" + std::to_string(cy) + "\"/></a:xfrm>"
        "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom><a:noFill/></xdr:spPr>\n"
        "</xdr:pic>\n"
        "<xdr:clientData/>\n"
        "</xdr:oneCellAnchor>\n"
        "</xdr:wsDr>\n";

    std::string drawsDir = tmpBase + "/xl/drawings";
    std::string drawsRelsDir = drawsDir + "/_rels";
#ifdef _WIN32
    _mkdir(drawsDir.c_str());
    _mkdir(drawsRelsDir.c_str());
#else
    mkdir(drawsDir.c_str(), 0755);
    mkdir(drawsRelsDir.c_str(), 0755);
#endif
    if (!writeFile(drawsDir + "/drawing1.xml", drawingXml)) {
        cleanup();
        return false;
    }
    const char* drawingRels = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/image" Target="../media/image1.png"/>
</Relationships>
)";
    if (!writeFile(drawsRelsDir + "/drawing1.xml.rels", drawingRels)) {
        cleanup();
        return false;
    }

    // Update sheet1
    std::string sheetPath = tmpBase + "/xl/worksheets/sheet1.xml";
    std::string sheetRelsDir = tmpBase + "/xl/worksheets/_rels";
    std::string sheetRelsPath = sheetRelsDir + "/sheet1.xml.rels";
    std::string sheetXml = readFile(sheetPath);
    if (sheetXml.empty()) {
        cleanup();
        return false;
    }
    std::string sheetRels = readFile(sheetRelsPath);
    if (sheetRels.empty()) {
        // Worksheet may not have _rels yet; create it
#ifdef _WIN32
        _mkdir(sheetRelsDir.c_str());
#else
        mkdir(sheetRelsDir.c_str(), 0755);
#endif
        sheetRels = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>
)";
    }
    std::string rId = addDrawingRelationship(sheetRels, "../drawings/drawing1.xml");
    if (rId.empty() || !insertDrawingRef(sheetXml, rId)) {
        cleanup();
        return false;
    }

    // Add pageSetup (fit to one page) in same pass to avoid second unzip/zip
    const std::string pageSetup = "\n  <pageSetup paperSize=\"9\" orientation=\"portrait\" fitToPage=\"1\" fitToWidth=\"1\" fitToHeight=\"1\"/>";
    const std::string pmMarker = "<pageMargins ";
    auto pmStart = sheetXml.find(pmMarker);
    if (pmStart != std::string::npos) {
        auto pmEnd = sheetXml.find("/>", pmStart);
        if (pmEnd != std::string::npos) {
            sheetXml.insert(pmEnd + 2, pageSetup);
        }
    }

    if (!writeFile(sheetPath, sheetXml) || !writeFile(sheetRelsPath, sheetRels)) {
        cleanup();
        return false;
    }

    // Update [Content_Types].xml
    std::string ctPath = tmpBase + "/[Content_Types].xml";
    std::string ctXml = readFile(ctPath);
    if (ctXml.empty()) {
        cleanup();
        return false;
    }
    addContentTypeOverrides(ctXml, "/xl/drawings/drawing1.xml",
                           "application/vnd.openxmlformats-officedocument.drawing+xml");
    addContentTypeOverrides(ctXml, "/xl/media/image1.png", "image/png");
    if (!writeFile(ctPath, ctXml)) {
        cleanup();
        return false;
    }

    // Zip back - use absolute path so output is next to original (not inside temp dir)
    std::string outPath = xlsxPath + ".new";
#if defined(__APPLE__) || defined(__linux__)
    {
        size_t sep = xlsxPath.rfind('/');
        std::string dir = (sep != std::string::npos) ? xlsxPath.substr(0, sep + 1) : "";
        char resolved[PATH_MAX];
        if (realpath(dir.empty() ? "." : dir.c_str(), resolved)) {
            outPath = std::string(resolved) + "/" +
                      ((sep != std::string::npos) ? xlsxPath.substr(sep + 1) : xlsxPath) + ".new";
        }
    }
#endif
#ifdef _WIN32
    int zipR = zipWithContentTypesFirst(tmpBase, outPath) ? 0 : 1;
#else
    // Excel expects [Content_Types].xml first in zip; explicit order avoids recovery dialog
    int zipR = system(("cd \"" + tmpBase + "\" && zip -X -r -q \"" + outPath + "\" \"[Content_Types].xml\" _rels docProps xl").c_str());
#endif
    if (zipR != 0) {
        cleanup();
        return false;
    }

    cleanup();

#ifdef _WIN32
    if (copyFile(outPath, xlsxPath)) {
        std::remove(outPath.c_str());
        ok = true;
    }
#else
    if (rename(outPath.c_str(), xlsxPath.c_str()) == 0) {
        ok = true;
    } else if (copyFile(outPath, xlsxPath)) {
        unlink(outPath.c_str());
        ok = true;
    }
#endif

    return ok;
}

bool injectPrintFitToPage(const std::string& xlsxPath) {
    std::ifstream checkXlsx(xlsxPath);
    if (!checkXlsx) return false;
    checkXlsx.close();

    std::string tmpBase;
    {
        unsigned seed = static_cast<unsigned>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> dist(100000, 999999);
#ifdef _WIN32
        std::string base = std::getenv("TEMP") ? std::getenv("TEMP") : ".";
        while (!base.empty() && (base.back() == '\\' || base.back() == '/')) base.pop_back();
        tmpBase = base + "\\excel_fit_" + std::to_string(dist(rng));
        if (_mkdir(tmpBase.c_str()) != 0) return false;
#else
        tmpBase = "/tmp/excel_fit_" + std::to_string(dist(rng));
        if (mkdir(tmpBase.c_str(), 0755) != 0) return false;
#endif
    }

    auto cleanup = [&]() {
#ifdef _WIN32
        (void)system(("rd /s /q \"" + tmpBase + "\"").c_str());
#else
        (void)system(("rm -rf \"" + tmpBase + "\"").c_str());
#endif
    };

#ifdef _WIN32
    int unzipR = system(("powershell -Command \"Expand-Archive -Path '" + xlsxPath + "' -DestinationPath '" + tmpBase + "'\"").c_str());
#else
    int unzipR = system(("unzip -q -o \"" + xlsxPath + "\" -d \"" + tmpBase + "\"").c_str());
#endif
    if (unzipR != 0) {
        cleanup();
        return false;
    }

    std::string sheetPath = tmpBase + "/xl/worksheets/sheet1.xml";
    std::string sheetXml = readFile(sheetPath);
    if (sheetXml.empty()) {
        cleanup();
        return false;
    }

    // Insert pageSetup right after pageMargins (OOXML order: pageMargins, pageSetup, ...)
    // Must not append at end: drawing/mergeCells would then precede pageSetup, causing Excel recovery error
    const std::string pageSetup = "\n  <pageSetup paperSize=\"9\" orientation=\"portrait\" fitToPage=\"1\" fitToWidth=\"1\" fitToHeight=\"1\"/>";
    const std::string marker = "<pageMargins ";
    auto start = sheetXml.find(marker);
    if (start == std::string::npos) {
        cleanup();
        return false;
    }
    // Find end of pageMargins element (self-closing <pageMargins .../>)
    auto end = sheetXml.find("/>", start);
    if (end == std::string::npos) {
        cleanup();
        return false;
    }
    end += 2;  // past ">"
    sheetXml.insert(end, pageSetup);

    if (!writeFile(sheetPath, sheetXml)) {
        cleanup();
        return false;
    }

    std::string outPath = xlsxPath + ".new";
#if defined(__APPLE__) || defined(__linux__)
    {
        size_t sep = xlsxPath.rfind('/');
        std::string dir = (sep != std::string::npos) ? xlsxPath.substr(0, sep + 1) : "";
        char resolved[PATH_MAX];
        if (realpath(dir.empty() ? "." : dir.c_str(), resolved)) {
            outPath = std::string(resolved) + "/" +
                      ((sep != std::string::npos) ? xlsxPath.substr(sep + 1) : xlsxPath) + ".new";
        }
    }
#endif
#ifdef _WIN32
    int zipR = zipWithContentTypesFirst(tmpBase, outPath) ? 0 : 1;
#else
    // Excel expects [Content_Types].xml first in zip; explicit order avoids recovery dialog
    int zipR = system(("cd \"" + tmpBase + "\" && zip -X -r -q \"" + outPath + "\" \"[Content_Types].xml\" _rels docProps xl").c_str());
#endif
    cleanup();
    if (zipR != 0) return false;

#ifdef _WIN32
    bool ok = copyFile(outPath, xlsxPath);
    if (ok) std::remove(outPath.c_str());
#else
    bool ok = (rename(outPath.c_str(), xlsxPath.c_str()) == 0);
    if (!ok) {
        ok = copyFile(outPath, xlsxPath);
        if (ok) unlink(outPath.c_str());
    }
#endif
    return ok;
}

} // namespace excel
