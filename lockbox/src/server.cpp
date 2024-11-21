#include "server.h"
#include <crow.h>
#include <openssl/rand.h>
#include "utils.h"
#include "enclave.h"

namespace lockbox {
    void start_server() {

        enclave::generate_new_keypair();

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