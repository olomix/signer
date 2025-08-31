#include <gtest/gtest.h>
#include <signer/signer.hpp>
#include <array>
#include <vector>

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
