#include "AuthController.hpp"
#include "services/AuthService.hpp"
#include "middleware/JwtFilter.hpp"
#include "core/Response.hpp"
#include "core/Exception.hpp"
#include "lock/UserLock.hpp"
#include <spdlog/spdlog.h>

namespace controllers {

drogon::Task<> AuthController::registerUser(drogon::HttpRequestPtr req,
                                             std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(core::Response::error(core::ErrorCode::INVALID_PARAMS, "invalid JSON"));
            co_return;
        }

        auto username = (*json)["username"].asString();
        auto password = (*json)["password"].asString();
        auto email = (*json)["email"].asString();

        auto& authService = services::AuthService::instance();
        auto result = co_await authService.registerUser(username, password, email);

        Json::Value data;
        data["userId"] = result.userId;
        data["username"] = result.username;

        callback(core::Response::success(data));

    } catch (const core::AppException& e) {
        callback(core::Response::fromException(e));
    } catch (const std::exception& e) {
        spdlog::error("Register error: {}", e.what());
        callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

drogon::Task<> AuthController::login(drogon::HttpRequestPtr req,
                                      std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        auto json = req->getJsonObject();
        if (!json) {
            callback(core::Response::error(core::ErrorCode::INVALID_PARAMS, "invalid JSON"));
            co_return;
        }

        auto username = (*json)["username"].asString();
        auto password = (*json)["password"].asString();

        auto& authService = services::AuthService::instance();
        auto result = co_await authService.login(username, password);

        Json::Value data;
        data["userId"] = result.userId;
        data["username"] = result.username;
        data["token"] = result.token;
        data["expiresAt"] = result.expiresAt;

        callback(core::Response::success(data));

    } catch (const core::AppException& e) {
        callback(core::Response::fromException(e));
    } catch (const std::exception& e) {
        spdlog::error("Login error: {}", e.what());
        callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

drogon::Task<> AuthController::logout(drogon::HttpRequestPtr req,
                                       std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        auto userId = req->getAttributes()->get<std::string>("userId");
        auto token = middleware::JwtUtil::extractToken(req).value_or("");

        // 用户锁：串行处理
        auto lockValue = co_await lock::UserLock::instance().lock(userId);
        if (lockValue.empty()) {
            callback(core::Response::error(core::ErrorCode::RATE_LIMIT_EXCEEDED));
            co_return;
        }

        // 确保锁释放
        auto guard = lock::UserLockGuard(userId, lockValue);

        auto& authService = services::AuthService::instance();
        co_await authService.logout(userId, token);

        co_await guard.release();
        callback(core::Response::successMsg("logged out"));

    } catch (const core::AppException& e) {
        callback(core::Response::fromException(e));
    } catch (const std::exception& e) {
        spdlog::error("Logout error: {}", e.what());
        callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

drogon::Task<> AuthController::refresh(drogon::HttpRequestPtr req,
                                        std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        auto userId = req->getAttributes()->get<std::string>("userId");

        // 用户锁：串行处理
        auto lockValue = co_await lock::UserLock::instance().lock(userId);
        if (lockValue.empty()) {
            callback(core::Response::error(core::ErrorCode::RATE_LIMIT_EXCEEDED));
            co_return;
        }

        auto guard = lock::UserLockGuard(userId, lockValue);

        auto& authService = services::AuthService::instance();
        auto result = co_await authService.refreshToken(userId);

        Json::Value data;
        data["userId"] = result.userId;
        data["username"] = result.username;
        data["token"] = result.token;
        data["expiresAt"] = result.expiresAt;

        co_await guard.release();
        callback(core::Response::success(data));

    } catch (const core::AppException& e) {
        callback(core::Response::fromException(e));
    } catch (const std::exception& e) {
        spdlog::error("Refresh token error: {}", e.what());
        callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

drogon::Task<> AuthController::changePassword(drogon::HttpRequestPtr req,
                                               std::function<void(const drogon::HttpResponsePtr&)> callback) {
    try {
        auto userId = req->getAttributes()->get<std::string>("userId");

        auto json = req->getJsonObject();
        if (!json) {
            callback(core::Response::error(core::ErrorCode::INVALID_PARAMS, "invalid JSON"));
            co_return;
        }

        auto oldPassword = (*json)["oldPassword"].asString();
        auto newPassword = (*json)["newPassword"].asString();

        // 用户锁：串行处理
        auto lockValue = co_await lock::UserLock::instance().lock(userId);
        if (lockValue.empty()) {
            callback(core::Response::error(core::ErrorCode::RATE_LIMIT_EXCEEDED));
            co_return;
        }

        auto guard = lock::UserLockGuard(userId, lockValue);

        auto& authService = services::AuthService::instance();
        co_await authService.changePassword(userId, oldPassword, newPassword);

        co_await guard.release();
        callback(core::Response::successMsg("password changed"));

    } catch (const core::AppException& e) {
        callback(core::Response::fromException(e));
    } catch (const std::exception& e) {
        spdlog::error("Change password error: {}", e.what());
        callback(core::Response::error(core::ErrorCode::UNKNOWN_ERROR, e.what()));
    }
}

} // namespace controllers