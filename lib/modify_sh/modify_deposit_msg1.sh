#!/bin/bash

# Directory where Kotlin files are located
SEARCH_DIR="out-kotlin/com/mercurylayer"

# File pattern to search for (assuming all Kotlin files end with .kt)
FILE_PATTERN="*.kt"

# awk script to add @SerialName within data class Token
add_serial_name() {
    local file_path="$1"  # Path to the file being processed

    awk '
    BEGIN { in_data_class = 0 }

    /data class DepositMsg1\(/ { in_data_class = 1 }
    /^}/ { if (in_data_class) in_data_class = 0 }

    in_data_class && /var `authKey`:/ {
        print "\t@SerialName(\"auth_key\")"
    }
    in_data_class && /var `tokenId`:/ {
        print "\t@SerialName(\"token_id\")"
    }
    in_data_class && /var `signedTokenId`:/ {
        print "\t@SerialName(\"signed_token_id\")"
    }

    { print }
    ' "$file_path" > tmp && mv tmp "$file_path"
}

# Export the function so it's available to the subshell spawned by find
export -f add_serial_name

# Use find to locate Kotlin files and process them with the add_serial_name function
find "$SEARCH_DIR" -type f -name "$FILE_PATTERN" -exec bash -c 'add_serial_name "$0"' {} \;

echo "@SerialName annotations added to DepositMsg1 successfully."
