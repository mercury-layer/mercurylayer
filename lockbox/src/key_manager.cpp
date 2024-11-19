#include "key_manager.h"

#include "google/cloud/kms/v1/key_management_client.h"
#include "google/cloud/secretmanager/v1/secret_manager_client.h"
#include "google/cloud/location.h"
#include <iostream>

namespace key_manager {
    
    std::string get_encrypted_secret() {
        auto project_id = "mercury-441416";
        auto project_number = "100600525477";
        auto key_name = "encrypted-key";
        auto version = 1;

        namespace secretmanager = ::google::cloud::secretmanager_v1;
        auto client = secretmanager::SecretManagerServiceClient(
            secretmanager::MakeSecretManagerServiceConnection());

        auto const parent = std::string("projects/") + project_number;
        auto secret_name = parent + "/secrets/" + key_name;
        auto secret_version_name = secret_name + "/versions/" + std::to_string(version);

        auto secret_version = client.AccessSecretVersion(secret_version_name);
        if (!secret_version) throw std::move(secret_version).status();

        return secret_version->payload().data();
    }

    std::string decrypt_secret(std::string const& encrypted_secret) {
        auto location_id = "global";
        auto project_id = "mercury-441416";
        auto key_ring = "enclave";
        auto crypto_key = "sealing";

        namespace kms = ::google::cloud::kms_v1;
        auto client = kms::KeyManagementServiceClient(kms::MakeKeyManagementServiceConnection());

        auto const location = google::cloud::Location(project_id, location_id);
        auto crypto_key_name =  location.FullName() + "/keyRings/" + key_ring + "/cryptoKeys/" + crypto_key;
        auto decrypt_response = client.Decrypt(crypto_key_name, encrypted_secret);
        if (!decrypt_response) throw std::move(decrypt_response).status();
        auto plaintext = decrypt_response->plaintext();
        return plaintext;
    }

} // namespace key_manager