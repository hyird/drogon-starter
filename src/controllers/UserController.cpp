#include "UserController.hpp"
#include "services/UserService.hpp"
#include "core/Response.hpp"
#include "core/Exception.hpp"
#include "core/Constants.hpp"
#include "lock/UserLock.hpp"
#include <spdlog/spdlog.h>

namespace controllers {

drogon::Task<> UserController::getCurrentUser(drogon::HttpRequestPtr req,
                                               std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        auto userIdStr = req->getAttributes()->get<std::string>("userId");
        int64_t userId = std::stoll(userIdStr);

        auto& userService = services::UserService::instance();
        auto user = co_await userService.getUserById(userId);

        callback(core::Response::success(user.toJsonForApi()));

    } catch (const core::AppException& e) {
        callback(core::Response::fromException(e));
    } catch (const std::exception& e) {
        spdlog::error("Get current user error: {}", e.what());
        callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

drogon::Task<> UserController::listUsers(drogon::HttpRequestPtr req,
                                          std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        // 从查询参数获取分页信息
        auto pageStr = req->getParameter("page");
        auto pageSizeStr = req->getParameter("pageSize");
        auto keyword = req->getParameter("keyword");

        int page = pageStr.empty() ? core::constants::DEFAULT_PAGE : std::stoi(pageStr);
        int pageSize = pageSizeStr.empty() ? core::constants::DEFAULT_PAGE_SIZE : std::stoi(pageSizeStr);

        auto& userService = services::UserService::instance();
        auto result = co_await userService.listUsers(page, pageSize, keyword);

        // 构建列表 JSON
        Json::Value list(Json::arrayValue);
        for (const auto& user : result.list) {
            list.append(user.toJsonForApi());
        }

        callback(core::Response::page(list, result.total, result.page, result.pageSize));

    } catch (const core::AppException& e) {
        callback(core::Response::fromException(e));
    } catch (const std::exception& e) {
        spdlog::error("List users error: {}", e.what());
        callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

drogon::Task<> UserController::getUserById(drogon::HttpRequestPtr req,
                                            std::function<void(const drogon::HttpResponsePtr&)> callback,
                                            int64_t id) {
    try {
        auto& userService = services::UserService::instance();
        auto user = co_await userService.getUserById(id);

        callback(core::Response::success(user.toJsonForApi()));

    } catch (const core::AppException& e) {
        callback(core::Response::fromException(e));
    } catch (const std::exception& e) {
        spdlog::error("Get user error: {}", e.what());
        callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

drogon::Task<> UserController::updateUser(drogon::HttpRequestPtr req,
                                           std::function<void(const drogon::HttpResponsePtr&)> callback,
                                           int64_t id) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(core::Response::error(core::ErrorCode::INVALID_PARAMS, "invalid JSON"));
            co_return;
        }

        // 用户锁：串行处理
        auto lockKey = std::to_string(id);
        auto lockValue = co_await lock::UserLock::instance().lock(lockKey);
        if (lockValue.empty()) {
            callback(core::Response::error(core::ErrorCode::RATE_LIMIT_EXCEEDED));
            co_return;
        }

        auto guard = lock::UserLockGuard(lockKey, lockValue);

        std::optional<std::string> email;
        std::optional<std::string> role;

        if (json->isMember("email")) {
            email = (*json)["email"].asString();
        }
        if (json->isMember("role")) {
            role = (*json)["role"].asString();
        }

        auto& userService = services::UserService::instance();
        co_await userService.updateUser(id, email, role);

        co_await guard.release();
        callback(core::Response::successMsg("user updated"));

    } catch (const core::AppException& e) {
        callback(core::Response::fromException(e));
    } catch (const std::exception& e) {
        spdlog::error("Update user error: {}", e.what());
        callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

drogon::Task<> UserController::setUserStatus(drogon::HttpRequestPtr req,
                                              std::function<void(const drogon::HttpResponsePtr&)> callback,
                                              int64_t id) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(core::Response::error(core::ErrorCode::INVALID_PARAMS, "invalid JSON"));
            co_return;
        }

        int status = (*json)["status"].asInt();

        // 用户锁：串行处理
        auto lockKey = std::to_string(id);
        auto lockValue = co_await lock::UserLock::instance().lock(lockKey);
        if (lockValue.empty()) {
            callback(core::Response::error(core::ErrorCode::RATE_LIMIT_EXCEEDED));
            co_return;
        }

        auto guard = lock::UserLockGuard(lockKey, lockValue);

        auto& userService = services::UserService::instance();
        co_await userService.setUserStatus(id, status);

        co_await guard.release();
        callback(core::Response::successMsg("status updated"));

    } catch (const core::AppException& e) {
        callback(core::Response::fromException(e));
    } catch (const std::exception& e) {
        spdlog::error("Set user status error: {}", e.what());
        callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

drogon::Task<> UserController::deleteUser(drogon::HttpRequestPtr req,
                                           std::function<void(const drogon::HttpResponsePtr&)> callback,
                                           int64_t id) {
    try {
        // 用户锁：串行处理
        auto lockKey = std::to_string(id);
        auto lockValue = co_await lock::UserLock::instance().lock(lockKey);
        if (lockValue.empty()) {
            callback(core::Response::error(core::ErrorCode::RATE_LIMIT_EXCEEDED));
            co_return;
        }

        auto guard = lock::UserLockGuard(lockKey, lockValue);

        auto& userService = services::UserService::instance();
        co_await userService.deleteUser(id);

        co_await guard.release();
        callback(core::Response::successMsg("user deleted"));

    } catch (const core::AppException& e) {
        callback(core::Response::fromException(e));
    } catch (const std::exception& e) {
        spdlog::error("Delete user error: {}", e.what());
        callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

} // namespace controllers