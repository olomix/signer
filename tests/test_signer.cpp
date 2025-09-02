#include <gtest/gtest.h>
#include <signer/signer.hpp>
#include <signer/internal.hpp>
#include <array>
#include <vector>
#include <iomanip>

namespace {
// Helper: convert array/vector of bytes to lowercase hex string (fast)
template <typename ByteRange>
std::string hex_str(const ByteRange& bytes) {
    static constexpr char lut[] = "0123456789abcdef";
    const size_t n = bytes.size();
    std::string out;
    out.resize(n * 2);
    size_t j = 0;
    for (size_t i = 0; i < n; ++i) {
        const unsigned char b = static_cast<unsigned char>(bytes[i]);
        out[j++] = lut[b >> 4];
        out[j++] = lut[b & 0x0F];
    }
    return out;
}
} // namespace

TEST(SignerTest, One) {
    std::array<uint8_t, 64> seed;

    std::string mnemonic = std::string("sphere inflict carpet lamp clock tail talk project curtain grid pole soap cherry fiber flat");
    std::string salt_str = std::string("mnemonic") + std::string("xxxg");

    auto x = internal::pbkdf2_hmac_sha512(
        reinterpret_cast<const uint8_t*>(mnemonic.data()), mnemonic.size(),
        reinterpret_cast<const uint8_t*>(salt_str.data()), salt_str.size(),
        2048,
        seed.data(), seed.size()
    );
    EXPECT_EQ(x, 0);

    std::cout << "seed: " << hex_str(seed) << "\n";

    auto master = internal::bip32_master_from_seed(seed.data(), seed.size());

    std::cout << "Master Private Key: " << hex_str(master.priv) << "\n";
    std::cout << "Master Chain Code: " << hex_str(master.chain) << "\n";

    auto master_2 = internal::CKDpriv(master, 4);
    std::cout << "Master Private Key: " << hex_str(master_2.priv) << "\n";
    std::cout << "Master Chain Code: " << hex_str(master_2.chain) << "\n";

    auto master_3 = internal::CKDpriv(master, (1<<31) + 4);
    std::cout << "Master Private Key: " << hex_str(master_3.priv) << "\n";
    std::cout << "Master Chain Code: " << hex_str(master_3.chain) << "\n";

    auto master_bip44 = internal::CKDpriv(master, (1<<31) + 44);
    std::cout << "BIP44 Private Key: " << hex_str(master_bip44.priv) << "\n";
    std::cout << "BIP44 Chain Code: " << hex_str(master_bip44.chain) << "\n";
    auto master_ether = internal::CKDpriv(master_bip44, (1<<31) + 60);
    std::cout << "Ether Private Key: " << hex_str(master_ether.priv) << "\n";
    std::cout << "Ether Chain Code: " << hex_str(master_ether.chain) << "\n";
}

TEST(SignerTest, Two) {
    std::vector<uint8_t> in = {0x1B, 0x3E, 0x26};
    auto b = internal::encodeBase58(in);
    EXPECT_EQ(b, "A9jT");

    in = {0x00, 0x1B, 0x3E, 0x26};
    b = internal::encodeBase58(in);
    EXPECT_EQ(b, "1A9jT");
}


TEST(SignerTest, SignData) {
    // Test private key (32 bytes)
    std::array<uint8_t, 32> private_key = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
    };
    
    // Test data to sign
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
    
    // Sign the data
    auto signature = sign(private_key, data);
    
    // Check that signature was created (should be 64 bytes)
    EXPECT_EQ(signature.size(), 64);
    
    // Check that signature is not all zeros
    bool all_zeros = true;
    for (auto byte : signature) {
        if (byte != 0) {
            all_zeros = false;
            break;
        }
    }
    EXPECT_FALSE(all_zeros);
}

TEST(SignerTest, SignWithMnemonic) {
    std::string mnemonic = "sphere inflict carpet lamp clock tail talk project curtain grid pole soap cherry fiber flat";
    std::string password = "xxxg";
    std::string path = "m/44'/60'/0'/0/0";
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
    
    auto signature = sign(mnemonic, password, path, data);
    
    EXPECT_EQ(signature.size(), 64);
    
    bool all_zeros = true;
    for (auto byte : signature) {
        if (byte != 0) {
            all_zeros = false;
            break;
        }
    }
    EXPECT_FALSE(all_zeros);
}

TEST(SignerTest, ParsePath) {
    auto indices = internal::parsePath("m/44'/60'/0'/0/0");
    EXPECT_EQ(indices.size(), 5);
    EXPECT_EQ(indices[0], (1u << 31) + 44);  // 44' (hardened)
    EXPECT_EQ(indices[1], (1u << 31) + 60);  // 60' (hardened)
    EXPECT_EQ(indices[2], (1u << 31) + 0);   // 0' (hardened)
    EXPECT_EQ(indices[3], 0);                // 0 (not hardened)
    EXPECT_EQ(indices[4], 0);                // 0 (not hardened)
    
    auto simple_path = internal::parsePath("m/44'/60'/3");
    EXPECT_EQ(simple_path.size(), 3);
    EXPECT_EQ(simple_path[0], (1u << 31) + 44);
    EXPECT_EQ(simple_path[1], (1u << 31) + 60);
    EXPECT_EQ(simple_path[2], 3);
    
    auto invalid_path = internal::parsePath("invalid");
    EXPECT_EQ(invalid_path.size(), 0);
}
