#ifndef SIGNER_SIGNER_HPP
#define SIGNER_SIGNER_HPP

#ifdef __cplusplus
#include <vector>
#include <array>
#include <cstdint>
#include <string>
#else
#include <stdint.h>
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// C-compatible sign function with private key
// Returns 1 on success, 0 on failure
// private_key: 32-byte private key
// data: data to sign
// data_len: length of data
// signature_out: output buffer for 64-byte signature (must be at least 64 bytes)
int sign_c(const uint8_t* private_key,
           const uint8_t* data,
           size_t data_len,
           uint8_t* signature_out);

// C-compatible sign function with mnemonic/path
// Returns 1 on success, 0 on failure
// mnemonic: BIP39 mnemonic phrase (null-terminated string)
// password: Optional password (null-terminated string, can be NULL for no password)
// path: BIP32 derivation path like "m/44'/60'/0'/0/0" (null-terminated string)
// data: data to sign
// data_len: length of data
// signature_out: output buffer for 64-byte signature (must be at least 64 bytes)
int sign_mnemonic_c(const char* mnemonic,
                    const char* password,
                    const char* path,
                    const uint8_t* data,
                    size_t data_len,
                    uint8_t* signature_out);

#ifdef __cplusplus
}

// C++ API
// Sign arbitrary data with a private key using secp256k1
// Returns the signature in compact format (64 bytes)
// private_key: 32-byte private key
// data: arbitrary data to sign
// Returns: 64-byte signature or empty vector on failure
std::vector<uint8_t> sign(const std::array<uint8_t, 32>& private_key, 
                          const std::vector<uint8_t>& data);

// Sign arbitrary data using a mnemonic, password, and BIP32 derivation path
// mnemonic: BIP39 mnemonic phrase
// password: Optional password (use empty string for no password)
// path: BIP32 derivation path like "m/44'/60'/0'/0/0" (' means hardened)
// data: arbitrary data to sign
// Returns: 64-byte signature or empty vector on failure
std::vector<uint8_t> sign(const std::string& mnemonic,
                          const std::string& password,
                          const std::string& path,
                          const std::vector<uint8_t>& data);
#endif

#endif // SIGNER_SIGNER_HPP
