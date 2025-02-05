#ifndef HASHICORP_API_KEY_MANAGER_H
#define HASHICORP_API_KEY_MANAGER_H

#include <string>
#include <vector>

namespace hashicorp_api_key_manager {
    std::vector<uint8_t> get_seed();
} // namespace key_manager

#endif // HASHICORP_API_KEY_MANAGER_H