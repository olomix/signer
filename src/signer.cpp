#include <signer/signer.hpp>
#include <sodium.h>
#include <secp256k1.h>
#include <memory>

std::vector<uint8_t> sign(const std::array<uint8_t, 32>& private_key, 
                          const std::vector<uint8_t>& data) {
    // Create context
    auto secp256k1_deleter = [](secp256k1_context* ctx) {
        if (ctx) {
            secp256k1_context_destroy(ctx);
        }
    };
    
    std::unique_ptr<secp256k1_context, decltype(secp256k1_deleter)> ctx(
        secp256k1_context_create(SECP256K1_CONTEXT_NONE),
        secp256k1_deleter
    );
    
    if (!ctx) {
        return {};
    }
    
    // Hash the data with SHA256 using libsodium
    std::array<uint8_t, 32> hash;
    crypto_hash_sha256(hash.data(), data.data(), data.size());
    
    // Create signature
    secp256k1_ecdsa_signature sig;
    if (!secp256k1_ecdsa_sign(ctx.get(), &sig, hash.data(), private_key.data(), nullptr, nullptr)) {
        return {};
    }
    
    // Serialize signature to compact format (64 bytes)
    std::vector<uint8_t> sig_compact(64);
    secp256k1_ecdsa_signature_serialize_compact(ctx.get(), sig_compact.data(), &sig);
    
    return sig_compact;
}
