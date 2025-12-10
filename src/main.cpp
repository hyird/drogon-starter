#include <drogon/drogon.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <fstream>
#include <filesystem>

#include "core/Response.hpp"
#include "core/Exception.hpp"
#include "core/Constants.hpp"
#include "middleware/JwtFilter.hpp"
#include "queue/MessageQueue.hpp"

// 初始化日志
void initLogger() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/app.log", 1024 * 1024 * 10, 5);  // 10MB, 保留 5 个文件
    file_sink->set_level(spdlog::level::info);

    auto logger = std::make_shared<spdlog::logger>("multi_sink",
        spdlog::sinks_init_list{console_sink, file_sink});

    logger->set_level(spdlog::level::debug);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

    spdlog::set_default_logger(logger);
    spdlog::info("Logger initialized");
}

// 初始化 JWT
void initJwt(const Json::Value& config) {
    auto secret = config.get("secret", core::constants::JWT_SECRET).asString();
    auto issuer = config.get("issuer", core::constants::JWT_ISSUER).asString();
    auto expireHours = config.get("expire_hours", 24).asInt();

    middleware::JwtUtil::setSecret(secret);
    middleware::JwtUtil::setIssuer(issuer);
    middleware::JwtUtil::setExpireDuration(std::chrono::hours(expireHours));
}

// 初始化消息队列
void initMessageQueue(const Json::Value& config) {
    auto consumerThreads = config.get("consumer_threads", 4).asInt();
    auto maxQueueSize = config.get("max_queue_size", 10000).asUInt();
    auto maxRetries = config.get("max_retries", 3).asInt();

    auto& mq = queue::MessageQueue::instance();
    mq.init(consumerThreads);
    mq.setMaxQueueSize(maxQueueSize);
    mq.setMaxRetries(maxRetries);

    // 注册示例消息处理器（协程版）
    mq.registerHandler("email", [](const queue::Message& msg) -> drogon::Task<bool> {
        spdlog::info("Processing email: to={}", msg.payload["to"].asString());
        // TODO: 实现邮件发送逻辑
        co_return true;
    });

    mq.registerHandler("notification", [](const queue::Message& msg) -> drogon::Task<bool> {
        spdlog::info("Processing notification: userId={}",
                     msg.payload["userId"].asString());
        // TODO: 实现通知逻辑
        co_return true;
    });

    // 启动消费者
    mq.startConsumers("tasks");
}

// 全局异常处理器
void setupExceptionHandler() {
    drogon::app().setExceptionHandler(
        [](const std::exception& e,
           const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& callback) {

            spdlog::error("Unhandled exception: {} - {}", req->getPath(), e.what());

            // 尝试转换为 AppException
            if (auto* appEx = dynamic_cast<const core::AppException*>(&e)) {
                callback(core::Response::fromException(*appEx));
            } else {
                callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
            }
        }
    );
}

// 自定义 404 处理
void setupNotFoundHandler() {
    drogon::app().setCustom404Page(
        core::Response::error(core::ErrorCode::RESOURCE_NOT_FOUND, "endpoint not found")
    );
}

// 加载自定义配置
Json::Value loadCustomConfig(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        spdlog::warn("Custom config not found: {}", path);
        return Json::Value();
    }

    Json::Value config;
    Json::CharReaderBuilder builder;
    std::string errors;

    if (!Json::parseFromStream(builder, file, &config, &errors)) {
        spdlog::error("Failed to parse config: {}", errors);
        return Json::Value();
    }

    return config;
}

int main(int argc, char* argv[]) {
    // 创建日志目录
    std::filesystem::create_directories("logs");

    // 初始化日志
    initLogger();
    spdlog::info("Starting Drogon Scaffold...");

    // 加载 Drogon 配置（包含 Redis 配置）
    drogon::app().loadConfigFile("config.json");

    // 加载自定义配置
    auto customConfig = loadCustomConfig("config.json");

    // 初始化组件
    try {
        if (customConfig.isMember("jwt")) {
            initJwt(customConfig["jwt"]);
        }

        if (customConfig.isMember("queue")) {
            initMessageQueue(customConfig["queue"]);
        } else {
            queue::MessageQueue::instance().init();
            queue::MessageQueue::instance().startConsumers("tasks");
        }

    } catch (const std::exception& e) {
        spdlog::error("Initialization failed: {}", e.what());
        return 1;
    }

    // 设置异常处理
    setupExceptionHandler();
    setupNotFoundHandler();

    // 注册启动回调
    drogon::app().registerBeginningAdvice([]() {
        spdlog::info("Server started");
    });

    // 启动服务器
    spdlog::info("Server listening on {}:{}",
                 drogon::app().getListeners()[0].toIp(),
                 drogon::app().getListeners()[0].toPort());

    drogon::app().run();

    // 服务器停止后清理资源
    spdlog::info("Server shutting down...");
    queue::MessageQueue::instance().shutdown();
    spdlog::info("Server stopped");

    return 0;
}