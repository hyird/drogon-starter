#include "UserService.hpp"
#include "models/UserMapper.hpp"
#include "core/Exception.hpp"
#include "core/Constants.hpp"
#include <spdlog/spdlog.h>

namespace services {

UserService& UserService::instance() {
    static UserService instance;
    return instance;
}

drogon::Task<models::Users> UserService::getUserById(int64_t userId) {
    auto dbClient = drogon::app().getDbClient();
    models::UserMapper mapper(dbClient);

    co_return co_await mapper.findById(userId);
}

drogon::Task<models::PageResult> UserService::listUsers(int page, int pageSize,
                                                         const std::string& keyword) {
    auto dbClient = drogon::app().getDbClient();
    models::UserMapper mapper(dbClient);

    auto result = co_await mapper.findPage(page, pageSize, keyword);

    spdlog::debug("Listed users: page={}, pageSize={}, total={}",
                  page, pageSize, result.total);

    co_return result;
}

drogon::Task<bool> UserService::updateUser(int64_t userId,
                                            const std::optional<std::string>& email,
                                            const std::optional<std::string>& role) {
    if (!email && !role) {
        throw core::ParamException("nothing to update");
    }

    auto dbClient = drogon::app().getDbClient();
    models::UserMapper mapper(dbClient);

    // 检查用户是否存在
    auto userOpt = co_await mapper.findByIdOptional(userId);
    if (!userOpt) {
        throw core::NotFoundException("user not found: " + std::to_string(userId));
    }

    // 构建更新字段
    std::vector<std::pair<std::string, std::string>> fields;
    if (email) {
        fields.emplace_back("email", *email);
    }
    if (role) {
        fields.emplace_back("role", *role);
    }

    co_await mapper.updateFields(userId, fields);

    spdlog::info("User updated: userId={}", userId);
    co_return true;
}

drogon::Task<bool> UserService::setUserStatus(int64_t userId, int status) {
    if (status != 0 && status != 1) {
        throw core::ParamException("status must be 0 or 1");
    }

    auto dbClient = drogon::app().getDbClient();
    models::UserMapper mapper(dbClient);

    // 检查用户是否存在
    auto userOpt = co_await mapper.findByIdOptional(userId);
    if (!userOpt) {
        throw core::NotFoundException("user not found: " + std::to_string(userId));
    }

    std::vector<std::pair<std::string, std::string>> fields = {
        {"status", std::to_string(status)}
    };
    co_await mapper.updateFields(userId, fields);

    spdlog::info("User status updated: userId={}, status={}", userId, status);
    co_return true;
}

drogon::Task<bool> UserService::deleteUser(int64_t userId) {
    auto dbClient = drogon::app().getDbClient();
    models::UserMapper mapper(dbClient);

    bool deleted = co_await mapper.deleteById(userId);
    if (!deleted) {
        throw core::NotFoundException("user not found: " + std::to_string(userId));
    }

    spdlog::info("User deleted: userId={}", userId);
    co_return true;
}

} // namespace services