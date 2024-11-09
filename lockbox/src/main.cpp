#include <crow.h>
#include "library.h"
#include "CLI11.hpp"
#include <toml++/toml.h>

int main(int argc, char *argv[]) {

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