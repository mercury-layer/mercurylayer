#ifndef ENCLAVE_H
#define ENCLAVE_H

#include "utils.h"

namespace enclave {

    struct NewKeyPairResponse {
        unsigned char server_pubkey[33];
        utils::chacha20_poly1305_encrypted_data encrypted_data;
    };

    struct NewNonceResponse {
        unsigned char server_pubnonce[66];
        utils::chacha20_poly1305_encrypted_data encrypted_secnonce;
    };

    NewKeyPairResponse generate_new_keypair(unsigned char* seed);
    NewNonceResponse generate_nonce(unsigned char* seed, utils::chacha20_poly1305_encrypted_data *encrypted_keypair);
} // namespace enclave

#endif // ENCLAVE_H