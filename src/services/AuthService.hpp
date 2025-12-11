#pragma once

#include <string>
#include <drogon/drogon.h>

namespace services {

    // 登录结果
    struct LoginResult {
        int64_t userId;
        std::string username;
        std::string token;
        int64_t expiresAt;
    };

    // 注册结果
    struct RegisterResult {
        int64_t userId;
        std::string username;
    };

    // 认证服务
    class AuthService {
    public:
        // 单例获取
        static AuthService& instance();

        // 用户注册
        drogon::Task<RegisterResult> registerUser(const std::string& username,
                                                   const std::string& password,
                                                   const std::string& email);

        // 用户登录
        drogon::Task<LoginResult> login(const std::string& username,
                                         const std::string& password);

        // 登出（使 Token 失效）
        drogon::Task<bool> logout(int64_t userId, const std::string& token);

        // 刷新 Token
        drogon::Task<LoginResult> refreshToken(int64_t userId);

        // 验证 Token 是否在黑名单
        drogon::Task<bool> isTokenBlacklisted(const std::string& token);

        // 修改密码
        drogon::Task<bool> changePassword(int64_t userId,
                                           const std::string& oldPassword,
                                           const std::string& newPassword);

    private:
        AuthService() = default;
        ~AuthService() = default;
        AuthService(const AuthService&) = delete;
        AuthService& operator=(const AuthService&) = delete;

        // Token 黑名单 key
        [[nodiscard]] std::string buildBlacklistKey(const std::string& token) const;
    };

} // namespace services