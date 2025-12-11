#include "JwtFilter.hpp"
#include "core/Response.hpp"
#include "core/Constants.hpp"
#include "core/Exception.hpp"
#include <spdlog/spdlog.h>

namespace middleware {

// 静态成员初始化
std::string JwtUtil::secret_ = core::constants::JWT_SECRET;
std::string JwtUtil::issuer_ = core::constants::JWT_ISSUER;
std::chrono::seconds JwtUtil::expireDuration_ = 
    std::chrono::duration_cast<std::chrono::seconds>(core::constants::JWT_EXPIRE_DURATION);

void JwtUtil::setSecret(const std::string& secret) {
    secret_ = secret;
}

void JwtUtil::setIssuer(const std::string& issuer) {
    issuer_ = issuer;
}

void JwtUtil::setExpireDuration(std::chrono::seconds duration) {
    expireDuration_ = duration;
}

std::string JwtUtil::generate(const std::string& userId,
                               const std::string& username,
                               const std::string& role) {
    auto now = std::chrono::system_clock::now();
    auto exp = now + expireDuration_;

    auto token = jwt::create()
        .set_issuer(issuer_)
        .set_type("JWT")
        .set_issued_at(now)
        .set_expires_at(exp)
        .set_payload_claim("userId", jwt::claim(userId))
        .set_payload_claim("username", jwt::claim(username))
        .set_payload_claim("role", jwt::claim(role))
        .sign(jwt::algorithm::hs256{secret_});

    spdlog::debug("JWT generated for user: {}", userId);
    return token;
}

std::optional<JwtPayload> JwtUtil::verify(const std::string& token) {
    try {
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secret_})
            .with_issuer(issuer_);

        auto decoded = jwt::decode(token);
        verifier.verify(decoded);

        JwtPayload payload;
        payload.userId = decoded.get_payload_claim("userId").as_string();
        payload.username = decoded.get_payload_claim("username").as_string();
        payload.role = decoded.get_payload_claim("role").as_string();
        payload.exp = decoded.get_expires_at().time_since_epoch().count();
        payload.iat = decoded.get_issued_at().time_since_epoch().count();

        return payload;

    } catch (const jwt::error::token_verification_exception& e) {
        spdlog::warn("JWT verification failed: {}", e.what());
        return std::nullopt;
    } catch (const std::exception& e) {
        spdlog::error("JWT decode error: {}", e.what());
        return std::nullopt;
    }
}

std::optional<std::string> JwtUtil::extractToken(const drogon::HttpRequestPtr& req) {
    auto authHeader = req->getHeader(core::constants::HEADER_AUTHORIZATION);
    
    if (authHeader.empty()) {
        return std::nullopt;
    }

    // 检查 Bearer 前缀
    const std::string_view prefix = core::constants::HEADER_BEARER_PREFIX;
    if (authHeader.size() <= prefix.size() || 
        authHeader.substr(0, prefix.size()) != prefix) {
        return std::nullopt;
    }

    return authHeader.substr(prefix.size());
}

// ==================== JwtFilter ====================

void JwtFilter::doFilter(const drogon::HttpRequestPtr& req,
                         drogon::FilterCallback&& fcb,
                         drogon::FilterChainCallback&& fccb) {
    // 提取 Token
    auto token = JwtUtil::extractToken(req);
    if (!token) {
        spdlog::debug("JWT missing in request: {}", req->getPath());
        fcb(core::Response::error(core::ErrorCode::TOKEN_MISSING));
        return;
    }

    // 验证 Token
    auto payload = JwtUtil::verify(*token);
    if (!payload) {
        spdlog::debug("JWT invalid for request: {}", req->getPath());
        fcb(core::Response::error(core::ErrorCode::TOKEN_INVALID));
        return;
    }

    // 检查过期
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    if (payload->exp < now) {
        spdlog::debug("JWT expired for user: {}", payload->userId);
        fcb(core::Response::error(core::ErrorCode::TOKEN_EXPIRED));
        return;
    }

    // 将用户信息存入请求属性
    req->getAttributes()->insert("userId", payload->userId);
    req->getAttributes()->insert("username", payload->username);
    req->getAttributes()->insert("role", payload->role);

    spdlog::debug("JWT verified: userId={}, path={}", payload->userId, req->getPath());

    // 继续处理链
    fccb();
}

} // namespace middleware