#pragma once

#include <string>
#include <vector>
#include <optional>
#include <drogon/drogon.h>
#include <json/json.h>

namespace services {

    // 用户信息
    struct UserInfo {
        std::string id;
        std::string username;
        std::string email;
        std::string role;
        int status;
        std::string createdAt;
        std::string lastLoginAt;

        // 转换为 JSON
        [[nodiscard]] Json::Value toJson() const;
    };

    // 分页结果
    struct PageResult {
        std::vector<UserInfo> list;
        int64_t total;
        int page;
        int pageSize;
    };

    // 用户服务
    class UserService {
    public:
        // 单例获取
        static UserService& instance();

        // 获取用户信息
        drogon::Task<UserInfo> getUserById(const std::string& userId);

        // 获取用户列表（分页）
        drogon::Task<PageResult> listUsers(int page, int pageSize,
                                            const std::string& keyword = "");

        // 更新用户信息
        drogon::Task<bool> updateUser(const std::string& userId,
                                       const std::optional<std::string>& email,
                                       const std::optional<std::string>& role);

        // 禁用/启用用户
        drogon::Task<bool> setUserStatus(const std::string& userId, int status);

        // 删除用户
        drogon::Task<bool> deleteUser(const std::string& userId);

    private:
        UserService() = default;
        ~UserService() = default;
        UserService(const UserService&) = delete;
        UserService& operator=(const UserService&) = delete;

        // 从数据库行构建 UserInfo
        static UserInfo buildUserInfo(const drogon::orm::Row& row);
    };

} // namespace services