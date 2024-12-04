#include "hashicorp_key_manager.h"

#include <crow.h>
#include <cpr/cpr.h>
#include <iostream>
#include "utils.h"
#include <toml++/toml.h>

namespace hashicorp_key_manager {

    std::string get_access_token() {

        auto config = toml::parse_file("Settings.toml");

        const std::string client_id = config["hashicorp"]["hcp_client_id"].as_string()->get();
        const std::string client_secret = config["hashicorp"]["hcp_client_secret"].as_string()->get();

        // Make the HTTP POST request using cpr
        cpr::Response response = cpr::Post(
            cpr::Url{"https://auth.idp.hashicorp.com/oauth2/token"},
            cpr::Header{{"Content-Type", "application/x-www-form-urlencoded"}},
            cpr::Payload{
                {"client_id", client_id},
                {"client_secret", client_secret},
                {"grant_type", "client_credentials"},
                {"audience", "https://api.hashicorp.cloud"}}
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

        // Extract the access token
        if (!json.has("access_token")) {
            throw std::runtime_error("Response JSON does not contain 'access_token'. Response: " + response.text);
        }

        auto access_token = json["access_token"].s();

        return access_token;
    }

    std::string get_secret(const std::string& hcp_api_token) {

        auto config = toml::parse_file("Settings.toml");

        const std::string organization_id = config["hashicorp"]["organization_id"].as_string()->get();
        const std::string project_id = config["hashicorp"]["project_id"].as_string()->get();
        const std::string app_name = config["hashicorp"]["app_name"].as_string()->get();
        const std::string secret_name = config["hashicorp"]["secret_name"].as_string()->get();

        // Construct the URL
        std::string url = "https://api.cloud.hashicorp.com/secrets/2023-11-28/organizations/" + organization_id +
                        "/projects/" + project_id +
                        "/apps/" + app_name +
                        "/secrets/" + secret_name + ":open";

        // Make the HTTP GET request using CPR
        cpr::Response response = cpr::Get(
            cpr::Url{url},
            cpr::Header{{"Authorization", "Bearer " + hcp_api_token}}
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

        return json["secret"]["static_version"]["value"].s();
    }

    std::vector<uint8_t> get_seed() {

        auto access_token = get_access_token();

        auto secret = get_secret(access_token);

        std::vector<unsigned char> serialized_secret = utils::ParseHex(secret);
        
        return serialized_secret;
    }

}