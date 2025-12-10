#include "AuthService.hpp"
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
    // 使用 token 的 hash 作为 key，避免 key 过长
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

    auto dbClient = drogon::app().getDbClient();

    // 检查用户是否已存在
    auto existResult = co_await dbClient->execSqlCoro(
        "SELECT id FROM users WHERE username = ? OR email = ? LIMIT 1",
        username, email
    );

    if (!existResult.empty()) {
        throw core::AppException(core::ErrorCode::USER_ALREADY_EXISTS);
    }

    // 生成盐值和密码哈希
    auto salt = utils::Crypto::generateSalt();
    auto passwordHash = utils::Crypto::hashPassword(password, salt);
    auto userId = utils::Crypto::uuid();

    // 插入用户
    co_await dbClient->execSqlCoro(
        "INSERT INTO users (id, username, email, password_hash, salt, created_at) "
        "VALUES (?, ?, ?, ?, ?, NOW())",
        userId, username, email, passwordHash, salt
    );

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

    // 查询用户
    auto result = co_await dbClient->execSqlCoro(
        "SELECT id, username, password_hash, salt, role, status FROM users "
        "WHERE username = ? LIMIT 1",
        username
    );

    if (result.empty()) {
        throw core::AppException(core::ErrorCode::USER_NOT_FOUND);
    }

    auto row = result[0];
    auto status = row["status"].as<int>();

    // 检查用户状态
    if (status != 1) {
        throw core::AppException(core::ErrorCode::USER_DISABLED);
    }

    // 验证密码
    auto passwordHash = row["password_hash"].as<std::string>();
    auto salt = row["salt"].as<std::string>();

    if (!utils::Crypto::verifyPassword(password, salt, passwordHash)) {
        throw core::AppException(core::ErrorCode::PASSWORD_INCORRECT);
    }

    auto userId = row["id"].as<std::string>();
    auto role = row["role"].as<std::string>();

    // 生成 Token
    auto token = middleware::JwtUtil::generate(userId, username, role);

    // 计算过期时间
    auto expiresAt = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch() + 
        core::constants::JWT_EXPIRE_DURATION
    ).count();

    // 更新最后登录时间
    co_await dbClient->execSqlCoro(
        "UPDATE users SET last_login_at = NOW() WHERE id = ?",
        userId
    );

    spdlog::info("User logged in: userId={}, username={}", userId, username);

    co_return LoginResult{
        .userId = userId,
        .username = username,
        .token = token,
        .expiresAt = expiresAt
    };
}

drogon::Task<bool> AuthService::logout(const std::string& userId, const std::string& token) {
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

drogon::Task<LoginResult> AuthService::refreshToken(const std::string& userId) {
    auto dbClient = drogon::app().getDbClient();

    // 查询用户
    auto result = co_await dbClient->execSqlCoro(
        "SELECT id, username, role, status FROM users WHERE id = ? LIMIT 1",
        userId
    );

    if (result.empty()) {
        throw core::AppException(core::ErrorCode::USER_NOT_FOUND);
    }

    auto row = result[0];
    auto status = row["status"].as<int>();

    if (status != 1) {
        throw core::AppException(core::ErrorCode::USER_DISABLED);
    }

    auto username = row["username"].as<std::string>();
    auto role = row["role"].as<std::string>();

    // 生成新 Token
    auto token = middleware::JwtUtil::generate(userId, username, role);
    auto expiresAt = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch() +
        core::constants::JWT_EXPIRE_DURATION
    ).count();

    spdlog::debug("Token refreshed: userId={}", userId);

    co_return LoginResult{
        .userId = userId,
        .username = username,
        .token = token,
        .expiresAt = expiresAt
    };
}

drogon::Task<bool> AuthService::isTokenBlacklisted(const std::string& token) {
    auto& redis = utils::Redis::instance();
    auto key = buildBlacklistKey(token);
    co_return co_await redis.exists(key);
}

drogon::Task<bool> AuthService::changePassword(const std::string& userId,
                                                const std::string& oldPassword,
                                                const std::string& newPassword) {
    if (newPassword.length() < 6) {
        throw core::ParamException("new password must be at least 6 characters");
    }

    auto dbClient = drogon::app().getDbClient();

    // 查询当前密码
    auto result = co_await dbClient->execSqlCoro(
        "SELECT password_hash, salt FROM users WHERE id = ? LIMIT 1",
        userId
    );

    if (result.empty()) {
        throw core::AppException(core::ErrorCode::USER_NOT_FOUND);
    }

    auto row = result[0];
    auto currentHash = row["password_hash"].as<std::string>();
    auto currentSalt = row["salt"].as<std::string>();

    // 验证旧密码
    if (!utils::Crypto::verifyPassword(oldPassword, currentSalt, currentHash)) {
        throw core::AppException(core::ErrorCode::PASSWORD_INCORRECT);
    }

    // 生成新盐值和哈希
    auto newSalt = utils::Crypto::generateSalt();
    auto newHash = utils::Crypto::hashPassword(newPassword, newSalt);

    // 更新密码
    co_await dbClient->execSqlCoro(
        "UPDATE users SET password_hash = ?, salt = ?, updated_at = NOW() WHERE id = ?",
        newHash, newSalt, userId
    );

    spdlog::info("Password changed: userId={}", userId);
    co_return true;
}

} // namespace services