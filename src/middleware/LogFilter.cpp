#include "LogFilter.hpp"
#include "core/Constants.hpp"
#include "utils/Crypto.hpp"
#include <spdlog/spdlog.h>

namespace middleware {

    // ==================== RequestContext ====================

    void RequestContext::start() {
        startTime_ = std::chrono::steady_clock::now();
    }

    int64_t RequestContext::elapsedMs() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now - startTime_
        ).count();
    }

    void RequestContext::setRequestId(const std::string& id) {
        requestId_ = id;
    }

    const std::string& RequestContext::requestId() const {
        return requestId_;
    }

    // ==================== LogFilter ====================

    void LogFilter::doFilter(const drogon::HttpRequestPtr& req,
                             drogon::FilterCallback&& fcb,
                             drogon::FilterChainCallback&& fccb) {
        // 生成或获取请求 ID
        auto requestId = req->getHeader(core::constants::HEADER_REQUEST_ID);
        if (requestId.empty()) {
            requestId = utils::Crypto::uuid();
        }

        // 创建请求上下文
        auto ctx = std::make_shared<RequestContext>();
        ctx->start();
        ctx->setRequestId(requestId);

        // 存入请求属性
        req->getAttributes()->insert("requestId", requestId);
        req->getAttributes()->insert("requestContext", ctx);

        // 获取客户端 IP
        auto clientIp = req->getPeerAddr().toIp();

        // 记录请求开始
        spdlog::info("[{}] --> {} {} from {}",
                     requestId,
                     req->getMethodString(),
                     req->getPath(),
                     clientIp);

        // 如果有 body，记录（仅 debug 级别）
        if (spdlog::get_level() <= spdlog::level::debug) {
            auto body = req->getBody();
            if (!body.empty() && body.size() < 1024) {
                spdlog::debug("[{}] Body: {}", requestId, body);
            }
        }

        // 继续处理，并在响应时记录
        fccb();
    }

} // namespace middleware