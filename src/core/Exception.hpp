#pragma once

#include <stdexcept>
#include <string>
#include <utility>
#include "Error.hpp"

namespace core {

// 业务异常基类
class AppException : public std::runtime_error {
public:
    AppException(ErrorCode code, std::string detail = "")
        : std::runtime_error(getErrorMessage(code))
        , code_(code)
        , detail_(std::move(detail)) {}

    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] const std::string& detail() const noexcept { return detail_; }

    [[nodiscard]] std::string fullMessage() const {
        if (detail_.empty()) {
            return what();
        }
        return std::string(what()) + ": " + detail_;
    }

private:
    ErrorCode code_;
    std::string detail_;
};

// 认证异常
class AuthException : public AppException {
public:
    explicit AuthException(ErrorCode code = ErrorCode::AUTH_FAILED, 
                          const std::string& detail = "")
        : AppException(code, detail) {}
};

// 参数异常
class ParamException : public AppException {
public:
    explicit ParamException(const std::string& detail = "")
        : AppException(ErrorCode::INVALID_PARAMS, detail) {}
};

// 资源未找到异常
class NotFoundException : public AppException {
public:
    explicit NotFoundException(const std::string& detail = "")
        : AppException(ErrorCode::RESOURCE_NOT_FOUND, detail) {}
};

// 数据库异常
class DbException : public AppException {
public:
    explicit DbException(ErrorCode code = ErrorCode::DB_QUERY_ERROR,
                        const std::string& detail = "")
        : AppException(code, detail) {}
};

// Redis 异常
class RedisException : public AppException {
public:
    explicit RedisException(ErrorCode code = ErrorCode::REDIS_OPERATION_ERROR,
                           const std::string& detail = "")
        : AppException(code, detail) {}
};

// 队列异常
class QueueException : public AppException {
public:
    explicit QueueException(ErrorCode code = ErrorCode::QUEUE_TIMEOUT,
                           const std::string& detail = "")
        : AppException(code, detail) {}
};

} // namespace core