#include "UserLock.hpp"
#include "utils/Redis.hpp"
#include "utils/Crypto.hpp"
#include "core/Constants.hpp"
#include "core/Exception.hpp"
#include <spdlog/spdlog.h>

namespace lock {

// ==================== UserLock ====================

UserLock& UserLock::instance() {
    static UserLock instance;
    return instance;
}

void UserLock::setLockTimeout(std::chrono::seconds timeout) {
    lockTimeout_ = timeout;
}

void UserLock::setRetryInterval(std::chrono::milliseconds interval) {
    retryInterval_ = interval;
}

void UserLock::setMaxRetries(int retries) {
    maxRetries_ = retries;
}

std::string UserLock::buildKey(const std::string& userId) const {
    return std::string(core::constants::REDIS_USER_LOCK_PREFIX) + userId;
}

std::string UserLock::generateLockValue() const {
    return utils::Crypto::uuid();
}

drogon::Task<std::string> UserLock::tryLock(const std::string& userId) {
    const auto key = buildKey(userId);
    const auto value = generateLockValue();

    try {
        auto& redis = utils::Redis::instance();
        bool acquired = co_await redis.lock(key, value, lockTimeout_);

        if (acquired) {
            spdlog::debug("User lock acquired: userId={}", userId);
            co_return value;
        }
        co_return "";
    } catch (const core::RedisException& e) {
        spdlog::error("Failed to acquire user lock: {}", e.what());
        co_return "";
    }
}

drogon::Task<std::string> UserLock::lock(const std::string& userId) {
    for (int i = 0; i < maxRetries_; ++i) {
        auto lockValue = co_await tryLock(userId);
        if (!lockValue.empty()) {
            co_return lockValue;
        }

        // 等待后重试（使用 Drogon 的协程睡眠）
        co_await drogon::sleepCoro(
            drogon::app().getLoop(),
            std::chrono::duration_cast<std::chrono::duration<double>>(retryInterval_)
        );
    }

    spdlog::warn("User lock timeout: userId={}, retries={}", userId, maxRetries_);
    co_return "";
}

drogon::Task<bool> UserLock::unlock(const std::string& userId, const std::string& lockValue) {
    if (lockValue.empty()) {
        co_return false;
    }

    const auto key = buildKey(userId);

    try {
        auto& redis = utils::Redis::instance();
        bool released = co_await redis.unlock(key, lockValue);
        if (released) {
            spdlog::debug("User lock released: userId={}", userId);
        }
        co_return released;
    } catch (const core::RedisException& e) {
        spdlog::error("Failed to release user lock: {}", e.what());
        co_return false;
    }
}

// ==================== UserLockGuard ====================

UserLockGuard::UserLockGuard(std::string userId, std::string lockValue)
    : userId_(std::move(userId))
    , lockValue_(std::move(lockValue)) {
}

UserLockGuard::~UserLockGuard() {
    // 注意：析构函数中不能使用协程
    // 如果需要在协程中自动释放，请手动调用 release()
    if (!lockValue_.empty()) {
        spdlog::warn("UserLockGuard destroyed without release, userId={}", userId_);
    }
}

UserLockGuard::UserLockGuard(UserLockGuard&& other) noexcept
    : userId_(std::move(other.userId_))
    , lockValue_(std::move(other.lockValue_)) {
    other.lockValue_.clear();
}

UserLockGuard& UserLockGuard::operator=(UserLockGuard&& other) noexcept {
    if (this != &other) {
        userId_ = std::move(other.userId_);
        lockValue_ = std::move(other.lockValue_);
        other.lockValue_.clear();
    }
    return *this;
}

drogon::Task<void> UserLockGuard::release() {
    if (!lockValue_.empty()) {
        co_await UserLock::instance().unlock(userId_, lockValue_);
        lockValue_.clear();
    }
}

} // namespace lock