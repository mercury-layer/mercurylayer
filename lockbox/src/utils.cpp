#include "utils.h"
#include <string.h>

namespace utils {

    const signed char p_util_hexdigit[256] =
    { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,
    -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,0xa,0xb,0xc,0xd,0xe,0xf,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, };

    signed char HexDigit(char c)
    {
        return p_util_hexdigit[(unsigned char)c];
    }

    constexpr inline bool IsSpace(char c) noexcept {
        return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
    }

    template <typename Byte>
    std::vector<Byte> ParseHex(std::string_view str)
    {
        std::vector<Byte> vch;
        auto it = str.begin();
        while (it != str.end() && it + 1 != str.end()) {
            if (IsSpace(*it)) {
                ++it;
                continue;
            }
            auto c1 = HexDigit(*(it++));
            auto c2 = HexDigit(*(it++));
            if (c1 < 0 || c2 < 0) break;
            vch.push_back(Byte(c1 << 4) | Byte(c2));
        }
        return vch;
    }

    template std::vector<std::byte> ParseHex(std::string_view);
    template std::vector<uint8_t> ParseHex(std::string_view);

    std::string key_to_string(const unsigned char* key, size_t keylen) {
        std::stringstream sb;
        sb << "0x";
        for (int i = 0; i < keylen; i++)
            sb << std::hex << std::setw(2) << std::setfill('0') << (int)key[i];
        return sb.str();
    }

    void initialize_encrypted_data(chacha20_poly1305_encrypted_data& encrypted_data, size_t data_len) {

        // initialize encrypted_data
        encrypted_data.data_len = data_len;
        encrypted_data.data = new unsigned char[encrypted_data.data_len];
        memset(encrypted_data.data, 0, encrypted_data.data_len);

        memset(encrypted_data.mac, 0, sizeof(encrypted_data.mac));
        memset(encrypted_data.nonce, 0, sizeof(encrypted_data.nonce));
    }

}
