#ifndef UTILS_H
#define UTILS_H

#include <cstddef>
#include <stdint.h>
#include <string_view>
#include <vector>
#include <iomanip>
#include <string>

namespace utils {

    struct chacha20_poly1305_encrypted_data {
        size_t data_len;
        unsigned char* data;
        unsigned char nonce[24];
        unsigned char mac[16];
    };

    /** Parse the hex string into bytes (uint8_t or std::byte). Ignores whitespace. */
    template <typename Byte = uint8_t>
    std::vector<Byte> ParseHex(std::string_view str);
    
    std::string key_to_string(const unsigned char* key, size_t keylen);

    void initialize_encrypted_data(chacha20_poly1305_encrypted_data& encrypted_data, size_t data_len);

} // namespace utils

#endif // UTILS_H