#include "Users.hpp"

namespace models {

// 静态成员初始化
const std::string Users::tableName = "users";
const std::string Users::primaryKeyName = "id";
const bool Users::hasPrimaryKey = true;
const bool Users::hasAutoIncrementPrimary = true;

const std::string Users::Cols::id = "id";
const std::string Users::Cols::username = "username";
const std::string Users::Cols::email = "email";
const std::string Users::Cols::password_hash = "password_hash";
const std::string Users::Cols::salt = "salt";
const std::string Users::Cols::role = "role";
const std::string Users::Cols::status = "status";
const std::string Users::Cols::created_at = "created_at";
const std::string Users::Cols::updated_at = "updated_at";
const std::string Users::Cols::last_login_at = "last_login_at";

Users::Users(const drogon::orm::Row& row) {
    if (!row["id"].isNull()) {
        id_ = row["id"].as<int64_t>();
    }
    if (!row["username"].isNull()) {
        username_ = row["username"].as<std::string>();
    }
    if (!row["email"].isNull()) {
        email_ = row["email"].as<std::string>();
    }
    if (!row["password_hash"].isNull()) {
        passwordHash_ = row["password_hash"].as<std::string>();
    }
    if (!row["salt"].isNull()) {
        salt_ = row["salt"].as<std::string>();
    }
    if (!row["role"].isNull()) {
        role_ = row["role"].as<std::string>();
    }
    if (!row["status"].isNull()) {
        status_ = row["status"].as<int>();
    }
    if (!row["created_at"].isNull()) {
        createdAt_ = row["created_at"].as<std::string>();
    }
    if (!row["updated_at"].isNull()) {
        updatedAt_ = row["updated_at"].as<std::string>();
    }
    if (!row["last_login_at"].isNull()) {
        lastLoginAt_ = row["last_login_at"].as<std::string>();
    }
}

Json::Value Users::toJson() const {
    Json::Value json;
    json["id"] = static_cast<Json::Int64>(id_);
    json["username"] = username_;
    json["email"] = email_;
    json["passwordHash"] = passwordHash_;
    json["salt"] = salt_;
    json["role"] = role_;
    json["status"] = status_;
    json["createdAt"] = createdAt_;
    json["updatedAt"] = updatedAt_.value_or("");
    json["lastLoginAt"] = lastLoginAt_.value_or("");
    return json;
}

Json::Value Users::toJsonForApi() const {
    Json::Value json;
    json["id"] = static_cast<Json::Int64>(id_);
    json["username"] = username_;
    json["email"] = email_;
    json["role"] = role_;
    json["status"] = status_;
    json["createdAt"] = createdAt_;
    if (updatedAt_) {
        json["updatedAt"] = *updatedAt_;
    } else {
        json["updatedAt"] = Json::nullValue;
    }
    if (lastLoginAt_) {
        json["lastLoginAt"] = *lastLoginAt_;
    } else {
        json["lastLoginAt"] = Json::nullValue;
    }
    return json;
}

std::vector<std::pair<std::string, std::string>> Users::getChangedColumns() const {
    std::vector<std::pair<std::string, std::string>> cols;
    
    if (usernameChanged_) cols.emplace_back(Cols::username, username_);
    if (emailChanged_) cols.emplace_back(Cols::email, email_);
    if (passwordHashChanged_) cols.emplace_back(Cols::password_hash, passwordHash_);
    if (saltChanged_) cols.emplace_back(Cols::salt, salt_);
    if (roleChanged_) cols.emplace_back(Cols::role, role_);
    if (statusChanged_) cols.emplace_back(Cols::status, std::to_string(status_));
    if (updatedAtChanged_ && updatedAt_) cols.emplace_back(Cols::updated_at, *updatedAt_);
    if (lastLoginAtChanged_ && lastLoginAt_) cols.emplace_back(Cols::last_login_at, *lastLoginAt_);
    
    return cols;
}

void Users::resetChangedFlags() {
    idChanged_ = false;
    usernameChanged_ = false;
    emailChanged_ = false;
    passwordHashChanged_ = false;
    saltChanged_ = false;
    roleChanged_ = false;
    statusChanged_ = false;
    createdAtChanged_ = false;
    updatedAtChanged_ = false;
    lastLoginAtChanged_ = false;
}

} // namespace models