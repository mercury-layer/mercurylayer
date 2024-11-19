#include <crow.h>
#include "library.h"
#include "key_manager.h"
#include "server.h"
#include "CLI11.hpp"
#include <toml++/toml.h>
#include "utils.h"

/* #include "google/cloud/kms/v1/key_management_client.h"
#include "google/cloud/secretmanager/v1/secret_manager_client.h"
#include "google/cloud/location.h"
#include <iostream>

void test_kms() {
    try {
        auto location_id = "global";
        auto project_id = "mercury-441416";

        auto const location = google::cloud::Location(project_id, location_id);
        std::cout << "Location: " << location.FullName() << std::endl;

        namespace kms = ::google::cloud::kms_v1;
        auto client = kms::KeyManagementServiceClient(kms::MakeKeyManagementServiceConnection());

        // for (auto kr : client.ListKeyRings(location.FullName())) {
        //     if (!kr) throw std::move(kr).status();
        //     std::cout << kr->DebugString() << "\n";
        // }

        auto key_ring_name =  location.FullName() + "/keyRings/enclave";
        auto kr = client.GetKeyRing(key_ring_name);
        std::cout << kr->DebugString() << "\n";

        std::cout << "kr name: " << kr->name() << "\n";

        auto crypto_key_name = kr->name() + "/cryptoKeys/sealing";
        std::cout << "ck crypto_key_name: " << crypto_key_name << "\n";

        auto ck = client.GetCryptoKey(crypto_key_name);

        std::cout << "ck name: " << ck->name() << "\n";

        auto encrypt_response = client.Encrypt(ck->name(), "Hello, World!");
        std::cout << "encrypt_response: " << encrypt_response->ciphertext() << "\n";

        auto decrypt_response = client.Decrypt(ck->name(), encrypt_response->ciphertext());
        std::cout << "decrypt_response: " << decrypt_response->plaintext() << "\n";

    } catch (google::cloud::Status const& status) {
        std::cerr << "google::cloud::Status thrown: " << status << "\n";
    }
}

void test_secret() {

    auto project_id = "mercury-441416";
    auto project_number = "100600525477";
    auto key_name = "key";
    auto version = 1;

    namespace secretmanager = ::google::cloud::secretmanager_v1;
    auto client = secretmanager::SecretManagerServiceClient(
        secretmanager::MakeSecretManagerServiceConnection());

    // auto const parent = std::string("projects/") + project_id;
    // for (auto secret : client.ListSecrets(parent)) {
    //     if (!secret) throw std::move(secret).status();
    //     std::cout << secret->DebugString() << "\n";
    // }

    auto const parent = std::string("projects/") + project_number;
    auto secret_name = parent + "/secrets/" + key_name;
    auto secret = client.GetSecret(secret_name);
    if (!secret) throw std::move(secret).status();
    std::cout << secret->DebugString() << "\n";

    auto secret_version_name = secret_name + "/versions/" + std::to_string(version);
    auto secret_version = client.AccessSecretVersion(secret_version_name);
    if (!secret_version) throw std::move(secret_version).status();
    // std::cout << secret_version->DebugString() << "\n";
    std::cout << "secret_version name: " << secret_version->name() << "\n";
    std::cout << "secret_version payload: " << secret_version->payload().data() << "\n";
}

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
} */

int main(int argc, char *argv[]) {

    /* test_kms();

    test_secret(); */

    std::vector<uint8_t> sealing_secret = key_manager::get_sealing_secret();

    std::string sealing_secret_hex = utils::key_to_string(sealing_secret.data(), sealing_secret.size());

    std::cout << "sealing_secret_hex: " << sealing_secret_hex << std::endl;

    CLI::App cli_app{"Lockbox Server"};
    cli_app.set_version_flag("--version", std::string("0.0.1"));

    CLI11_PARSE(cli_app, argc, argv);

    int result = add(3, 4);
    std::cout << "The result of add(3, 4) is: " << result << std::endl;

    auto config = toml::parse_file("../Settings.toml");
    std::string seed_dir = config["intel_sgx"]["seed_dir"].as_string()->get();
    std::cout << "seed_dir: " << seed_dir << std::endl;

    lockbox::start_server();

    return 0;
}