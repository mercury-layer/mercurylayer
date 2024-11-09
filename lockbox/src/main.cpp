#include "server.h"
#include "CLI11.hpp"
#include "utils.h"

int main(int argc, char *argv[]) {

    CLI::App cli_app{"Lockbox Server"};
    cli_app.set_version_flag("--version", std::string("0.0.1"));

    CLI11_PARSE(cli_app, argc, argv);

    lockbox::start_server();

    return 0;
}