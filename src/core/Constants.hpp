#pragma once

#include <string>
#include <chrono>

namespace core::constants {

    // JWT 配置
    inline constexpr const char* JWT_SECRET = "your-secret-key-change-in-production";
    inline constexpr const char* JWT_ISSUER = "drogon-scaffold";
    inline constexpr auto JWT_EXPIRE_DURATION = std::chrono::hours(24);

    // Redis Key 前缀
    inline constexpr const char* REDIS_PREFIX = "drogon:";
    inline constexpr const char* REDIS_TOKEN_PREFIX = "drogon:token:";
    inline constexpr const char* REDIS_USER_LOCK_PREFIX = "drogon:lock:user:";
    inline constexpr const char* REDIS_QUEUE_PREFIX = "drogon:queue:";

    // 队列配置
    inline constexpr size_t QUEUE_MAX_SIZE = 10000;
    inline constexpr auto QUEUE_TIMEOUT = std::chrono::seconds(30);

    // 用户锁配置
    inline constexpr auto USER_LOCK_TIMEOUT = std::chrono::seconds(60);

    // 分页默认值
    inline constexpr int DEFAULT_PAGE = 1;
    inline constexpr int DEFAULT_PAGE_SIZE = 10;
    inline constexpr int MAX_PAGE_SIZE = 100;

    // HTTP Header
    inline constexpr const char* HEADER_AUTHORIZATION = "Authorization";
    inline constexpr const char* HEADER_BEARER_PREFIX = "Bearer ";
    inline constexpr const char* HEADER_REQUEST_ID = "X-Request-Id";

} // namespace core::constants