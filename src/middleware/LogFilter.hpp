#pragma once

#include <drogon/HttpFilter.h>
#include <chrono>

namespace middleware {

    // 请求日志过滤器
    class LogFilter : public drogon::HttpFilter<LogFilter> {
    public:
        LogFilter() = default;

        void doFilter(const drogon::HttpRequestPtr& req,
                      drogon::FilterCallback&& fcb,
                      drogon::FilterChainCallback&& fccb) override;
    };

    // 请求上下文（用于记录耗时等）
    class RequestContext {
    public:
        void start();
        [[nodiscard]] int64_t elapsedMs() const;
        void setRequestId(const std::string& id);
        [[nodiscard]] const std::string& requestId() const;

    private:
        std::chrono::steady_clock::time_point startTime_;
        std::string requestId_;
    };

} // namespace middleware