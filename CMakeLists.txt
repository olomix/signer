cmake_minimum_required(VERSION 4.1.1)
project(signer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(signer
    src/signer.cpp
)

target_include_directories(signer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)


# Add secp256k1 as subdirectory
set(SECP256K1_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_EXHAUSTIVE_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_RECOVERY ON CACHE BOOL "" FORCE)
add_subdirectory(external/secp256k1)

# Download and build libsodium from release tarball
include(ExternalProject)

set(LIBSODIUM_VERSION "1.0.20")
set(LIBSODIUM_URL "https://download.libsodium.org/libsodium/releases/libsodium-${LIBSODIUM_VERSION}.tar.gz")
set(LIBSODIUM_SHA256 "ebb65ef6ca439333c2bb41a0c1990587288da07f6c7fd07cb3a18cc18d30ce19")

ExternalProject_Add(
    libsodium_build
    URL ${LIBSODIUM_URL}
    URL_HASH SHA256=${LIBSODIUM_SHA256}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=${CMAKE_BINARY_DIR}/libsodium --disable-shared --enable-static --with-pic
    BUILD_COMMAND make
    INSTALL_COMMAND make install
    BUILD_IN_SOURCE 0
    BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/libsodium/lib/libsodium.a
)

# Create imported target for libsodium
add_library(sodium INTERFACE)
add_dependencies(sodium libsodium_build)
target_link_libraries(sodium INTERFACE ${CMAKE_BINARY_DIR}/libsodium/lib/libsodium.a)
target_include_directories(sodium INTERFACE ${CMAKE_BINARY_DIR}/libsodium/include)

target_link_libraries(signer
    PRIVATE
        secp256k1
        sodium
)


# Example: dependencies (if any)
# target_link_libraries(mylib PUBLIC fmt::fmt)

add_library(signer::signer ALIAS signer)

enable_testing()
add_subdirectory(tests)
