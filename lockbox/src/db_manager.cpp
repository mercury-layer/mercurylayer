#include "db_manager.h"

#include <iostream>
#include <memory>
#include <pqxx/pqxx>
#include <string>
#include <toml++/toml.h>
#include "utils.h"

namespace db_manager {

    std::string getDatabaseConnectionString() {
        const char* value = std::getenv("ENCLAVE_DATABASE_URL");

        if (value == nullptr) {
            auto config = toml::parse_file("Settings.toml");
            return config["intel_sgx"]["database_connection_string"].as_string()->get();
        } else {
            return std::string(value);        
        }
    }

    // Assumes the buffer is large enough. In a real application, ensure buffer safety.
    void serialize(const utils::chacha20_poly1305_encrypted_data* src, unsigned char* buffer, size_t* serialized_len) {
        // Copy `data_len`, `nonce`, and `mac` directly
        size_t offset = 0;
        memcpy(buffer + offset, &src->data_len, sizeof(src->data_len));
        offset += sizeof(src->data_len);

        memcpy(buffer + offset, src->nonce, sizeof(src->nonce));
        offset += sizeof(src->nonce);

        memcpy(buffer + offset, src->mac, sizeof(src->mac));
        offset += sizeof(src->mac);

        // Now copy dynamic `data`
        memcpy(buffer + offset, src->data, src->data_len);
        offset += src->data_len;

        *serialized_len = offset;
    }

    // Returns a newly allocated structure that must be freed by the caller.
    bool deserialize(const unsigned char* buffer, utils::chacha20_poly1305_encrypted_data* dest) {

        if (!dest) return false;

        size_t offset = 0;
        memcpy(&dest->data_len, buffer + offset, sizeof(dest->data_len));
        offset += sizeof(dest->data_len);

        memcpy(dest->nonce, buffer + offset, sizeof(dest->nonce));
        offset += sizeof(dest->nonce);

        memcpy(dest->mac, buffer + offset, sizeof(dest->mac));
        offset += sizeof(dest->mac);

        dest->data = new unsigned char[dest->data_len];
        if (!dest->data) {
            return false; // NULL;
        }
        memcpy(dest->data, buffer + offset, dest->data_len);

        return true;
    }

    bool save_generated_public_key(
        const utils::chacha20_poly1305_encrypted_data& encrypted_keypair, 
        unsigned char* server_public_key, size_t server_public_key_size,
        const std::string& statechain_id,
        std::string& error_message) {

        auto database_connection_string = getDatabaseConnectionString();

        try
        {
            pqxx::connection conn(database_connection_string);
            if (conn.is_open()) {

                std::string create_table_query =
                    "CREATE TABLE IF NOT EXISTS generated_public_key ( "
                    "id SERIAL PRIMARY KEY, "
                    "statechain_id varchar(50), "
                    "sealed_keypair BYTEA, "
                    "sealed_secnonce BYTEA, "
                    "public_nonce BYTEA, "
                    "public_key BYTEA UNIQUE, "
                    "sig_count INTEGER DEFAULT 0);";

                pqxx::work txn(conn);
                txn.exec(create_table_query);
                txn.commit();


                size_t serialized_len = 0;

                size_t bufferSize = sizeof(encrypted_keypair.data_len) + sizeof(encrypted_keypair.nonce) + sizeof(encrypted_keypair.mac) + encrypted_keypair.data_len;
                unsigned char* buffer = (unsigned char*) malloc(bufferSize);

                if (!buffer) {
                    error_message = "Failed to allocate memory for serialization!";
                    return false;
                }

                serialize(&encrypted_keypair, buffer, &serialized_len);
                assert(serialized_len == bufferSize);

                std::basic_string_view<std::byte> sealed_data_view(reinterpret_cast<std::byte*>(buffer), bufferSize);
                std::basic_string_view<std::byte> public_key_data_view(reinterpret_cast<std::byte*>(server_public_key), server_public_key_size);

                std::string insert_query =
                    "INSERT INTO generated_public_key (sealed_keypair, public_key, statechain_id) VALUES ($1, $2, $3);";
                pqxx::work txn2(conn);

                txn2.exec_params(insert_query, sealed_data_view, public_key_data_view, statechain_id);
                txn2.commit();

                conn.close();
                return true;

                return true;
            } else {
                error_message = "Failed to connect to the database!";
                return false;
            }
        }
        catch (std::exception const &e)
        {
            error_message = e.what();
            return false;
        }

        return true;
    }
} // namespace db_manager