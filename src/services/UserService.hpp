#pragma once

#include <string>
#include <vector>
#include <optional>
#include <drogon/drogon.h>
#include <json/json.h>
#include "models/Users.hpp"
#include "models/UserMapper.hpp"

namespace services {

    // 用户服务
    class UserService {
    public:
        // 单例获取
        static UserService& instance();

        // 获取用户信息
        drogon::Task<models::Users> getUserById(int64_t userId);

        // 获取用户列表（分页）
        drogon::Task<models::PageResult> listUsers(int page, int pageSize,
                                                    const std::string& keyword = "");

        // 更新用户信息
        drogon::Task<bool> updateUser(int64_t userId,
                                       const std::optional<std::string>& email,
                                       const std::optional<std::string>& role);

        // 禁用/启用用户
        drogon::Task<bool> setUserStatus(int64_t userId, int status);

        // 删除用户
        drogon::Task<bool> deleteUser(int64_t userId);

    private:
        UserService() = default;
        ~UserService() = default;
        UserService(const UserService&) = delete;
        UserService& operator=(const UserService&) = delete;
    };

} // namespace services