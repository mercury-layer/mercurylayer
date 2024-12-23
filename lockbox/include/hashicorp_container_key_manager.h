#ifndef HASHICORP_CONTAINER_KEY_MANAGER_H
#define HASHICORP_CONTAINER_KEY_MANAGER_H

#include <string>
#include <vector>

namespace hashicorp_container_key_manager {
    std::vector<uint8_t> get_seed();
} // namespace key_manager

#endif // HASHICORP_CONTAINER_KEY_MANAGER_H