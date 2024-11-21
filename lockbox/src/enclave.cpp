#include <assert.h>
#include "enclave.h"
#include <openssl/rand.h>
#include <stdexcept>
#include <string.h>
#include "utils.h"
#include <iostream>
#include "secp256k1.h"
#include "secp256k1_schnorrsig.h"
#include "secp256k1_musig.h"
#include "monocypher.h"

void encrypt_data(
    utils::chacha20_poly1305_encrypted_data *encrypted_data,
    unsigned char* seed, 
    uint8_t* raw_data, size_t raw_data_size)
{
    // Associated data (optional, can be NULL if not used)
    uint8_t *ad = NULL;
    size_t ad_size = 0;

    if (RAND_bytes(encrypted_data->nonce, sizeof(encrypted_data->nonce)) != 1) {
        throw std::runtime_error("Failed to generate random bytes");
    }

    encrypted_data->data_len = raw_data_size;
    crypto_aead_lock(encrypted_data->data, encrypted_data->mac, seed, encrypted_data->nonce, ad, ad_size, raw_data, raw_data_size);

    /* char* seed_hex = data_to_hex(seed, sizeof(seed));
    ocall_print_string("seed:");
    ocall_print_string(seed_hex);

    char* mac_hex = data_to_hex(encrypted_data->mac, sizeof(encrypted_data->mac));
    ocall_print_string("mac:");
    ocall_print_string(mac_hex);

    char* nonce_hex = data_to_hex(encrypted_data->nonce, sizeof(encrypted_data->nonce));
    ocall_print_string("nonce:");
    ocall_print_string(nonce_hex);

    char* encrypted_hex = data_to_hex(encrypted_data->data, encrypted_data->data_len);
    ocall_print_string("encrypted:");
    ocall_print_string(encrypted_hex); */

}
namespace enclave {

    void generate_new_keypair(
        unsigned char* seed
    ) {

        secp256k1_context* ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

        unsigned char server_privkey[32];
        memset(server_privkey, 0, 32);

        do {
            if (RAND_bytes(server_privkey, sizeof(server_privkey)) != 1) {
                throw std::runtime_error("Failed to generate random bytes");
            }
        } while (!secp256k1_ec_seckey_verify(ctx, server_privkey));

        secp256k1_keypair server_keypair;

        int return_val = secp256k1_keypair_create(ctx, &server_keypair, server_privkey);
        assert(return_val);

        secp256k1_pubkey server_pubkey;
        return_val = secp256k1_keypair_pub(ctx, &server_pubkey, &server_keypair);
        assert(return_val);

        // remove

        std::string server_privkey_hex = utils::key_to_string(server_privkey, 32);
        std::cout << "server_privkey_hex: " << server_privkey_hex << std::endl;

        unsigned char local_compressed_server_pubkey[33];
        memset(local_compressed_server_pubkey, 0, 33);

        size_t len = sizeof(local_compressed_server_pubkey);
        return_val = secp256k1_ec_pubkey_serialize(ctx, local_compressed_server_pubkey, &len, &server_pubkey, SECP256K1_EC_COMPRESSED);
        assert(return_val);
        // Should be the same size as the size of the output, because we passed a 33 byte array.
        assert(len == sizeof(local_compressed_server_pubkey));

        std::string server_pubkey_hex = utils::key_to_string(local_compressed_server_pubkey, 33);
        std::cout << "server_pubkey_hex: " << server_pubkey_hex << std::endl;

        // --- remove

        utils::chacha20_poly1305_encrypted_data encrypted_data;
        utils::initialize_encrypted_data(encrypted_data, sizeof(secp256k1_keypair));

        encrypt_data(&encrypted_data, seed, server_keypair.data, sizeof(secp256k1_keypair::data));

        secp256k1_context_destroy(ctx);
        
    }

} // namespace enclave