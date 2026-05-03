#include "certosc/auth/jwt_manager.h"
#include "certosc/common/logging.h"
#include "certosc/common/types.h"

#include <jwt-cpp/jwt.h>
#include <chrono>

namespace certosc {

JwtManager::JwtManager(const std::string& secret, uint32_t expiry_seconds)
    : secret_(secret), expiry_seconds_(expiry_seconds) {
}

std::string JwtManager::create_token(const std::string& user_id,
                                      const std::string& username,
                                      const std::string& role) {
    auto now = std::chrono::system_clock::now();
    auto expires = now + std::chrono::seconds(expiry_seconds_);

    auto token = jwt::create()
        .set_issuer("certosc")
        .set_type("JWT")
        .set_issued_at(now)
        .set_expires_at(expires)
        .set_payload_claim("user_id", jwt::claim(user_id))
        .set_payload_claim("username", jwt::claim(username))
        .set_payload_claim("role", jwt::claim(role))
        .sign(jwt::algorithm::hs256{secret_});

    LOG_DEBUG("Created JWT token for user '{}' (role={})", username, role);
    return token;
}

std::optional<TokenClaims> JwtManager::validate_token(const std::string& token) {
    try {
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secret_})
            .with_issuer("certosc");

        auto decoded = jwt::decode(token);
        verifier.verify(decoded);

        TokenClaims claims;
        claims.user_id = decoded.get_payload_claim("user_id").as_string();
        claims.username = decoded.get_payload_claim("username").as_string();
        claims.role = decoded.get_payload_claim("role").as_string();
        claims.expires_at = std::chrono::duration_cast<std::chrono::seconds>(
            decoded.get_expires_at().time_since_epoch()
        ).count();
        claims.issued_at = std::chrono::duration_cast<std::chrono::seconds>(
            decoded.get_issued_at().time_since_epoch()
        ).count();

        return claims;
    } catch (const std::exception& e) {
        LOG_WARN("JWT validation failed: {}", e.what());
        return std::nullopt;
    }
}

} // namespace certosc
