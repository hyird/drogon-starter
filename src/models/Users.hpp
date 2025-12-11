#pragma once

#include <drogon/orm/Result.h>
#include <drogon/orm/Row.h>
#include <drogon/orm/Field.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/Criteria.h>
#include <json/json.h>
#include <string>
#include <optional>
#include <cstdint>

namespace models {

class Users {
public:
    // 表名
    static const std::string tableName;
    
    // 列名常量
    struct Cols {
        static const std::string id;
        static const std::string username;
        static const std::string email;
        static const std::string password_hash;
        static const std::string salt;
        static const std::string role;
        static const std::string status;
        static const std::string created_at;
        static const std::string updated_at;
        static const std::string last_login_at;
    };

    // 主键名
    static const std::string primaryKeyName;
    static const bool hasPrimaryKey;
    static const bool hasAutoIncrementPrimary;

    // 构造函数
    Users() = default;
    explicit Users(const drogon::orm::Row& row);

    // Getter
    [[nodiscard]] int64_t getId() const { return id_; }
    [[nodiscard]] const std::string& getUsername() const { return username_; }
    [[nodiscard]] const std::string& getEmail() const { return email_; }
    [[nodiscard]] const std::string& getPasswordHash() const { return passwordHash_; }
    [[nodiscard]] const std::string& getSalt() const { return salt_; }
    [[nodiscard]] const std::string& getRole() const { return role_; }
    [[nodiscard]] int getStatus() const { return status_; }
    [[nodiscard]] const std::string& getCreatedAt() const { return createdAt_; }
    [[nodiscard]] const std::optional<std::string>& getUpdatedAt() const { return updatedAt_; }
    [[nodiscard]] const std::optional<std::string>& getLastLoginAt() const { return lastLoginAt_; }

    // Setter
    void setId(int64_t id) { id_ = id; idChanged_ = true; }
    void setUsername(const std::string& username) { username_ = username; usernameChanged_ = true; }
    void setEmail(const std::string& email) { email_ = email; emailChanged_ = true; }
    void setPasswordHash(const std::string& hash) { passwordHash_ = hash; passwordHashChanged_ = true; }
    void setSalt(const std::string& salt) { salt_ = salt; saltChanged_ = true; }
    void setRole(const std::string& role) { role_ = role; roleChanged_ = true; }
    void setStatus(int status) { status_ = status; statusChanged_ = true; }
    void setCreatedAt(const std::string& time) { createdAt_ = time; createdAtChanged_ = true; }
    void setUpdatedAt(const std::string& time) { updatedAt_ = time; updatedAtChanged_ = true; }
    void setLastLoginAt(const std::string& time) { lastLoginAt_ = time; lastLoginAtChanged_ = true; }

    // JSON 转换
    [[nodiscard]] Json::Value toJson() const;
    [[nodiscard]] Json::Value toJsonForApi() const;  // 不包含敏感字段

    // 获取主键值
    [[nodiscard]] int64_t getPrimaryKey() const { return id_; }

    // 获取变更的列（用于 UPDATE）
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> getChangedColumns() const;

    // 重置变更标记
    void resetChangedFlags();

private:
    int64_t id_{0};
    std::string username_;
    std::string email_;
    std::string passwordHash_;
    std::string salt_;
    std::string role_{"user"};
    int status_{1};
    std::string createdAt_;
    std::optional<std::string> updatedAt_;
    std::optional<std::string> lastLoginAt_;

    // 变更标记
    bool idChanged_{false};
    bool usernameChanged_{false};
    bool emailChanged_{false};
    bool passwordHashChanged_{false};
    bool saltChanged_{false};
    bool roleChanged_{false};
    bool statusChanged_{false};
    bool createdAtChanged_{false};
    bool updatedAtChanged_{false};
    bool lastLoginAtChanged_{false};
};

} // namespace models