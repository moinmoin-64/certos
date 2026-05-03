#include <gtest/gtest.h>
#include "certosc/auth/jwt_manager.h"
#include <string>

using namespace certosc;

TEST(JwtManagerTest, GenerateAndValidateToken) {
    JwtManager jwt_mgr("test_secret", 3600); // 1 hour expiry
    
    std::string token = jwt_mgr.create_token("u123", "alice", "user");
    EXPECT_FALSE(token.empty());
    
    auto decoded = jwt_mgr.validate_token(token);
    ASSERT_TRUE(decoded.has_value());
    
    EXPECT_EQ(decoded->user_id, "u123");
    EXPECT_EQ(decoded->username, "alice");
    EXPECT_EQ(decoded->role, "user");
}

TEST(JwtManagerTest, InvalidSignatureRejected) {
    JwtManager server1("secret1", 3600);
    JwtManager server2("secret2", 3600);
    
    std::string token = server1.create_token("u123", "alice", "user");
    
    // Server 2 should reject it because of different secret
    auto decoded = server2.validate_token(token);
    EXPECT_FALSE(decoded.has_value());
}

TEST(JwtManagerTest, ExpiredTokenRejected) {
    JwtManager jwt_mgr("test_secret", 0); // 0 seconds expiry => instantly expires
    
    std::string token = jwt_mgr.create_token("u123", "alice", "user");
    
    // It should be invalid immediately
    auto decoded = jwt_mgr.validate_token(token);
    EXPECT_FALSE(decoded.has_value());
}
