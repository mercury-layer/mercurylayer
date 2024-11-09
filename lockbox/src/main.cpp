#include <crow.h>
#include "library.h"

int main() {
    // Initialize Crow HTTP server
    crow::SimpleApp app;

    // Define a simple route
    CROW_ROUTE(app, "/")([](){
        return "Hello, Crow!";
    });

    int result = add(3, 4);
    std::cout << "The result of add(3, 4) is: " << result << std::endl;

    // Start the server on port 18080
    app.port(18080).multithreaded().run();

    return 0;
}