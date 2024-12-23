#include "hashicorp_container_key_manager.h"

#include <crow.h>
#include <cpr/cpr.h>
#include <iostream>
#include "utils.h"

namespace hashicorp_container_key_manager {

    std::string get_secret() {

        const std::string token = utils::getStringConfigVar(utils::HASHICORP_CONTAINER_TOKEN);
        const std::string container_url = utils::getStringConfigVar(utils::HASHICORP_CONTAINER_URL);
        const std::string container_path = utils::getStringConfigVar(utils::HASHICORP_CONTAINER_PATH);
        const std::string mount_point = utils::getStringConfigVar(utils::HASHICORP_CONTAINER_MOUNT_POINT);
        const std::string key_name = utils::getStringConfigVar(utils::HASHICORP_CONTAINER_KEY_NAME);

        // Construct the URL
        std::string url = container_url + "/v1/" + mount_point + "/data/" + container_path;

        // Make the HTTP GET request using cpr
        cpr::Response response = cpr::Get(
            cpr::Url{url},
            cpr::Header{{"X-Vault-Token", token}}
        );

        // Check for HTTP request success
        if (response.status_code != 200) {
            throw std::runtime_error("HTTP request failed with status code: " +
                                    std::to_string(response.status_code) + "\nResponse: " + response.text);
        }

        // Parse the JSON response using CrowCPP
        crow::json::rvalue json = crow::json::load(response.text);
        if (!json) {
            throw std::runtime_error("Failed to parse JSON response: " + response.text);
        }
        
        return json["data"]["data"][key_name].s();
    }

    std::vector<uint8_t> get_seed() {
        auto secret = get_secret();

        std::vector<unsigned char> serialized_secret = utils::ParseHex(secret);
        
        return serialized_secret;
    }
}