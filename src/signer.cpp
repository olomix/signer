#include <signer/signer.hpp>
#include <signer/internal.hpp>
#include <sodium.h>
#include <secp256k1.h>
#include <memory>
#include <cstring>

// C wrapper functions
extern "C" {

int sign_c(const uint8_t* private_key,
           const uint8_t* data,
           size_t data_len,
           uint8_t* signature_out) {
    if (!private_key || !data || !signature_out) {
        return 0;
    }
    
    std::array<uint8_t, 32> priv_key;
    std::memcpy(priv_key.data(), private_key, 32);
    
    std::vector<uint8_t> data_vec(data, data + data_len);
    
    auto result = sign(priv_key, data_vec);
    
    if (result.empty()) {
        return 0;
    }
    
    std::memcpy(signature_out, result.data(), 64);
    return 1;
}

int sign_mnemonic_c(const char* mnemonic,
                    const char* password,
                    const char* path,
                    const uint8_t* data,
                    size_t data_len,
                    uint8_t* signature_out) {
    if (!mnemonic || !path || !data || !signature_out) {
        return 0;
    }
    
    std::string mnemonic_str(mnemonic);
    std::string password_str(password ? password : "");
    std::string path_str(path);
    std::vector<uint8_t> data_vec(data, data + data_len);
    
    auto result = sign(mnemonic_str, password_str, path_str, data_vec);
    
    if (result.empty()) {
        return 0;
    }
    
    std::memcpy(signature_out, result.data(), 64);
    return 1;
}

} // extern "C"

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

std::vector<uint8_t> sign(const std::string& mnemonic,
                          const std::string& password,
                          const std::string& path,
                          const std::vector<uint8_t>& data) {
    std::array<uint8_t, 64> seed;
    std::string salt_str = std::string("mnemonic") + password;
    
    auto result = internal::pbkdf2_hmac_sha512(
        reinterpret_cast<const uint8_t*>(mnemonic.data()), mnemonic.size(),
        reinterpret_cast<const uint8_t*>(salt_str.data()), salt_str.size(),
        2048,
        seed.data(), seed.size()
    );
    
    if (result != 0) {
        return {};
    }
    
    auto master = internal::bip32_master_from_seed(seed.data(), seed.size());
    
    auto path_indices = internal::parsePath(path);
    if (path_indices.empty()) {
        return {};
    }
    
    auto current_node = master;
    for (uint32_t index : path_indices) {
        current_node = internal::CKDpriv(current_node, index);
    }
    
    return sign(current_node.priv, data);
}

namespace internal {
    const char* BASE58_ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    std::string encodeBase58(const std::vector<uint8_t>& data) {
        // 1. Count leading zeros
        size_t zeroCount = 0;
        while (zeroCount < data.size() && data[zeroCount] == 0) {
            zeroCount++;
        }
    
        // 2. Convert byte array to big integer (base256 → base58)
        std::vector<uint8_t> temp(data.begin(), data.end());
        std::string result;
    
        size_t startAt = zeroCount;
        while (startAt < temp.size()) {
            int remainder = 0;
            std::vector<uint8_t> newTemp;
        
            for (size_t i = startAt; i < temp.size(); i++) {
                int value = (remainder << 8) + temp[i];
                int quotient = value / 58;
                remainder = value % 58;
                if (!newTemp.empty() || quotient != 0) {
                    newTemp.push_back((uint8_t)quotient);
                }
            }
        
            result.insert(result.begin(), BASE58_ALPHABET[remainder]);
            temp = newTemp;
            startAt = 0;
        }
    
        // 3. Add '1' for each leading 0 byte
        while (zeroCount--) {
            result.insert(result.begin(), '1');
        }
    
        return result;
    }

    int pbkdf2_hmac_sha512(const uint8_t* password, size_t pwLen,
                          const uint8_t* salt, size_t saltLen,
                          uint32_t iterations,
                          uint8_t* out, size_t dkLen) {
        if (iterations == 0 || dkLen == 0) return -1;

        const size_t hLen = crypto_auth_hmacsha512_BYTES; // 64
        uint32_t blocks = static_cast<uint32_t>((dkLen + hLen - 1) / hLen);

        std::vector<uint8_t> U(hLen);
        std::vector<uint8_t> T(hLen);
        std::vector<uint8_t> saltBlock(saltLen + 4);

        // salt || INT_32_BE(i)
        std::memcpy(saltBlock.data(), salt, saltLen);

        size_t outPos = 0;
        for (uint32_t i = 1; i <= blocks; ++i) {
            // Write block index in big-endian
            saltBlock[saltLen + 0] = static_cast<uint8_t>((i >> 24) & 0xFF);
            saltBlock[saltLen + 1] = static_cast<uint8_t>((i >> 16) & 0xFF);
            saltBlock[saltLen + 2] = static_cast<uint8_t>((i >> 8) & 0xFF);
            saltBlock[saltLen + 3] = static_cast<uint8_t>((i >> 0) & 0xFF);

            // U1 = HMAC(password, salt||i)
            crypto_auth_hmacsha512_state st;
            crypto_auth_hmacsha512_init(&st, password, pwLen);
            crypto_auth_hmacsha512_update(&st, saltBlock.data(), saltBlock.size());
            crypto_auth_hmacsha512_final(&st, U.data());

            // T = U1
            std::memcpy(T.data(), U.data(), hLen);

            // U2..Uc
            for (uint32_t c = 2; c <= iterations; ++c) {
                crypto_auth_hmacsha512_state st2;
                crypto_auth_hmacsha512_init(&st2, password, pwLen);
                crypto_auth_hmacsha512_update(&st2, U.data(), hLen);
                crypto_auth_hmacsha512_final(&st2, U.data());

                // T ^= Uc
                for (size_t k = 0; k < hLen; ++k) T[k] ^= U[k];
            }

            size_t copyLen = (outPos + hLen <= dkLen) ? hLen : (dkLen - outPos);
            std::memcpy(out + outPos, T.data(), copyLen);
            outPos += copyLen;
        }
        return 0;
    }

