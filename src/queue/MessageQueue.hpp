#pragma once

#include <drogon/drogon.h>
#include <string>
#include <functional>
#include <atomic>
#include <unordered_map>
#include <json/json.h>

namespace queue {

// 消息结构
struct Message {
    std::string id;           // 消息ID
    std::string type;         // 消息类型
    Json::Value payload;      // 消息内容
    int64_t timestamp;        // 时间戳
    int retryCount = 0;       // 重试次数

    // 序列化
    [[nodiscard]] std::string serialize() const;

    // 反序列化
    static Message deserialize(const std::string& data);
};

// 消息处理器类型（协程）
using MessageHandler = std::function<drogon::Task<bool>(const Message&)>;

// 消息队列（基于 Drogon Redis，协程版）
class MessageQueue {
public:
    // 单例获取
    static MessageQueue& instance();

    // 初始化
    void init(int consumerCount = 4);

    // 关闭
    void shutdown();

    // 注册消息处理器
    void registerHandler(const std::string& messageType, MessageHandler handler);

    // 生产者：发送消息（协程）
    drogon::Task<bool> publish(const std::string& queueName, const Message& message);

    // 便捷方法：发送消息（协程）
    drogon::Task<bool> publish(const std::string& queueName,
                               const std::string& type,
                               const Json::Value& payload);

    // 启动消费者
    void startConsumers(const std::string& queueName);

    // 停止消费者
    void stopConsumers();

    // 获取队列长度（协程）
    drogon::Task<int64_t> queueLength(const std::string& queueName);

    // 队列是否已满（协程）
    drogon::Task<bool> isFull(const std::string& queueName);

    // 配置
    void setMaxQueueSize(size_t size);
    void setMaxRetries(int retries);

private:
    MessageQueue() = default;
    ~MessageQueue() = default;
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;

    // 消费者协程
    drogon::Task<void> consumerTask(const std::string& queueName);

    // 处理单条消息
    drogon::Task<bool> processMessage(const Message& message);

    // 重试消息
    drogon::Task<void> retryMessage(const std::string& queueName, Message message);

    // 构建 Redis key
    [[nodiscard]] std::string buildKey(const std::string& queueName) const;

    std::unordered_map<std::string, MessageHandler> handlers_;
    std::atomic<bool> running_{false};

    size_t maxQueueSize_ = 10000;
    int maxRetries_ = 3;
    int consumerCount_ = 4;
};

} // namespace queue