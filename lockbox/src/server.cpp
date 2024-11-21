#include "server.h"
#include <crow.h>
#include <openssl/rand.h>
#include "utils.h"
#include "enclave.h"
#include "key_manager.h"

namespace lockbox {
    void start_server() {

        std::vector<uint8_t> seed = key_manager::get_seed();

        std::string seed_hex = utils::key_to_string(seed.data(), seed.size());

        std::cout << "seed_hex: " << seed_hex << std::endl;

        enclave::generate_new_keypair(seed.data());

        // Initialize Crow HTTP server
        crow::SimpleApp app;

        // Define a simple route
        CROW_ROUTE(app, "/")([](){
            return "Hello, Crow!";
        });

        // Start the server on port 18080
        app.port(18080).multithreaded().run();
    }
} // namespace lockbox