#include "AuthService.hpp"
#include "models/UserMapper.hpp"
#include "core/Exception.hpp"
#include "core/Constants.hpp"
#include "utils/Redis.hpp"
#include "utils/Crypto.hpp"
#include "middleware/JwtFilter.hpp"
#include <spdlog/spdlog.h>

namespace services {

AuthService& AuthService::instance() {
    static AuthService instance;
    return instance;
}

std::string AuthService::buildBlacklistKey(const std::string& token) const {
    return std::string(core::constants::REDIS_TOKEN_PREFIX) + "blacklist:" +
           utils::Crypto::md5(token);
}

drogon::Task<RegisterResult> AuthService::registerUser(const std::string& username,
                                                        const std::string& password,
                                                        const std::string& email) {
    // 参数验证
    if (username.empty() || password.empty()) {
        throw core::ParamException("username and password required");
    }

    if (password.length() < 6) {
        throw core::ParamException("password must be at least 6 characters");
    }

    if (email.empty()) {
        throw core::ParamException("email required");
    }

    auto dbClient = drogon::app().getDbClient();
    models::UserMapper mapper(dbClient);

    // 检查用户是否已存在
    bool exists = co_await mapper.existsByUsernameOrEmail(username, email);
    if (exists) {
        throw core::AppException(core::ErrorCode::USER_ALREADY_EXISTS);
    }

    // 生成盐值和密码哈希
    auto salt = utils::Crypto::generateSalt();
    auto passwordHash = utils::Crypto::hashPassword(password, salt);

    // 构建用户对象
    models::Users user;
    user.setUsername(username);
    user.setEmail(email);
    user.setPasswordHash(passwordHash);
    user.setSalt(salt);
    user.setRole("user");
    user.setStatus(1);

    // 插入用户
    int64_t userId = co_await mapper.insert(user);

    spdlog::info("User registered: userId={}, username={}", userId, username);

    co_return RegisterResult{
        .userId = userId,
        .username = username
    };
}

drogon::Task<LoginResult> AuthService::login(const std::string& username,
                                              const std::string& password) {
    if (username.empty() || password.empty()) {
        throw core::ParamException("username and password required");
    }

    auto dbClient = drogon::app().getDbClient();
    models::UserMapper mapper(dbClient);

    // 查询用户
    auto userOpt = co_await mapper.findByUsername(username);
    if (!userOpt) {
        throw core::AppException(core::ErrorCode::USER_NOT_FOUND);
    }

    auto& user = *userOpt;

    // 检查用户状态
    if (user.getStatus() != 1) {
        throw core::AppException(core::ErrorCode::USER_DISABLED);
    }

    // 验证密码
    if (!utils::Crypto::verifyPassword(password, user.getSalt(), user.getPasswordHash())) {
        throw core::AppException(core::ErrorCode::PASSWORD_INCORRECT);
    }

    // 生成 Token
    auto token = middleware::JwtUtil::generate(
        std::to_string(user.getId()),
        user.getUsername(),
        user.getRole()
    );

    // 计算过期时间
    auto expiresAt = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch() +
        core::constants::JWT_EXPIRE_DURATION
    ).count();

    // 更新最后登录时间
    co_await mapper.updateLastLoginTime(user.getId());

    spdlog::info("User logged in: userId={}, username={}", user.getId(), username);

    co_return LoginResult{
        .userId = user.getId(),
        .username = user.getUsername(),
        .token = token,
        .expiresAt = expiresAt
    };
}

drogon::Task<bool> AuthService::logout(int64_t userId, const std::string& token) {
    // 将 Token 加入黑名单
    auto& redis = utils::Redis::instance();
    auto key = buildBlacklistKey(token);

    // 黑名单过期时间与 Token 过期时间一致
    auto ttl = std::chrono::duration_cast<std::chrono::seconds>(
        core::constants::JWT_EXPIRE_DURATION
    );

    co_await redis.setEx(key, "1", ttl);

    spdlog::info("User logged out: userId={}", userId);
    co_return true;
}

drogon::Task<LoginResult> AuthService::refreshToken(int64_t userId) {
    auto dbClient = drogon::app().getDbClient();
    models::UserMapper mapper(dbClient);

    // 查询用户
    auto user = co_await mapper.findById(userId);

    if (user.getStatus() != 1) {
        throw core::AppException(core::ErrorCode::USER_DISABLED);
    }

    // 生成新 Token
    auto token = middleware::JwtUtil::generate(
        std::to_string(user.getId()),
        user.getUsername(),
        user.getRole()
    );

    auto expiresAt = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch() +
        core::constants::JWT_EXPIRE_DURATION
    ).count();

    spdlog::debug("Token refreshed: userId={}", userId);

    co_return LoginResult{
        .userId = user.getId(),
        .username = user.getUsername(),
        .token = token,
        .expiresAt = expiresAt
    };
}

drogon::Task<bool> AuthService::isTokenBlacklisted(const std::string& token) {
    auto& redis = utils::Redis::instance();
    auto key = buildBlacklistKey(token);
    co_return co_await redis.exists(key);
}

drogon::Task<bool> AuthService::changePassword(int64_t userId,
                                                const std::string& oldPassword,
                                                const std::string& newPassword) {
    if (newPassword.length() < 6) {
        throw core::ParamException("new password must be at least 6 characters");
    }

    auto dbClient = drogon::app().getDbClient();
    models::UserMapper mapper(dbClient);

    // 查询用户
    auto user = co_await mapper.findById(userId);

    // 验证旧密码
    if (!utils::Crypto::verifyPassword(oldPassword, user.getSalt(), user.getPasswordHash())) {
        throw core::AppException(core::ErrorCode::PASSWORD_INCORRECT);
    }

    // 生成新盐值和哈希
    auto newSalt = utils::Crypto::generateSalt();
    auto newHash = utils::Crypto::hashPassword(newPassword, newSalt);

    // 更新密码
    std::vector<std::pair<std::string, std::string>> fields = {
        {"password_hash", newHash},
        {"salt", newSalt}
    };
    co_await mapper.updateFields(userId, fields);

    spdlog::info("Password changed: userId={}", userId);
    co_return true;
}

} // namespace services