#ifndef SIGNER_INTERNAL_HPP
#define SIGNER_INTERNAL_HPP

#include <vector>
#include <string>
#include <array>
#include <cstdint>

namespace internal {
    std::string encodeBase58(const std::vector<uint8_t>& data);
    
    int pbkdf2_hmac_sha512(const uint8_t* password, size_t pwLen,
                          const uint8_t* salt, size_t saltLen,
                          uint32_t iterations,
                          uint8_t* out, size_t dkLen);
    
    struct MasterNode {
        std::array<uint8_t, 32> priv;   // k_master
        std::array<uint8_t, 32> chain;  // c_master
    };
    
    MasterNode bip32_master_from_seed(const uint8_t* seed, size_t seed_len);

    // Child Key Derivation (private): BIP-0032 CKDpriv
    // Given parent extended private key and index, derive child extended private key.
    // Hardened if index >= 0x80000000.
    MasterNode CKDpriv(const MasterNode& parent, uint32_t index);
    
    // Parse BIP32 derivation path like "m/44'/60'/3" into a vector of indices
    // ' means hardened (adds 0x80000000 to the index)
    std::vector<uint32_t> parsePath(const std::string& path);
}

#endif // SIGNER_INTERNAL_HPP
