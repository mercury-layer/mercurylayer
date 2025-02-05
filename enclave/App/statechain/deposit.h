#pragma once

#ifndef STATECHAIN_DEPOSIT_H
#define STATECHAIN_DEPOSIT_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#include <lib/crow_all.h>
#pragma GCC diagnostic pop

#include "../Enclave_u.h"
#include "../sealing_key_manager/sealing_key_manager.h"

namespace deposit {
    crow::response get_public_key(sgx_enclave_id_t& enclave_id, const std::string& statechain_id, sealing_key_manager::SealingKeyManager& sealing_key_manager);
}

#endif // STATECHAIN_DEPOSIT_H