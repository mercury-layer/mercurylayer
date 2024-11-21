#include "enclave.h"
#include <openssl/rand.h>
#include <stdexcept>

namespace enclave {

    void generate_new_keypair() {
        unsigned char key[32];
        if (RAND_bytes(key, sizeof(key)) != 1) {
            throw std::runtime_error("Failed to generate random bytes");
        }
    }

} // namespace enclave