#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include <string>
#include <vector>

namespace key_manager {
    std::string get_encrypted_secret();
    std::string decrypt_secret(std::string const& encrypted_secret);
    std::vector<uint8_t> get_seed();
} // namespace key_manager

#endif // KEY_MANAGER_H