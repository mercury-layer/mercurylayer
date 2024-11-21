# cmake/secp256k1_zkp.cmake
include(ExternalProject)

# Set secp256k1-zkp build directory relative to the main build directory
set(SECP256K1_BUILD_DIR ${CMAKE_BINARY_DIR}/secp256k1-zkp)
set(SECP256K1_INSTALL_DIR ${CMAKE_BINARY_DIR}/secp256k1-zkp-install)

# Configure and build secp256k1-zkp as an external project
ExternalProject_Add(
    secp256k1_zkp_external
    GIT_REPOSITORY https://github.com/ssantos21/secp256k1-zkp.git
    GIT_TAG blinded-musig-scheme
    PREFIX ${SECP256K1_BUILD_DIR}
    CONFIGURE_COMMAND 
        cd <SOURCE_DIR> &&
        ./autogen.sh &&
        ./configure 
            --prefix=${SECP256K1_INSTALL_DIR}
            --enable-module-schnorrsig 
            --enable-experimental 
            --enable-module-musig 
            --enable-benchmark=no 
            --enable-tests=no 
            --enable-exhaustive-tests=no
    BUILD_COMMAND cd <SOURCE_DIR> && make
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS 
        <SOURCE_DIR>/.libs/libsecp256k1.a
)

# Create an interface library for secp256k1-zkp
add_library(secp256k1_zkp INTERFACE)
add_dependencies(secp256k1_zkp secp256k1_zkp_external)

# Get the source directory of secp256k1-zkp after it's cloned
ExternalProject_Get_Property(secp256k1_zkp_external SOURCE_DIR)

# Set include directories and link the static library
target_include_directories(secp256k1_zkp 
    INTERFACE 
        ${SOURCE_DIR}/include
)

target_link_libraries(secp256k1_zkp
    INTERFACE
        ${SOURCE_DIR}/.libs/libsecp256k1.a
)
