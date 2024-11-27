#include "server.h"
#include <crow.h>
#include <openssl/rand.h>
#include "utils.h"
#include "enclave.h"
#include "key_manager.h"
#include "db_manager.h"

namespace lockbox {

    crow::response generate_new_keypair(const std::string& statechain_id,  unsigned char *seed) {
        auto new_key_pair_response = enclave::generate_new_keypair(seed);

        std::string error_message;
        bool data_saved = db_manager::save_generated_public_key(
            new_key_pair_response.encrypted_data, 
            new_key_pair_response.server_pubkey, 
            sizeof(new_key_pair_response.server_pubkey), 
            statechain_id, 
            error_message);

        if (!data_saved) {
            error_message = "Failed to save aggregated key data: " + error_message;
            return crow::response(500, error_message);
        }

        std::string server_pubkey_hex = utils::key_to_string(new_key_pair_response.server_pubkey, 33);

        crow::json::wvalue result({{"server_pubkey", server_pubkey_hex}});
        return crow::response{result};
    }

    crow::response generate_public_nonce(const std::string& statechain_id,  unsigned char *seed) {

        auto encrypted_keypair = std::make_unique<utils::chacha20_poly1305_encrypted_data>();

        // the secret nonce is not defined yet
        auto encrypted_secnonce = std::make_unique<utils::chacha20_poly1305_encrypted_data>();
        encrypted_secnonce.reset();

        std::string error_message;
        bool data_loaded = db_manager::load_generated_key_data(
            statechain_id,
            encrypted_keypair,
            encrypted_secnonce,
            nullptr,
            0,
            error_message
        );

        assert(encrypted_secnonce == nullptr);

        if (!data_loaded) {
            error_message = "Failed to load aggregated key data: " + error_message;
            return crow::response(500, error_message);
        }

        auto response = enclave::generate_nonce(seed, encrypted_keypair.get());

        bool data_saved = db_manager::update_sealed_secnonce(
            statechain_id,
            response.server_pubnonce, sizeof(response.server_pubnonce),
            response.encrypted_secnonce,
            error_message
        );

        if (!data_saved) {
            error_message = "Failed to save sealed secret nonce: " + error_message;
            return crow::response(500, error_message);
        }

        auto serialized_server_pubnonce_hex = utils::key_to_string(response.server_pubnonce, sizeof(response.server_pubnonce));

        crow::json::wvalue result({{"server_pubnonce", serialized_server_pubnonce_hex}});
        return crow::response{result};
    }

    crow::response generate_partial_signature(
        const std::string& statechain_id, 
        int64_t negate_seckey, 
        std::vector<unsigned char>& serialized_session,
        unsigned char *seed) {

            auto encrypted_keypair = std::make_unique<utils::chacha20_poly1305_encrypted_data>();
            auto encrypted_secnonce = std::make_unique<utils::chacha20_poly1305_encrypted_data>();

            unsigned char serialized_server_pubnonce[66];
            memset(serialized_server_pubnonce, 0, sizeof(serialized_server_pubnonce));

            std::string error_message;
            bool data_loaded = db_manager::load_generated_key_data(
                statechain_id,
                encrypted_keypair,
                encrypted_secnonce,
                serialized_server_pubnonce,
                sizeof(serialized_server_pubnonce),
                error_message
            );

            if (!data_loaded) {
                error_message = "Failed to load aggregated key data: " + error_message;
                return crow::response(500, error_message);
            }

            bool is_sealed_keypair_empty = encrypted_keypair == nullptr;
            bool is_sealed_secnonce_empty = encrypted_secnonce == nullptr;

            if (is_sealed_keypair_empty || is_sealed_secnonce_empty) {
                return crow::response(400, "Empty sealed keypair or sealed secnonce!");
            }

            auto response = enclave::partial_signature(
                seed, 
                encrypted_keypair.get(),
                encrypted_secnonce.get(),
                (int) negate_seckey,
                serialized_session.data(), serialized_session.size(),
                serialized_server_pubnonce);

            bool sig_count_updated = db_manager::update_sig_count(statechain_id);
            if (!sig_count_updated) {
                return crow::response(500, "Failed to update signature count!");
            }

            auto partial_sig_hex = utils::key_to_string(response.partial_sig_data, sizeof(response.partial_sig_data));

            crow::json::wvalue result({{"partial_sig", partial_sig_hex}});
            return crow::response{result};
    }

    void start_server() {

        std::vector<uint8_t> seed = key_manager::get_seed();

        std::string seed_hex = utils::key_to_string(seed.data(), seed.size());

        std::cout << "seed_hex: " << seed_hex << std::endl;

        // generate_new_keypair(seed.data());

        // Initialize Crow HTTP server
        crow::SimpleApp app;

        // Define a simple route
        CROW_ROUTE(app, "/")([](){
            return "Hello, Crow!";
        });

        CROW_ROUTE(app, "/get_public_key")
        .methods("POST"_method)([&seed](const crow::request& req) {

            auto req_body = crow::json::load(req.body);
            if (!req_body)
                return crow::response(400);

            if (req_body.count("statechain_id") == 0)
                return crow::response(400, "Invalid parameter. It must be 'statechain_id'.");

            std::string statechain_id = req_body["statechain_id"].s();

            return generate_new_keypair(statechain_id, seed.data());
        });

        CROW_ROUTE(app, "/get_public_nonce")
        .methods("POST"_method)([&seed](const crow::request& req) {

            auto req_body = crow::json::load(req.body);
            if (!req_body)
                return crow::response(400);

            if (req_body.count("statechain_id") == 0) {
                return crow::response(400, "Invalid parameters. They must be 'statechain_id'.");
            }

            std::string statechain_id = req_body["statechain_id"].s();

            return generate_public_nonce(statechain_id, seed.data());
        });

        CROW_ROUTE(app, "/get_partial_signature")
            .methods("POST"_method)([&seed](const crow::request& req) {

                auto req_body = crow::json::load(req.body);
                if (!req_body)
                    return crow::response(400);

                if (req_body.count("statechain_id") == 0 || 
                    req_body.count("negate_seckey") == 0 ||
                    req_body.count("session") == 0) {
                    return crow::response(400, "Invalid parameters. They must be 'statechain_id', 'negate_seckey' and 'session'.");
                }

                std::string statechain_id = req_body["statechain_id"].s();
                int64_t negate_seckey = req_body["negate_seckey"].i();
                std::string session_hex = req_body["session"].s();


                if (session_hex.substr(0, 2) == "0x") {
                    session_hex = session_hex.substr(2);
                }

                std::vector<unsigned char> serialized_session = utils::ParseHex(session_hex);

                if (serialized_session.size() != 133) {
                    return crow::response(400, "Invalid session length. Must be 133 bytes!");
                }

                return generate_partial_signature(statechain_id, negate_seckey, serialized_session, seed.data());
        
        });

        // Start the server on port 18080
        app.port(18080).multithreaded().run();
    }
} // namespace lockbox