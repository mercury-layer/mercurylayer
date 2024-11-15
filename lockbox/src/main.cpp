#include <crow.h>
#include "library.h"
#include "CLI11.hpp"
#include <toml++/toml.h>

#include "google/cloud/kms/v1/key_management_client.h"
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
    namespace secretmanager = ::google::cloud::secretmanager_v1;
    auto client = secretmanager::SecretManagerServiceClient(
        secretmanager::MakeSecretManagerServiceConnection());

    auto const parent = std::string("projects/") + project_id;
    for (auto secret : client.ListSecrets(parent)) {
        if (!secret) throw std::move(secret).status();
        std::cout << secret->DebugString() << "\n";
    }


}


int main(int argc, char *argv[]) {

    test_kms();

    test_secret();

    CLI::App cli_app{"Lockbox Server"};
    cli_app.set_version_flag("--version", std::string("0.0.1"));

    CLI11_PARSE(cli_app, argc, argv);

    // Initialize Crow HTTP server
    crow::SimpleApp app;

    // Define a simple route
    CROW_ROUTE(app, "/")([](){
        return "Hello, Crow!";
    });

    int result = add(3, 4);
    std::cout << "The result of add(3, 4) is: " << result << std::endl;

    auto config = toml::parse_file("../Settings.toml");
    std::string seed_dir = config["intel_sgx"]["seed_dir"].as_string()->get();
    std::cout << "seed_dir: " << seed_dir << std::endl;

    // Start the server on port 18080
    app.port(18080).multithreaded().run();

    return 0;
}