    MasterNode bip32_master_from_seed(const uint8_t* seed, size_t seed_len) {
        uint8_t I[64];
        const char* key = "Bitcoin seed";
        crypto_auth_hmacsha512_state st;
        crypto_auth_hmacsha512_init(&st, (const unsigned char*)key, strlen(key));
        crypto_auth_hmacsha512_update(&st, seed, seed_len);
        crypto_auth_hmacsha512_final(&st, I);

        MasterNode out{};
        memcpy(out.priv.data(),  I, 32);
        memcpy(out.chain.data(), I+32, 32);

        // Optional: check 0 < priv < n using libsecp256k1
        // (If invalid, treat the seed as invalid.)
        return out;
    }

    MasterNode CKDpriv(const MasterNode& parent, uint32_t index) {
        // Static context for speed (reuse across calls)
        static secp256k1_context* ctx = [](){
            return secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
        }();

        // Data per BIP32: HMAC-SHA512(key=c_par, data=...)
        // hardened: 0x00 || ser256(k_par) || ser32(i)
        // normal  : serP(point(k_par)) || ser32(i)
        uint8_t data[1 + 33 + 32 + 4]; // max size
        size_t data_len = 0;

        const bool hardened = (index & 0x80000000u) != 0;
        if (hardened) {
            data[0] = 0x00;
            memcpy(data + 1, parent.priv.data(), 32);
            data_len = 1 + 32;
        } else {
            // Compute compressed pubkey from parent priv
            secp256k1_pubkey pubkey;
            if (!secp256k1_ec_pubkey_create(ctx, &pubkey, parent.priv.data())) {
                return MasterNode{}; // invalid parent key
            }
            uint8_t pubser[33];
            size_t publen = sizeof(pubser);
            secp256k1_ec_pubkey_serialize(ctx, pubser, &publen, &pubkey, SECP256K1_EC_COMPRESSED);
            memcpy(data, pubser, publen);
            data_len = publen;
        }
        // Append ser32(index) big-endian
        data[data_len + 0] = static_cast<uint8_t>((index >> 24) & 0xFF);
        data[data_len + 1] = static_cast<uint8_t>((index >> 16) & 0xFF);
        data[data_len + 2] = static_cast<uint8_t>((index >> 8) & 0xFF);
        data[data_len + 3] = static_cast<uint8_t>((index >> 0) & 0xFF);
        data_len += 4;

        // I = HMAC-SHA512(key=c_par, data)
        uint8_t I[64];
        crypto_auth_hmacsha512_state st;
        crypto_auth_hmacsha512_init(&st, parent.chain.data(), parent.chain.size());
        crypto_auth_hmacsha512_update(&st, data, data_len);
        crypto_auth_hmacsha512_final(&st, I);

        // Split IL, IR
        const uint8_t* IL = I;
        const uint8_t* IR = I + 32;

        // child_priv = (IL + k_par) mod n, failing if IL >= n or result == 0
        MasterNode child{};
        memcpy(child.chain.data(), IR, 32);
        memcpy(child.priv.data(), parent.priv.data(), 32);

        // Tweak add in place
        if (!secp256k1_ec_seckey_tweak_add(ctx, child.priv.data(), IL)) {
            // invalid derivation per BIP32; return zeroed node
            return MasterNode{};
        }

        return child;
    }

    std::vector<uint32_t> parsePath(const std::string& path) {
        std::vector<uint32_t> indices;
        
        if (path.empty() || path[0] != 'm') {
            return {};
        }
        
        size_t pos = 1;
        if (pos >= path.length() || path[pos] != '/') {
            return {};
        }
        pos++;
        
        while (pos < path.length()) {
            size_t end = path.find('/', pos);
            if (end == std::string::npos) {
                end = path.length();
            }
            
            std::string component = path.substr(pos, end - pos);
            if (component.empty()) {
                return {};
            }
            
            bool hardened = false;
            if (component.back() == '\'') {
                hardened = true;
                component.pop_back();
            }
            
            uint32_t index = 0;
            try {
                index = std::stoul(component);
            } catch (...) {
                return {};
            }
            
            if (hardened) {
                index += (1u << 31);
            }
            
            indices.push_back(index);
            pos = end + 1;
        }
        
        return indices;
    }
}
