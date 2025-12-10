#pragma once

#include <string>
#include <unordered_map>

namespace core {

// 错误码枚举
enum class ErrorCode : int {
    // 成功
    SUCCESS = 0,

    // 通用错误 1xxx
    UNKNOWN_ERROR = 1000,
    INVALID_PARAMS = 1001,
    RESOURCE_NOT_FOUND = 1002,
    OPERATION_FAILED = 1003,
    RATE_LIMIT_EXCEEDED = 1004,

    // 认证错误 2xxx
    AUTH_FAILED = 2000,
    TOKEN_INVALID = 2001,
    TOKEN_EXPIRED = 2002,
    TOKEN_MISSING = 2003,
    PERMISSION_DENIED = 2004,

    // 用户错误 3xxx
    USER_NOT_FOUND = 3000,
    USER_ALREADY_EXISTS = 3001,
    USER_DISABLED = 3002,
    PASSWORD_INCORRECT = 3003,

    // 数据库错误 4xxx
    DB_CONNECTION_ERROR = 4000,
    DB_QUERY_ERROR = 4001,
    DB_TRANSACTION_ERROR = 4002,

    // Redis 错误 5xxx
    REDIS_CONNECTION_ERROR = 5000,
    REDIS_OPERATION_ERROR = 5001,

    // 队列错误 6xxx
    QUEUE_FULL = 6000,
    QUEUE_TIMEOUT = 6001,
};

// 错误码对应的消息
inline const std::unordered_map<ErrorCode, std::string>& getErrorMessages() {
    static const std::unordered_map<ErrorCode, std::string> messages = {
        {ErrorCode::SUCCESS, "success"},
        
        // 通用错误
        {ErrorCode::UNKNOWN_ERROR, "unknown error"},
        {ErrorCode::INVALID_PARAMS, "invalid parameters"},
        {ErrorCode::RESOURCE_NOT_FOUND, "resource not found"},
        {ErrorCode::OPERATION_FAILED, "operation failed"},
        {ErrorCode::RATE_LIMIT_EXCEEDED, "rate limit exceeded"},

        // 认证错误
        {ErrorCode::AUTH_FAILED, "authentication failed"},
        {ErrorCode::TOKEN_INVALID, "invalid token"},
        {ErrorCode::TOKEN_EXPIRED, "token expired"},
        {ErrorCode::TOKEN_MISSING, "token missing"},
        {ErrorCode::PERMISSION_DENIED, "permission denied"},

        // 用户错误
        {ErrorCode::USER_NOT_FOUND, "user not found"},
        {ErrorCode::USER_ALREADY_EXISTS, "user already exists"},
        {ErrorCode::USER_DISABLED, "user disabled"},
        {ErrorCode::PASSWORD_INCORRECT, "password incorrect"},

        // 数据库错误
        {ErrorCode::DB_CONNECTION_ERROR, "database connection error"},
        {ErrorCode::DB_QUERY_ERROR, "database query error"},
        {ErrorCode::DB_TRANSACTION_ERROR, "database transaction error"},

        // Redis 错误
        {ErrorCode::REDIS_CONNECTION_ERROR, "redis connection error"},
        {ErrorCode::REDIS_OPERATION_ERROR, "redis operation error"},

        // 队列错误
        {ErrorCode::QUEUE_FULL, "queue is full"},
        {ErrorCode::QUEUE_TIMEOUT, "queue operation timeout"},
    };
    return messages;
}

// 获取错误消息
inline std::string getErrorMessage(ErrorCode code) {
    const auto& messages = getErrorMessages();
    if (auto it = messages.find(code); it != messages.end()) {
        return it->second;
    }
    return "unknown error";
}

// 错误码转 int
inline int toInt(ErrorCode code) {
    return static_cast<int>(code);
}

}