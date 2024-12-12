#ifndef UTILS_H
#define UTILS_H

#include <cstddef>
#include <stdint.h>
#include <string_view>
#include <vector>
#include <iomanip>
#include <string>

namespace utils {

    // config vars
    struct config_var
    {
        std::string env_var;
        std::string toml_var_1;
        std::string toml_var_2;
    };

    static const config_var DATABASE_URL = {"LOCKBOX_DATABASE_URL", "general", "database_connection_string"};
    static const config_var SEED_FILEPATH = {"SEED_FILEPATH", "filesystem", "seed_filepath"};
    static const config_var KEY_MANAGER = {"KEY_MANAGER", "general", "key_manager"};
    static const config_var SERVER_PORT = {"LOCKBOX_PORT", "general", "server_port"};
    

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

    uint16_t getServerPort();

    std::string getStringConfigVar(const std::string& env_var, const std::string& toml_var_1, const std::string& toml_var_2);

} // namespace utils

#endif // UTILS_H