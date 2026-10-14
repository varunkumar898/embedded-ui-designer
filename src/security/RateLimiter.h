#pragma once
#include <string>
#include <map>
#include <queue>
#include <chrono>
#include <memory>

namespace security {

struct RequestRecord {
    std::chrono::system_clock::time_point timestamp;
    std::string targetId;
};

class RateLimiter {
public:
    RateLimiter(uint32_t defaultRequestsPerWindow = 1000, 
                std::chrono::seconds defaultWindowDuration = std::chrono::seconds(60));
    
    // Check and enforce rate limit
    bool allowRequest(const std::string& targetId);
    
    // Set custom limit for a target
    void setLimit(const std::string& targetId, uint32_t requestsPerWindow, 
                  std::chrono::seconds windowDuration);
    
    // Get current request count for a target
    uint32_t getRequestCount(const std::string& targetId) const;
    
    // Reset counter for a target
    void resetTarget(const std::string& targetId);
    
    // Cleanup old records
    void cleanup();
    
private:
    struct LimitConfig {
        uint32_t requestsPerWindow;
        std::chrono::seconds windowDuration;
    };
    
    std::map<std::string, LimitConfig> customLimits;
    std::map<std::string, std::queue<RequestRecord>> requestQueues;
    
    uint32_t defaultRequestsPerWindow;
    std::chrono::seconds defaultWindowDuration;
};

} // namespace security
