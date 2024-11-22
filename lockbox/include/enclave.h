#ifndef ENCLAVE_H
#define ENCLAVE_H

#include "utils.h"

namespace enclave {

    struct NewKeyPairResponse {
        unsigned char server_pubkey[33];
        utils::chacha20_poly1305_encrypted_data encrypted_data;
    };

    NewKeyPairResponse generate_new_keypair(unsigned char* seed);
} // namespace enclave

#endif // ENCLAVE_H