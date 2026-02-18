#include "../include/base64.h"
#include <stdexcept>

namespace base64 {

static const char kChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string encode(const unsigned char* data, size_t len) {
    if (!data && len > 0) return "";
    std::string result;
    result.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        unsigned int n = static_cast<unsigned int>(data[i]) << 16;
        if (i + 1 < len) n |= static_cast<unsigned int>(data[i + 1]) << 8;
        if (i + 2 < len) n |= static_cast<unsigned int>(data[i + 2]);
        result += kChars[(n >> 18) & 63];
        result += kChars[(n >> 12) & 63];
        result += (i + 1 < len) ? kChars[(n >> 6) & 63] : '=';
        result += (i + 2 < len) ? kChars[n & 63] : '=';
    }
    return result;
}

static int decodeChar(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

std::vector<unsigned char> decode(const std::string& encoded) {
    std::vector<unsigned char> result;
    if (encoded.empty()) return result;
    size_t len = encoded.size();
    if (len % 4 != 0) return result;  // Invalid base64
    size_t outLen = (len / 4) * 3;
    if (len >= 1 && encoded[len - 1] == '=') { outLen--; }
    if (len >= 2 && encoded[len - 2] == '=') { outLen--; }
    result.reserve(outLen);
    for (size_t i = 0; i + 4 <= len; i += 4) {
        int a = decodeChar(encoded[i]);
        int b = decodeChar(encoded[i + 1]);
        int c = decodeChar(encoded[i + 2]);
        int d = decodeChar(encoded[i + 3]);
        if (a < 0 || b < 0 || c < 0 || d < 0) return {};
        unsigned int n = (static_cast<unsigned int>(a) << 18) | (static_cast<unsigned int>(b) << 12) |
                        (static_cast<unsigned int>(c) << 6) | static_cast<unsigned int>(d);
        result.push_back(static_cast<unsigned char>((n >> 16) & 255));
        if (encoded[i + 2] != '=') result.push_back(static_cast<unsigned char>((n >> 8) & 255));
        if (encoded[i + 3] != '=') result.push_back(static_cast<unsigned char>(n & 255));
    }
    return result;
}

} // namespace base64
