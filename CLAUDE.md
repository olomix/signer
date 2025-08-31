# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Test Commands

```shell
# Initial setup (first time only)
git submodule update --init --recursive

# Build the project
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build

# Run all tests with verbose output
ctest --test-dir build -V

# Run a specific test
ctest --test-dir build -R test_signer -V
```

## Architecture

This is a C++ cryptographic signing library that uses secp256k1 for ECDSA signatures.

### Core Components

- **signer library** (`src/signer.cpp`, `include/signer/signer.hpp`): Main library implementing the `sign()` function that creates ECDSA signatures using secp256k1
- **Dependencies**:
  - libsodium: Modern cryptographic library used for SHA256 hashing (downloaded and built from release tarball v1.0.20)
  - secp256k1: Bitcoin's elliptic curve library for signature generation (included as git submodule in `external/secp256k1`)
  - GoogleTest: Testing framework (included as git submodule in `external/googletest`)

### Key Implementation Details

- The `sign()` function takes a 32-byte private key and arbitrary data, hashes the data with SHA256 using libsodium's `crypto_hash_sha256`, and produces a 64-byte compact ECDSA signature
- Uses RAII with `std::unique_ptr` for secp256k1 context management
- libsodium is built using ExternalProject from official release tarball with SHA256 verification
- Test infrastructure uses GoogleTest with CTest integration