#include "UserService.hpp"
#include "core/Exception.hpp"
#include "core/Constants.hpp"
#include <spdlog/spdlog.h>

namespace services {

// ==================== UserInfo ====================

Json::Value UserInfo::toJson() const {
    Json::Value json;
    json["id"] = id;
    json["username"] = username;
    json["email"] = email;
    json["role"] = role;
    json["status"] = status;
    json["createdAt"] = createdAt;
    json["lastLoginAt"] = lastLoginAt;
    return json;
}

// ==================== UserService ====================

UserService& UserService::instance() {
    static UserService instance;
    return instance;
}

UserInfo UserService::buildUserInfo(const drogon::orm::Row& row) {
    UserInfo info;
    info.id = row["id"].as<std::string>();
    info.username = row["username"].as<std::string>();
    info.email = row["email"].as<std::string>();
    info.role = row["role"].as<std::string>();
    info.status = row["status"].as<int>();
    
    // 处理可能为 NULL 的时间字段
    if (!row["created_at"].isNull()) {
        info.createdAt = row["created_at"].as<std::string>();
    }
    if (!row["last_login_at"].isNull()) {
        info.lastLoginAt = row["last_login_at"].as<std::string>();
    }
    
    return info;
}

drogon::Task<UserInfo> UserService::getUserById(const std::string& userId) {
    auto dbClient = drogon::app().getDbClient();

    auto result = co_await dbClient->execSqlCoro(
        "SELECT id, username, email, role, status, created_at, last_login_at "
        "FROM users WHERE id = ? LIMIT 1",
        userId
    );

    if (result.empty()) {
        throw core::NotFoundException("user not found: " + userId);
    }

    co_return buildUserInfo(result[0]);
}

drogon::Task<PageResult> UserService::listUsers(int page, int pageSize,
                                                 const std::string& keyword) {
    // 参数校验
    if (page < 1) page = core::constants::DEFAULT_PAGE;
    if (pageSize < 1) pageSize = core::constants::DEFAULT_PAGE_SIZE;
    if (pageSize > core::constants::MAX_PAGE_SIZE) {
        pageSize = core::constants::MAX_PAGE_SIZE;
    }

    auto dbClient = drogon::app().getDbClient();
    int offset = (page - 1) * pageSize;

    PageResult result;
    result.page = page;
    result.pageSize = pageSize;

    if (keyword.empty()) {
        // 无搜索条件
        auto countResult = co_await dbClient->execSqlCoro(
            "SELECT COUNT(*) as total FROM users"
        );
        result.total = countResult[0]["total"].as<int64_t>();

        auto listResult = co_await dbClient->execSqlCoro(
            "SELECT id, username, email, role, status, created_at, last_login_at "
            "FROM users ORDER BY created_at DESC LIMIT ? OFFSET ?",
            pageSize, offset
        );

        for (const auto& row : listResult) {
            result.list.push_back(buildUserInfo(row));
        }
    } else {
        // 带搜索条件
        std::string likePattern = "%" + keyword + "%";

        auto countResult = co_await dbClient->execSqlCoro(
            "SELECT COUNT(*) as total FROM users "
            "WHERE username LIKE ? OR email LIKE ?",
            likePattern, likePattern
        );
        result.total = countResult[0]["total"].as<int64_t>();

        auto listResult = co_await dbClient->execSqlCoro(
            "SELECT id, username, email, role, status, created_at, last_login_at "
            "FROM users WHERE username LIKE ? OR email LIKE ? "
            "ORDER BY created_at DESC LIMIT ? OFFSET ?",
            likePattern, likePattern, pageSize, offset
        );

        for (const auto& row : listResult) {
            result.list.push_back(buildUserInfo(row));
        }
    }

    spdlog::debug("Listed users: page={}, pageSize={}, total={}", 
                  page, pageSize, result.total);

    co_return result;
}

drogon::Task<bool> UserService::updateUser(const std::string& userId,
                                            const std::optional<std::string>& email,
                                            const std::optional<std::string>& role) {
    if (!email && !role) {
        throw core::ParamException("nothing to update");
    }

    auto dbClient = drogon::app().getDbClient();

    // 检查用户是否存在
    auto checkResult = co_await dbClient->execSqlCoro(
        "SELECT id FROM users WHERE id = ? LIMIT 1",
        userId
    );

    if (checkResult.empty()) {
        throw core::NotFoundException("user not found: " + userId);
    }

    // 动态构建 SQL
    std::string sql = "UPDATE users SET updated_at = NOW()";
    std::vector<std::string> params;

    if (email) {
        sql += ", email = ?";
        params.push_back(*email);
    }

    if (role) {
        sql += ", role = ?";
        params.push_back(*role);
    }

    sql += " WHERE id = ?";
    params.push_back(userId);

    // 执行更新（根据参数数量选择）
    if (email && role) {
        co_await dbClient->execSqlCoro(sql, *email, *role, userId);
    } else if (email) {
        co_await dbClient->execSqlCoro(sql, *email, userId);
    } else {
        co_await dbClient->execSqlCoro(sql, *role, userId);
    }

    spdlog::info("User updated: userId={}", userId);
    co_return true;
}

drogon::Task<bool> UserService::setUserStatus(const std::string& userId, int status) {
    if (status != 0 && status != 1) {
        throw core::ParamException("status must be 0 or 1");
    }

    auto dbClient = drogon::app().getDbClient();

    auto result = co_await dbClient->execSqlCoro(
        "UPDATE users SET status = ?, updated_at = NOW() WHERE id = ?",
        status, userId
    );

    if (result.affectedRows() == 0) {
        throw core::NotFoundException("user not found: " + userId);
    }

    spdlog::info("User status updated: userId={}, status={}", userId, status);
    co_return true;
}

drogon::Task<bool> UserService::deleteUser(const std::string& userId) {
    auto dbClient = drogon::app().getDbClient();

    auto result = co_await dbClient->execSqlCoro(
        "DELETE FROM users WHERE id = ?",
        userId
    );

    if (result.affectedRows() == 0) {
        throw core::NotFoundException("user not found: " + userId);
    }

    spdlog::info("User deleted: userId={}", userId);
    co_return true;
}

} // namespace services