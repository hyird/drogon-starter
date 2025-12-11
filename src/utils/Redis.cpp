#include "Redis.hpp"
#include "core/Exception.hpp"
#include <spdlog/spdlog.h>

namespace utils {

Redis& Redis::instance() {
    static Redis instance;
    return instance;
}

drogon::nosql::RedisClientPtr Redis::client() {
    auto redisClient = drogon::app().getRedisClient();
    if (!redisClient) {
        throw core::RedisException(core::ErrorCode::REDIS_CONNECTION_ERROR,
                                   "Redis client not configured");
    }
    return redisClient;
}

drogon::Task<bool> Redis::set(const std::string& key, const std::string& value) {
    try {
        auto redis = client();
        co_await redis->execCommandCoro("SET %s %s", key.c_str(), value.c_str());
        co_return true;
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis SET error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<bool> Redis::setEx(const std::string& key, const std::string& value,
                                 std::chrono::seconds ttl) {
    try {
        auto redis = client();
        co_await redis->execCommandCoro("SETEX %s %d %s",
                                        key.c_str(),
                                        static_cast<int>(ttl.count()),
                                        value.c_str());
        co_return true;
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis SETEX error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<std::string> Redis::get(const std::string& key) {
    try {
        auto redis = client();
        auto result = co_await redis->execCommandCoro("GET %s", key.c_str());
        if (result.isNil()) {
            co_return "";
        }
        co_return result.asString();
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis GET error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<bool> Redis::del(const std::string& key) {
    try {
        auto redis = client();
        auto result = co_await redis->execCommandCoro("DEL %s", key.c_str());
        co_return result.asInteger() > 0;
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis DEL error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<bool> Redis::exists(const std::string& key) {
    try {
        auto redis = client();
        auto result = co_await redis->execCommandCoro("EXISTS %s", key.c_str());
        co_return result.asInteger() > 0;
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis EXISTS error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<bool> Redis::expire(const std::string& key, std::chrono::seconds ttl) {
    try {
        auto redis = client();
        auto result = co_await redis->execCommandCoro("EXPIRE %s %d",
                                                      key.c_str(),
                                                      static_cast<int>(ttl.count()));
        co_return result.asInteger() > 0;
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis EXPIRE error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<bool> Redis::hset(const std::string& key, const std::string& field,
                                const std::string& value) {
    try {
        auto redis = client();
        co_await redis->execCommandCoro("HSET %s %s %s",
                                        key.c_str(), field.c_str(), value.c_str());
        co_return true;
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis HSET error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<std::string> Redis::hget(const std::string& key, const std::string