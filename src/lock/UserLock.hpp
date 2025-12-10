#pragma once

#include <drogon/drogon.h>
#include <string>
#include <chrono>

namespace lock {

    // 用户锁管理器（协程版）
    class UserLock {
    public:
        // 单例获取
        static UserLock& instance();

        // 配置
        void setLockTimeout(std::chrono::seconds timeout);
        void setRetryInterval(std::chrono::milliseconds interval);
        void setMaxRetries(int retries);

        // 尝试获取锁（协程，非阻塞）
        drogon::Task<std::string> tryLock(const std::string& userId);

        // 获取锁（协程，阻塞带重试）
        drogon::Task<std::string> lock(const std::string& userId);

        // 释放锁（协程）
        drogon::Task<bool> unlock(const std::string& userId, const std::string& lockValue);

    private:
        UserLock() = default;
        ~UserLock() = default;
        UserLock(const UserLock&) = delete;
        UserLock& operator=(const UserLock&) = delete;

        std::string buildKey(const std::string& userId) const;
        std::string generateLockValue() const;

        std::chrono::seconds lockTimeout_{60};
        std::chrono::milliseconds retryInterval_{100};
        int maxRetries_{50};
    };

    // RAII 锁守卫（用于协程中自动释放）
    class UserLockGuard {
    public:
        UserLockGuard(std::string userId, std::string lockValue);
        ~UserLockGuard();

        // 禁止拷贝
        UserLockGuard(const UserLockGuard&) = delete;
        UserLockGuard& operator=(const UserLockGuard&) = delete;

        // 允许移动
        UserLockGuard(UserLockGuard&& other) noexcept;
        UserLockGuard& operator=(UserLockGuard&& other) noexcept;

        [[nodiscard]] bool isLocked() const noexcept { return !lockValue_.empty(); }
        [[nodiscard]] const std::string& lockValue() const noexcept { return lockValue_; }

        // 手动释放（用于协程环境）
        drogon::Task<void> release();

    private:
        std::string userId_;
        std::string lockValue_;
    };

} // namespace lock