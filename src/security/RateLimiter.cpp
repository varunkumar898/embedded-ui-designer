#include "RateLimiter.h"
#include <algorithm>

namespace security {

RateLimiter::RateLimiter(uint32_t defaultRequestsPerWindow, 
                         std::chrono::seconds defaultWindowDuration)
    : defaultRequestsPerWindow(defaultRequestsPerWindow),
      defaultWindowDuration(defaultWindowDuration) {}

bool RateLimiter::allowRequest(const std::string& targetId) {
    auto now = std::chrono::system_clock::now();
    
    LimitConfig limit;
    if (customLimits.count(targetId)) {
        limit = customLimits[targetId];
    } else {
        limit = { defaultRequestsPerWindow, defaultWindowDuration };
    }
    
    auto& queue = requestQueues[targetId];
    while (!queue.empty() && (now - queue.front().timestamp) > limit.windowDuration) {
        queue.pop();
    }
    
    if (queue.size() < limit.requestsPerWindow) {
        queue.push({ now, targetId });
        return true;
    }
    
    return false;
}

void RateLimiter::setLimit(const std::string& targetId, uint32_t requestsPerWindow,
                           std::chrono::seconds windowDuration) {
    customLimits[targetId] = { requestsPerWindow, windowDuration };
}

uint32_t RateLimiter::getRequestCount(const std::string& targetId) const {
    auto it = requestQueues.find(targetId);
    if (it == requestQueues.end()) return 0;
    return static_cast<uint32_t>(it->second.size());
}

void RateLimiter::resetTarget(const std::string& targetId) {
    requestQueues.erase(targetId);
}

void RateLimiter::cleanup() {
    auto now = std::chrono::system_clock::now();
    
    for (auto& pair : requestQueues) {
        const std::string& targetId = pair.first;
        auto& queue = pair.second;
        
        LimitConfig limit;
        if (customLimits.count(targetId)) {
            limit = customLimits[targetId];
        } else {
            limit = { defaultRequestsPerWindow, defaultWindowDuration };
        }
        
        while (!queue.empty() && (now - queue.front().timestamp) > limit.windowDuration) {
            queue.pop();
        }
    }
}

} // namespace security
