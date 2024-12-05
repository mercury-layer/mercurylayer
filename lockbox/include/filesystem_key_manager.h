#ifndef FILESYSTEM_KEY_MANAGER_H
#define FILESYSTEM_KEY_MANAGER_H

#include <string>
#include <vector>

namespace filesystem_key_manager {
    std::vector<uint8_t> get_seed();
} // namespace key_manager

#endif // FILESYSTEM_KEY_MANAGER_H