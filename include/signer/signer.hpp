#include <vector>
#include <array>
#include <cstdint>

// Sign arbitrary data with a private key using secp256k1
// Returns the signature in compact format (64 bytes)
// private_key: 32-byte private key
// data: arbitrary data to sign
// Returns: 64-byte signature or empty vector on failure
std::vector<uint8_t> sign(const std::array<uint8_t, 32>& private_key, 
                          const std::vector<uint8_t>& data);
