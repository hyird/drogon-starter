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

drogon::Task<std::string> Redis::hget(const std::string& key, const std::string& field) {
    try {
        auto redis = client();
        auto result = co_await redis->execCommandCoro("HGET %s %s",
                                                      key.c_str(), field.c_str());
        if (result.isNil()) {
            co_return "";
        }
        co_return result.asString();
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis HGET error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<bool> Redis::hdel(const std::string& key, const std::string& field) {
    try {
        auto redis = client();
        auto result = co_await redis->execCommandCoro("HDEL %s %s",
                                                      key.c_str(), field.c_str());
        co_return result.asInteger() > 0;
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis HDEL error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<int64_t> Redis::lpush(const std::string& key, const std::string& value) {
    try {
        auto redis = client();
        auto result = co_await redis->execCommandCoro("LPUSH %s %s",
                                                      key.c_str(), value.c_str());
        co_return result.asInteger();
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis LPUSH error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<std::string> Redis::rpop(const std::string& key) {
    try {
        auto redis = client();
        auto result = co_await redis->execCommandCoro("RPOP %s", key.c_str());
        if (result.isNil()) {
            co_return "";
        }
        co_return result.asString();
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis RPOP error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<std::string> Redis::brpop(const std::string& key, std::chrono::seconds timeout) {
    try {
        auto redis = client();
        auto result = co_await redis->execCommandCoro("BRPOP %s %d",
                                                      key.c_str(),
                                                      static_cast<int>(timeout.count()));
        if (result.isNil() || result.type() != drogon::nosql::RedisResultType::kArray) {
            co_return "";
        }
        // BRPOP 返回 [key, value]
        auto arr = result.asArray();
        if (arr.size() < 2) {
            co_return "";
        }
        co_return arr[1].asString();
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis BRPOP error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<int64_t> Redis::llen(const std::string& key) {
    try {
        auto redis = client();
        auto result = co_await redis->execCommandCoro("LLEN %s", key.c_str());
        co_return result.asInteger();
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis LLEN error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<bool> Redis::lock(const std::string& key, const std::string& value,
                                std::chrono::seconds ttl) {
    try {
        auto redis = client();
        // SET key value NX EX ttl
        auto result = co_await redis->execCommandCoro("SET %s %s NX EX %d",
                                                      key.c_str(),
                                                      value.c_str(),
                                                      static_cast<int>(ttl.count()));
        co_return !result.isNil();
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis LOCK error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<bool> Redis::unlock(const std::string& key, const std::string& value) {
    try {
        auto redis = client();
        // Lua 脚本保证原子性
        const char* script = R"(
            if redis.call('get', KEYS[1]) == ARGV[1] then
                return redis.call('del', KEYS[1])
            else
                return 0
            end
        )";
        auto result = co_await redis->execCommandCoro("EVAL %s 1 %s %s",
                                                      script,
                                                      key.c_str(),
                                                      value.c_str());
        co_return result.asInteger() > 0;
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis UNLOCK error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

drogon::Task<int64_t> Redis::incr(const std::string& key) {
    try {
        auto redis = client();
        auto result = co_await redis->execCommandCoro("INCR %s", key.c_str());
        co_return result.asInteger();
    } catch (const drogon::nosql::RedisException& e) {
        spdlog::error("Redis INCR error: {}", e.what());
        throw core::RedisException(core::ErrorCode::REDIS_OPERATION_ERROR, e.what());
    }
}

} // namespace utils