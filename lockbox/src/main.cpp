#include "server.h"
#include "CLI11.hpp"
#include "utils.h"

int main(int argc, char *argv[]) {

    CLI::App cli_app{"Lockbox Server"};
    cli_app.set_version_flag("--version", std::string("0.0.1"));

    // Add the mandatory key_provider option
    std::string key_provider;
    cli_app.add_option("--key_provider", key_provider, "Key provider (google_kms or hashicorp)")
        ->required()  // Mark it as mandatory
        ->check(CLI::IsMember({"google_kms", "hashicorp"}));  // Validate allowed values

    try {
        CLI11_PARSE(cli_app, argc, argv);
    } catch (const CLI::ParseError &e) {
        return cli_app.exit(e); // Exit gracefully on parse errors
    }

    lockbox::start_server(key_provider);

    return 0;
}