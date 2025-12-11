#pragma once

#include <drogon/HttpFilter.h>
#include <jwt-cpp/jwt.h>
#include <string>
#include <optional>

namespace middleware {

    // JWT 载荷信息
    struct JwtPayload {
        std::string userId;   // 存储为字符串，使用时转换
        std::string username;
        std::string role;
        int64_t exp;  // 过期时间
        int64_t iat;  // 签发时间
    };

    // JWT 工具类
    class JwtUtil {
    public:
        // 生成 Token（userId 作为字符串传入）
        static std::string generate(const std::string& userId,
                                    const std::string& username,
                                    const std::string& role = "user");

        // 验证并解析 Token
        static std::optional<JwtPayload> verify(const std::string& token);

        // 从请求头提取 Token
        static std::optional<std::string> extractToken(const drogon::HttpRequestPtr& req);

        // 配置
        static void setSecret(const std::string& secret);
        static void setIssuer(const std::string& issuer);
        static void setExpireDuration(std::chrono::seconds duration);

    private:
        static std::string secret_;
        static std::string issuer_;
        static std::chrono::seconds expireDuration_;
    };

    // JWT 认证过滤器
    class JwtFilter : public drogon::HttpFilter<JwtFilter> {
    public:
        JwtFilter() = default;

        void doFilter(const drogon::HttpRequestPtr& req,
                      drogon::FilterCallback&& fcb,
                      drogon::FilterChainCallback&& fccb) override;
    };

} // namespace middleware