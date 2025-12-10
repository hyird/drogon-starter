#pragma once

#include <drogon/drogon.h>
#include <drogon/nosql/RedisClient.h>
#include <string>
#include <chrono>

namespace utils {

    // Redis 封装（基于 Drogon 自带的 Redis 客户端）
    class Redis {
    public:
        // 单例获取
        static Redis& instance();

        // 获取 Drogon Redis 客户端
        drogon::nosql::RedisClientPtr client();

        // ============ 协程方法 ============

        // 基础操作
        drogon::Task<bool> set(const std::string& key, const std::string& value);
        drogon::Task<bool> setEx(const std::string& key, const std::string& value,
                                  std::chrono::seconds ttl);
        drogon::Task<std::string> get(const std::string& key);
        drogon::Task<bool> del(const std::string& key);
        drogon::Task<bool> exists(const std::string& key);
        drogon::Task<bool> expire(const std::string& key, std::chrono::seconds ttl);

        // Hash 操作
        drogon::Task<bool> hset(const std::string& key, const std::string& field,
                                const std::string& value);
        drogon::Task<std::string> hget(const std::string& key, const std::string& field);
        drogon::Task<bool> hdel(const std::string& key, const std::string& field);

        // List 操作（用于消息队列）
        drogon::Task<int64_t> lpush(const std::string& key, const std::string& value);
        drogon::Task<std::string> rpop(const std::string& key);
        drogon::Task<std::string> brpop(const std::string& key, std::chrono::seconds timeout);
        drogon::Task<int64_t> llen(const std::string& key);

        // 分布式锁
        drogon::Task<bool> lock(const std::string& key, const std::string& value,
                                std::chrono::seconds ttl);
        drogon::Task<bool> unlock(const std::string& key, const std::string& value);

        // 原子递增
        drogon::Task<int64_t> incr(const std::string& key);

    private:
        Redis() = default;
        ~Redis() = default;
        Redis(const Redis&) = delete;
        Redis& operator=(const Redis&) = delete;
    };

} // namespace utils