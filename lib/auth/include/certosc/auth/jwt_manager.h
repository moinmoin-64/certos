#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace certosc {

/// Decoded JWT claims
struct TokenClaims {
    std::string user_id;
    std::string username;
    std::string role;    // "user", "admin", "node"
    int64_t expires_at;
    int64_t issued_at;
};

/// JWT token manager — creates and validates tokens
class JwtManager {
public:
    explicit JwtManager(const std::string& secret, uint32_t expiry_seconds = 3600);

    /// Create a signed JWT token
    std::string create_token(const std::string& user_id,
                              const std::string& username,
                              const std::string& role);

    /// Validate and decode a JWT token
    /// Returns nullopt if token is invalid or expired
    std::optional<TokenClaims> validate_token(const std::string& token);

private:
    std::string secret_;
    uint32_t expiry_seconds_;
};

} // namespace certosc
