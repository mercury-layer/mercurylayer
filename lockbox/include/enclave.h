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

    struct PatialSignatureResponse {
        unsigned char partial_sig_data[32];
    };

    NewKeyPairResponse generate_new_keypair(unsigned char* seed);
    NewNonceResponse generate_nonce(unsigned char* seed, utils::chacha20_poly1305_encrypted_data *encrypted_keypair);
    PatialSignatureResponse partial_signature(
        unsigned char* seed, 
        utils::chacha20_poly1305_encrypted_data *encrypted_keypair, 
        utils::chacha20_poly1305_encrypted_data *encrypted_secnonce,
        int negate_seckey,
        unsigned char* session_data, 
        size_t session_data_size,
        unsigned char* serialized_server_pubnonce);
    NewKeyPairResponse key_update(
        unsigned char* seed, 
        utils::chacha20_poly1305_encrypted_data *old_encrypted_keypair,
        unsigned char* serialized_x1,
        unsigned char* serialized_t2);
    

} // namespace enclave

#endif // ENCLAVE_H