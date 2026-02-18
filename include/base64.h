#ifndef BASE64_H
#define BASE64_H

#include <string>
#include <vector>

namespace base64 {

// Encode binary data to base64 string
std::string encode(const unsigned char* data, size_t len);

// Encode std::vector<unsigned char> to base64
inline std::string encode(const std::vector<unsigned char>& data) {
    return encode(data.empty() ? nullptr : data.data(), data.size());
}

// Decode base64 string to binary; returns empty vector on error
std::vector<unsigned char> decode(const std::string& encoded);

} // namespace base64

#endif // BASE64_H
