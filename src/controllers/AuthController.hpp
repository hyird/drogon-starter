#pragma once

#include <drogon/HttpController.h>

namespace controllers {

    class AuthController : public drogon::HttpController<AuthController> {
    public:
        METHOD_LIST_BEGIN
            // 注册
            ADD_METHOD_TO(AuthController::registerUser, "/api/auth/register", drogon::Post);
        // 登录
        ADD_METHOD_TO(AuthController::login, "/api/auth/login", drogon::Post);
        // 登出（需要 JWT）
        ADD_METHOD_TO(AuthController::logout, "/api/auth/logout", drogon::Post,
                      "middleware::JwtFilter", "middleware::LogFilter");
        // 刷新 Token（需要 JWT）
        ADD_METHOD_TO(AuthController::refresh, "/api/auth/refresh", drogon::Post,
                      "middleware::JwtFilter", "middleware::LogFilter");
        // 修改密码（需要 JWT）
        ADD_METHOD_TO(AuthController::changePassword, "/api/auth/password", drogon::Put,
                      "middleware::JwtFilter", "middleware::LogFilter");
        METHOD_LIST_END

        // 注册
        drogon::Task<> registerUser(drogon::HttpRequestPtr req,
                                     std::function<void(const drogon::HttpResponsePtr&)> callback);

        // 登录
        drogon::Task<> login(drogon::HttpRequestPtr req,
                              std::function<void(const drogon::HttpResponsePtr&)> callback);

        // 登出
        drogon::Task<> logout(drogon::HttpRequestPtr req,
                               std::function<void(const drogon::HttpResponsePtr&)> callback);

        // 刷新 Token
        drogon::Task<> refresh(drogon::HttpRequestPtr req,
                                std::function<void(const drogon::HttpResponsePtr&)> callback);

        // 修改密码
        drogon::Task<> changePassword(drogon::HttpRequestPtr req,
                                       std::function<void(const drogon::HttpResponsePtr&)> callback);
    };

} // namespace controllers