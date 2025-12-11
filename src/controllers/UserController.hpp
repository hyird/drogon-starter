#pragma once

#include <drogon/HttpController.h>

namespace controllers {

class UserController : public drogon::HttpController<UserController> {
public:
    METHOD_LIST_BEGIN
        // 获取当前用户信息
        ADD_METHOD_TO(UserController::getCurrentUser, "/api/user/me", drogon::Get,
                      "middleware::JwtFilter", "middleware::LogFilter");
        // 获取用户列表（管理员）
        ADD_METHOD_TO(UserController::listUsers, "/api/user/list", drogon::Get,
                      "middleware::JwtFilter", "middleware::LogFilter");
        // 获取指定用户
        ADD_METHOD_TO(UserController::getUserById, "/api/user/{id}", drogon::Get,
                      "middleware::JwtFilter", "middleware::LogFilter");
        // 更新用户
        ADD_METHOD_TO(UserController::updateUser, "/api/user/{id}", drogon::Put,
                      "middleware::JwtFilter", "middleware::LogFilter");
        // 禁用/启用用户
        ADD_METHOD_TO(UserController::setUserStatus, "/api/user/{id}/status", drogon::Put,
                      "middleware::JwtFilter", "middleware::LogFilter");
        // 删除用户
        ADD_METHOD_TO(UserController::deleteUser, "/api/user/{id}", drogon::Delete,
                      "middleware::JwtFilter", "middleware::LogFilter");
    METHOD_LIST_END

    // 获取当前用户信息
    drogon::Task<> getCurrentUser(drogon::HttpRequestPtr req,
                                   std::function<void(const drogon::HttpResponsePtr&)> callback);

    // 获取用户列表
    drogon::Task<> listUsers(drogon::HttpRequestPtr req,
                              std::function<void(const drogon::HttpResponsePtr&)> callback);

    // 获取指定用户
    drogon::Task<> getUserById(drogon::HttpRequestPtr req,
                                std::function<void(const drogon::HttpResponsePtr&)> callback,
                                int64_t id);

    // 更新用户
    drogon::Task<> updateUser(drogon::HttpRequestPtr req,
                               std::function<void(const drogon::HttpResponsePtr&)> callback,
                               int64_t id);

    // 禁用/启用用户
    drogon::Task<> setUserStatus(drogon::HttpRequestPtr req,
                                  std::function<void(const drogon::HttpResponsePtr&)> callback,
                                  int64_t id);

    // 删除用户
    drogon::Task<> deleteUser(drogon::HttpRequestPtr req,
                               std::function<void(const drogon::HttpResponsePtr&)> callback,
                               int64_t id);
};

} // namespace controllers