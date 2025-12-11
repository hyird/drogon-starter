#pragma once

#include "Users.hpp"
#include <drogon/drogon.h>
#include <optional>
#include <vector>

namespace models {

    // 分页结果
    struct PageResult {
        std::vector<Users> list;
        int64_t total{0};
        int page{1};
        int pageSize{10};
    };

    // 用户 Mapper（协程版）
    class UserMapper {
    public:
        explicit UserMapper(drogon::orm::DbClientPtr client);

        // 基础 CRUD
        drogon::Task<Users> findById(int64_t id);
        drogon::Task<std::optional<Users>> findByIdOptional(int64_t id);
        drogon::Task<std::optional<Users>> findByUsername(const std::string& username);
        drogon::Task<std::optional<Users>> findByEmail(const std::string& email);

        // 插入（返回自增 ID）
        drogon::Task<int64_t> insert(const Users& user);

        // 更新
        drogon::Task<bool> update(const Users& user);
        drogon::Task<bool> updateFields(int64_t id, const std::vector<std::pair<std::string, std::string>>& fields);

        // 删除
        drogon::Task<bool> deleteById(int64_t id);

        // 分页查询
        drogon::Task<PageResult> findPage(int page, int pageSize, const std::string& keyword = "");

        // 计数
        drogon::Task<int64_t> count();
        drogon::Task<int64_t> countByKeyword(const std::string& keyword);

        // 检查存在
        drogon::Task<bool> existsByUsername(const std::string& username);
        drogon::Task<bool> existsByEmail(const std::string& email);
        drogon::Task<bool> existsByUsernameOrEmail(const std::string& username, const std::string& email);

        // 更新登录时间
        drogon::Task<bool> updateLastLoginTime(int64_t id);

    private:
        drogon::orm::DbClientPtr dbClient_;
    };

} // namespace models