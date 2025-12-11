#pragma once

#include <drogon/HttpResponse.h>
#include <json/json.h>
#include "Error.hpp"
#include "Exception.hpp"

namespace core {

class Response {
public:
    // 成功响应（无数据）
    static drogon::HttpResponsePtr success() {
        return build(ErrorCode::SUCCESS, Json::Value(Json::nullValue));
    }

    // 成功响应（带数据）
    static drogon::HttpResponsePtr success(const Json::Value& data) {
        return build(ErrorCode::SUCCESS, data);
    }

    // 成功响应（带消息）
    static drogon::HttpResponsePtr successMsg(const std::string& message) {
        Json::Value json;
        json["code"] = toInt(ErrorCode::SUCCESS);
        json["message"] = message;
        json["data"] = Json::nullValue;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        resp->setStatusCode(drogon::k200OK);
        return resp;
    }

    // 错误响应
    static drogon::HttpResponsePtr error(ErrorCode code,
                                         const std::string& detail = "") {
        Json::Value json;
        json["code"] = toInt(code);
        json["message"] = detail.empty() ? getErrorMessage(code) : detail;
        json["data"] = Json::nullValue;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        resp->setStatusCode(mapHttpStatus(code));
        return resp;
    }

    // 从异常构建响应
    static drogon::HttpResponsePtr fromException(const AppException& e) {
        return error(e.code(), e.fullMessage());
    }

    // 分页响应
    static drogon::HttpResponsePtr page(const Json::Value& list,
                                        int64_t total,
                                        int page,
                                        int pageSize) {
        Json::Value data;
        data["list"] = list;
        data["total"] = static_cast<Json::Int64>(total);
        data["page"] = page;
        data["pageSize"] = pageSize;
        data["totalPages"] = static_cast<Json::Int64>((total + pageSize - 1) / pageSize);

        return success(data);
    }

private:
    // 构建响应
    static drogon::HttpResponsePtr build(ErrorCode code, const Json::Value& data) {
        Json::Value json;
        json["code"] = toInt(code);
        json["message"] = getErrorMessage(code);
        json["data"] = data;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        resp->setStatusCode(mapHttpStatus(code));
        return resp;
    }

    // 错误码映射到 HTTP 状态码
    static drogon::HttpStatusCode mapHttpStatus(ErrorCode code) {
        switch (code) {
            case ErrorCode::SUCCESS:
                return drogon::k200OK;

            case ErrorCode::INVALID_PARAMS:
                return drogon::k400BadRequest;

            case ErrorCode::AUTH_FAILED:
            case ErrorCode::TOKEN_INVALID:
            case ErrorCode::TOKEN_EXPIRED:
            case ErrorCode::TOKEN_MISSING:
            case ErrorCode::PASSWORD_INCORRECT:
                return drogon::k401Unauthorized;

            case ErrorCode::PERMISSION_DENIED:
            case ErrorCode::USER_DISABLED:
                return drogon::k403Forbidden;

            case ErrorCode::RESOURCE_NOT_FOUND:
            case ErrorCode::USER_NOT_FOUND:
                return drogon::k404NotFound;

            case ErrorCode::USER_ALREADY_EXISTS:
                return drogon::k409Conflict;
            
            case ErrorCode::RATE_LIMIT_EXCEEDED:
                return drogon::k429TooManyRequests;
            
            case ErrorCode::DB_CONNECTION_ERROR:
            case ErrorCode::DB_QUERY_ERROR:
            case ErrorCode::REDIS_CONNECTION_ERROR:
                return drogon::k503ServiceUnavailable;
            
            default:
                return drogon::k500InternalServerError;
        }
    }
};

} // namespace core