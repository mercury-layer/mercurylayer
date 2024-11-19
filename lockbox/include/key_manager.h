#ifndef KEY_MANAGER_H
#define KEY_MANAGER_H

#include <string>

namespace key_manager {
    std::string get_encrypted_secret();
    std::string decrypt_secret(std::string const& encrypted_secret);
} // namespace key_manager

#endif // KEY_MANAGER_H