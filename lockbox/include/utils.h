#ifndef UTILS_H
#define UTILS_H

#include <cstddef>
#include <stdint.h>
#include <string_view>
#include <vector>
#include <iomanip>
#include <string>

namespace utils {

    /** Parse the hex string into bytes (uint8_t or std::byte). Ignores whitespace. */
    template <typename Byte = uint8_t>
    std::vector<Byte> ParseHex(std::string_view str);
    
    std::string key_to_string(const unsigned char* key, size_t keylen);

} // namespace utils

#endif // UTILS_H