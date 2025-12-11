#include "UserMapper.hpp"
#include "core/Exception.hpp"
#include "core/Constants.hpp"
#include <spdlog/spdlog.h>

namespace models {

UserMapper::UserMapper(drogon::orm::DbClientPtr client)
    : dbClient_(std::move(client)) {
}

drogon::Task<Users> UserMapper::findById(int64_t id) {
    auto result = co_await dbClient_->execSqlCoro(
        "SELECT * FROM users WHERE id = ? LIMIT 1",
        id
    );
    
    if (result.empty()) {
        throw core::NotFoundException("user not found: " + std::to_string(id));
    }
    
    co_return Users(result[0]);
}

drogon::Task<std::optional<Users>> UserMapper::findByIdOptional(int64_t id) {
    auto result = co_await dbClient_->execSqlCoro(
        "SELECT * FROM users WHERE id = ? LIMIT 1",
        id
    );
    
    if (result.empty()) {
        co_return std::nullopt;
    }
    
    co_return Users(result[0]);
}

drogon::Task<std::optional<Users>> UserMapper::findByUsername(const std::string& username) {
    auto result = co_await dbClient_->execSqlCoro(
        "SELECT * FROM users WHERE username = ? LIMIT 1",
        username
    );
    
    if (result.empty()) {
        co_return std::nullopt;
    }
    
    co_return Users(result[0]);
}

drogon::Task<std::optional<Users>> UserMapper::findByEmail(const std::string& email) {
    auto result = co_await dbClient_->execSqlCoro(
        "SELECT * FROM users WHERE email = ? LIMIT 1",
        email
    );
    
    if (result.empty()) {
        co_return std::nullopt;
    }
    
    co_return Users(result[0]);
}

drogon::Task<int64_t> UserMapper::insert(const Users& user) {
    co_await dbClient_->execSqlCoro(
        "INSERT INTO users (username, email, password_hash, salt, role, status, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, NOW())",
        user.getUsername(),
        user.getEmail(),
        user.getPasswordHash(),
        user.getSalt(),
        user.getRole(),
        user.getStatus()
    );
    
    // 获取自增 ID
    auto idResult = co_await dbClient_->execSqlCoro("SELECT LAST_INSERT_ID() as id");
    
    if (idResult.empty()) {
        throw core::DbException(core::ErrorCode::DB_QUERY_ERROR, "Failed to get insert id");
    }
    
    co_return idResult[0]["id"].as<int64_t>();
}

drogon::Task<bool> UserMapper::update(const Users& user) {
    auto result = co_await dbClient_->execSqlCoro(
        "UPDATE users SET username = ?, email = ?, password_hash = ?, salt = ?, "
        "role = ?, status = ?, updated_at = NOW() WHERE id = ?",
        user.getUsername(),
        user.getEmail(),
        user.getPasswordHash(),
        user.getSalt(),
        user.getRole(),
        user.getStatus(),
        user.getId()
    );
    
    co_return result.affectedRows() > 0;
}

drogon::Task<bool> UserMapper::updateFields(int64_t id, 
                                             const std::vector<std::pair<std::string, std::string>>& fields) {
    if (fields.empty()) {
        co_return false;
    }
    
    std::string sql = "UPDATE users SET updated_at = NOW()";
    std::vector<std::string> values;
    
    for (const auto& [field, value] : fields) {
        sql += ", " + field + " = ?";
        values.push_back(value);
    }
    sql += " WHERE id = ?";
    
    // 根据字段数量动态执行
    drogon::orm::Result result;
    switch (values.size()) {
        case 1:
            result = co_await dbClient_->execSqlCoro(sql, values[0], id);
            break;
        case 2:
            result = co_await dbClient_->execSqlCoro(sql, values[0], values[1], id);
            break;
        case 3:
            result = co_await dbClient_->execSqlCoro(sql, values[0], values[1], values[2], id);
            break;
        case 4:
            result = co_await dbClient_->execSqlCoro(sql, values[0], values[1], values[2], values[3], id);
            break;
        default:
            throw core::DbException(core::ErrorCode::DB_QUERY_ERROR, "Too many fields to update");
    }
    
    co_return result.affectedRows() > 0;
}

drogon::Task<bool> UserMapper::deleteById(int64_t id) {
    auto result = co_await dbClient_->execSqlCoro(
        "DELETE FROM users WHERE id = ?",
        id
    );
    
    co_return result.affectedRows() > 0;
}

drogon::Task<PageResult> UserMapper::findPage(int page, int pageSize, const std::string& keyword) {
    if (page < 1) page = core::constants::DEFAULT_PAGE;
    if (pageSize < 1) pageSize = core::constants::DEFAULT_PAGE_SIZE;
    if (pageSize > core::constants::MAX_PAGE_SIZE) pageSize = core::constants::MAX_PAGE_SIZE;
    
    int offset = (page - 1) * pageSize;
    PageResult result;
    result.page = page;
    result.pageSize = pageSize;
    
    if (keyword.empty()) {
        // 无搜索条件
        auto countResult = co_await dbClient_->execSqlCoro(
            "SELECT COUNT(*) as total FROM users"
        );
        result.total = countResult[0]["total"].as<int64_t>();
        
        auto listResult = co_await dbClient_->execSqlCoro(
            "SELECT * FROM users ORDER BY created_at DESC LIMIT ? OFFSET ?",
            pageSize, offset
        );
        
        for (const auto& row : listResult) {
            result.list.emplace_back(row);
        }
    } else {
        // 带搜索条件
        std::string pattern = "%" + keyword + "%";
        
        auto countResult = co_await dbClient_->execSqlCoro(
            "SELECT COUNT(*) as total FROM users WHERE username LIKE ? OR email LIKE ?",
            pattern, pattern
        );
        result.total = countResult[0]["total"].as<int64_t>();
        
        auto listResult = co_await dbClient_->execSqlCoro(
            "SELECT * FROM users WHERE username LIKE ? OR email LIKE ? "
            "ORDER BY created_at DESC LIMIT ? OFFSET ?",
            pattern, pattern, pageSize, offset
        );
        
        for (const auto& row : listResult) {
            result.list.emplace_back(row);
        }
    }
    
    co_return result;
}

drogon::Task<int64_t> UserMapper::count() {
    auto result = co_await dbClient_->execSqlCoro(
        "SELECT COUNT(*) as total FROM users"
    );
    co_return result[0]["total"].as<int64_t>();
}

drogon::Task<int64_t> UserMapper::countByKeyword(const std::string& keyword) {
    std::string pattern = "%" + keyword + "%";
    auto result = co_await dbClient_->execSqlCoro(
        "SELECT COUNT(*) as total FROM users WHERE username LIKE ? OR email LIKE ?",
        pattern, pattern
    );
    co_return result[0]["total"].as<int64_t>();
}

drogon::Task<bool> UserMapper::existsByUsername(const std::string& username) {
    auto result = co_await dbClient_->execSqlCoro(
        "SELECT 1 FROM users WHERE username = ? LIMIT 1",
        username
    );
    co_return !result.empty();
}

drogon::Task<bool> UserMapper::existsByEmail(const std::string& email) {
    auto result = co_await dbClient_->execSqlCoro(
        "SELECT 1 FROM users WHERE email = ? LIMIT 1",
        email
    );
    co_return !result.empty();
}

drogon::Task<bool> UserMapper::existsByUsernameOrEmail(const std::string& username, 
                                                        const std::string& email) {
    auto result = co_await dbClient_->execSqlCoro(
        "SELECT 1 FROM users WHERE username = ? OR email = ? LIMIT 1",
        username, email
    );
    co_return !result.empty();
}

drogon::Task<bool> UserMapper::updateLastLoginTime(int64_t id) {
    auto result = co_await dbClient_->execSqlCoro(
        "UPDATE users SET last_login_at = NOW() WHERE id = ?",
        id
    );
    co_return result.affectedRows() > 0;
}

} // namespace models