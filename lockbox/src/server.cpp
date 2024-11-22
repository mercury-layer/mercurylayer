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

        // Start the server on port 18080
        app.port(18080).multithreaded().run();
    }
} // namespace lockbox