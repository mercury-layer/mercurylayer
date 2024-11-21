#include "server.h"
#include <crow.h>
#include <openssl/rand.h>
#include "utils.h"

namespace lockbox {
    void start_server() {

        unsigned char key[32];
        if (RAND_bytes(key, sizeof(key)) != 1) {
            std::cout << "Failed to generate random bytes" << std::endl;
        } else {
            std::cout << "Generated random bytes" << std::endl;
            std::string random_bytes_hex = utils::key_to_string(key, 32);
            std::cout << "random_bytes_hex: " << random_bytes_hex << std::endl;
        }

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