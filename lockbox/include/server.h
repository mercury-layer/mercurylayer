#ifndef SERVER_H
#define SERVER_H

#include <string>

namespace lockbox {
    void start_server(const std::string& key_provider);
} // namespace lockbox

#endif // SERVER_H