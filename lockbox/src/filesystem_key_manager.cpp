#include "hashicorp_key_manager.h"

#include <vector>
#include <stdexcept>
#include <openssl/rand.h>
#include <filesystem>
#include <fstream>
#include "utils.h"
#include <toml++/toml.h>

namespace filesystem_key_manager {

    std::string getSeedFilePath() {
        return utils::getStringConfigVar(
            utils::SEED_FILEPATH.env_var, utils::SEED_FILEPATH.toml_var_1, utils::SEED_FILEPATH.toml_var_2);
    }

    std::vector<uint8_t> get_seed() {
        const std::string seed_file = getSeedFilePath();

        if (std::filesystem::exists(seed_file)) {
            // The seed file exists
            std::ifstream seed_in(seed_file, std::ios::binary);
            if (!seed_in) {
                throw std::runtime_error("Error opening seed file for reading.");
            }

            // Read the contents into a vector
            std::vector<uint8_t> key((std::istreambuf_iterator<char>(seed_in)),
                                    std::istreambuf_iterator<char>());

            // Check if the key is 32 bytes
            if (key.size() != 32) {
                throw std::runtime_error("Seed file has invalid size.");
            }

            return key;
        } else {
            // Seed file does not exist, generate a new seed
            std::vector<uint8_t> key(32); // 256-bit key

            // Generate cryptographically secure random bytes
            if (RAND_bytes(key.data(), key.size()) != 1) {
                throw std::runtime_error("Error generating random bytes.");
            }

            // Write the key to the seed file
            std::ofstream seed_out(seed_file, std::ios::binary | std::ios::trunc);
            if (!seed_out) {
                throw std::runtime_error("Error opening seed file for writing.");
            }

            seed_out.write(reinterpret_cast<const char*>(key.data()), key.size());
            if (!seed_out) {
                throw std::runtime_error("Error writing seed to file.");
            }

            // Optionally, set file permissions to owner read/write only (platform-dependent)
#ifdef __unix__
            seed_out.close(); // Close the file before changing permissions
            std::filesystem::permissions(seed_file,
                std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                std::filesystem::perm_options::replace);
#endif

            return key;
        }
    }

}