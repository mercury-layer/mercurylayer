#pragma once

#ifndef ENDPOINT_SECRET_H
#define ENDPOINT_SECRET_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#include <lib/crow_all.h>
#pragma GCC diagnostic pop

#include "../sealing_key_manager/sealing_key_manager.h"

namespace endpointSecret {
    crow::response handleAddMnemonic(const crow::request& req, sgx_enclave_id_t& enclave_id, std::mutex& mutex_enclave_id, sealing_key_manager::SealingKeyManager& sealing_key_manager);
    crow::response handleAddSecret(const crow::request& req, sgx_enclave_id_t& enclave_id, std::mutex& mutex_enclave_id, sealing_key_manager::SealingKeyManager& sealing_key_manager);
    crow::response getEphemeralPublicKey(sealing_key_manager::SealingKeyManager& sealing_key_manager);
} // namespace endpointSecret

#endif // ENDPOINT_SECRET_H