#include "google_key_manager.h"

#include "google/cloud/kms/v1/key_management_client.h"
#include "google/cloud/secretmanager/v1/secret_manager_client.h"
#include "google/cloud/location.h"
#include <iostream>
#include "utils.h"
#include <toml++/toml.h>

namespace key_manager {

    std::string get_encrypted_secret() {

        auto config = toml::parse_file("Settings.toml");

        auto project_id = config["gcloud"]["project_id"].as_string()->get();
        auto project_number = config["gcloud"]["project_number"].as_string()->get();
        auto key_name = config["secretmanager"]["key_name"].as_string()->get();
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

        auto config = toml::parse_file("Settings.toml");

        auto location_id = config["gcloud"]["location_id"].as_string()->get();
        auto project_id = config["gcloud"]["project_id"].as_string()->get();
        auto key_ring = config["kms"]["ring"].as_string()->get();
        auto crypto_key = config["kms"]["crypto_key"].as_string()->get();

        namespace kms = ::google::cloud::kms_v1;
        auto client = kms::KeyManagementServiceClient(kms::MakeKeyManagementServiceConnection());

        auto const location = google::cloud::Location(project_id, location_id);
        auto crypto_key_name =  location.FullName() + "/keyRings/" + key_ring + "/cryptoKeys/" + crypto_key;
        auto decrypt_response = client.Decrypt(crypto_key_name, encrypted_secret);
        if (!decrypt_response) throw std::move(decrypt_response).status();
        auto plaintext = decrypt_response->plaintext();
        return plaintext;
    }

    std::vector<uint8_t> get_seed() {
        auto encrypted_secret = key_manager::get_encrypted_secret();
        std::string decrypted_secret;
        try {
            decrypted_secret = key_manager::decrypt_secret(encrypted_secret);
        } catch (google::cloud::Status const& status) {
            std::cerr << "google::cloud::Status thrown: " << status << "\n";
        }

        std::vector<unsigned char> serialized_secret = utils::ParseHex(decrypted_secret);
        return serialized_secret;
    }

} // namespace key_manager