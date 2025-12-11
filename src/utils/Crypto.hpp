#pragma once

#include <drogon/utils/Utilities.h>
#include <string>
#include <random>

namespace utils {

    class Crypto {
    public:
        // MD5 哈希
        static std::string md5(const std::string& input) {
            return drogon::utils::getMd5(input);
        }

        // SHA256 哈希
        static std::string sha256(const std::string& input) {
            return drogon::utils::getSha256(input);
        }

        // Base64 编码
        static std::string base64Encode(const std::string& input) {
            return drogon::utils::base64Encode(
                reinterpret_cast<const unsigned char*>(input.data()),
                input.size()
            );
        }

        // Base64 解码
        static std::string base64Decode(const std::string& input) {
            return drogon::utils::base64Decode(input);
        }

        // 生成 UUID
        static std::string uuid() {
            return drogon::utils::getUuid();
        }

        // 生成随机字符串
        static std::string randomString(size_t length) {
            static const char charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";

            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

            std::string result;
            result.reserve(length);
            for (size_t i = 0; i < length; ++i) {
                result += charset[dist(gen)];
            }
            return result;
        }

        // 密码哈希（加盐）
        static std::string hashPassword(const std::string& password, const std::string& salt) {
            return sha256(salt + password + salt);
        }

        // 生成盐值
        static std::string generateSalt() {
            return randomString(32);
        }

        // 验证密码
        static bool verifyPassword(const std::string& password,
                                   const std::string& salt,
                                   const std::string& hash) {
            return hashPassword(password, salt) == hash;
        }
    };

} // namespace utils