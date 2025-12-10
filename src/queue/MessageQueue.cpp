#include "MessageQueue.hpp"
#include "utils/Redis.hpp"
#include "utils/Crypto.hpp"
#include "core/Constants.hpp"
#include "core/Exception.hpp"
#include <spdlog/spdlog.h>

namespace queue {

// ==================== Message ====================

std::string Message::serialize() const {
    Json::Value json;
    json["id"] = id;
    json["type"] = type;
    json["payload"] = payload;
    json["timestamp"] = timestamp;
    json["retryCount"] = retryCount;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, json);
}

Message Message::deserialize(const std::string& data) {
    Json::Value json;
    Json::CharReaderBuilder builder;
    std::istringstream stream(data);
    std::string errors;

    if (!Json::parseFromStream(builder, stream, &json, &errors)) {
        throw core::AppException(core::ErrorCode::INVALID_PARAMS, 
                                 "Invalid message format: " + errors);
    }

    Message msg;
    msg.id = json["id"].asString();
    msg.type = json["type"].asString();
    msg.payload = json["payload"];
    msg.timestamp = json["timestamp"].asInt64();
    msg.retryCount = json["retryCount"].asInt();
    return msg;
}

// ==================== MessageQueue ====================

MessageQueue& MessageQueue::instance() {
    static MessageQueue instance;
    return instance;
}

void MessageQueue::init(int consumerCount) {
    consumerCount_ = consumerCount;
    spdlog::info("MessageQueue initialized with {} consumers", consumerCount_);
}

void MessageQueue::shutdown() {
    stopConsumers();
    handlers_.clear();
    spdlog::info("MessageQueue shutdown");
}

void MessageQueue::registerHandler(const std::string& messageType, MessageHandler handler) {
    handlers_[messageType] = std::move(handler);
    spdlog::info("Registered handler for message type: {}", messageType);
}

std::string MessageQueue::buildKey(const std::string& queueName) const {
    return std::string(core::constants::REDIS_QUEUE_PREFIX) + queueName;
}

drogon::Task<bool> MessageQueue::publish(const std::string& queueName, const Message& message) {
    bool full = co_await isFull(queueName);
    if (full) {
        spdlog::warn("Queue is full: {}", queueName);
        throw core::QueueException(core::ErrorCode::QUEUE_FULL);
    }

    try {
        auto& redis = utils::Redis::instance();
        const auto key = buildKey(queueName);
        co_await redis.lpush(key, message.serialize());

        spdlog::debug("Message published: queue={}, id={}, type={}",
                      queueName, message.id, message.type);
        co_return true;
    } catch (const core::RedisException& e) {
        spdlog::error("Failed to publish message: {}", e.what());
        co_return false;
    }
}

drogon::Task<bool> MessageQueue::publish(const std::string& queueName,
                                          const std::string& type,
                                          const Json::Value& payload) {
    Message msg;
    msg.id = utils::Crypto::uuid();
    msg.type = type;
    msg.payload = payload;
    msg.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    msg.retryCount = 0;

    co_return co_await publish(queueName, msg);
}

void MessageQueue::startConsumers(const std::string& queueName) {
    if (running_) {
        spdlog::warn("Consumers already running");
        return;
    }

    running_ = true;

    // 启动多个消费者协程
    for (int i = 0; i < consumerCount_; ++i) {
        // 使用 async_run 启动协程
        drogon::async_run([this, queueName]() -> drogon::Task<void> {
            co_await consumerTask(queueName);
        });
    }

    spdlog::info("Started {} consumers for queue: {}", consumerCount_, queueName);
}

void MessageQueue::stopConsumers() {
    running_ = false;
    spdlog::info("Consumers stop signal sent");
}

drogon::Task<void> MessageQueue::consumerTask(const std::string& queueName) {
    const auto key = buildKey(queueName);
    auto& redis = utils::Redis::instance();

    spdlog::info("Consumer started for queue: {}", queueName);

    while (running_) {
        bool shouldSleep = false;

        try {
            // 阻塞式获取消息，超时 1 秒
            auto data = co_await redis.brpop(key, std::chrono::seconds(1));

            if (data.empty()) {
                continue;  // 超时，继续循环
            }

            try {
                auto message = Message::deserialize(data);

                bool success = co_await processMessage(message);
                if (!success) {
                    // 处理失败，加入重试
                    co_await retryMessage(queueName, std::move(message));
                }
            } catch (const std::exception& e) {
                spdlog::error("Failed to process message: {}", e.what());
            }

        } catch (const core::RedisException& e) {
            spdlog::error("Consumer Redis error: {}", e.what());
            shouldSleep = true;
        }

        // 在 catch 块外执行 sleep
        if (shouldSleep) {
            co_await drogon::sleepCoro(drogon::app().getLoop(), 1.0);
        }
    }

    spdlog::info("Consumer stopped for queue: {}", queueName);
}

drogon::Task<bool> MessageQueue::processMessage(const Message& message) {
    auto it = handlers_.find(message.type);
    if (it == handlers_.end()) {
        spdlog::warn("No handler for message type: {}", message.type);
        co_return true;  // 无处理器，视为成功（丢弃消息）
    }

    try {
        spdlog::debug("Processing message: id={}, type={}", message.id, message.type);
        bool success = co_await it->second(message);

        if (success) {
            spdlog::debug("Message processed successfully: id={}", message.id);
        } else {
            spdlog::warn("Message processing returned false: id={}", message.id);
        }

        co_return success;
    } catch (const std::exception& e) {
        spdlog::error("Message handler exception: id={}, error={}", message.id, e.what());
        co_return false;
    }
}

drogon::Task<void> MessageQueue::retryMessage(const std::string& queueName, Message message) {
    message.retryCount++;

    if (message.retryCount > maxRetries_) {
        spdlog::error("Message exceeded max retries, discarding: id={}, type={}",
                      message.id, message.type);
        co_return;
    }

    spdlog::info("Retrying message: id={}, attempt={}/{}",
                 message.id, message.retryCount, maxRetries_);

    // 延迟重试
    co_await drogon::sleepCoro(drogon::app().getLoop(), 5.0);

    try {
        auto& redis = utils::Redis::instance();
        const auto key = buildKey(queueName);
        co_await redis.lpush(key, message.serialize());
    } catch (const core::RedisException& e) {
        spdlog::error("Failed to retry message: {}", e.what());
    }
}

drogon::Task<int64_t> MessageQueue::queueLength(const std::string& queueName) {
    try {
        auto& redis = utils::Redis::instance();
        co_return co_await redis.llen(buildKey(queueName));
    } catch (const core::RedisException& e) {
        spdlog::error("Failed to get queue length: {}", e.what());
        co_return -1;
    }
}

drogon::Task<bool> MessageQueue::isFull(const std::string& queueName) {
    auto len = co_await queueLength(queueName);
    co_return len >= 0 && static_cast<size_t>(len) >= maxQueueSize_;
}

void MessageQueue::setMaxQueueSize(size_t size) {
    maxQueueSize_ = size;
}

void MessageQueue::setMaxRetries(int retries) {
    maxRetries_ = retries;
}

} // namespace queue