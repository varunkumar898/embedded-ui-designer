#pragma once
#include <string>
#include <map>
#include <chrono>
#include <memory>
#include <cstdint>

namespace performance {

struct CacheEntry {
    std::string key;
    std::string value;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastAccessedAt;
    uint32_t accessCount{0};
    uint32_t ttlSeconds{300};
};

class ResponseCache {
public:
    ResponseCache(uint32_t maxEntries = 1000, uint32_t defaultTtlSeconds = 300);
    
    void put(const std::string& key, const std::string& value, uint32_t ttlSeconds = 0);
    bool get(const std::string& key, std::string& outValue);
    void remove(const std::string& key);
    void clear();
    uint32_t evictExpired();
    
    uint32_t getSize() const;
    uint32_t getHitCount() const;
    uint32_t getMissCount() const;
    
private:
    std::map<std::string, std::shared_ptr<CacheEntry>> entries;
    uint32_t maxEntries;
    uint32_t defaultTtlSeconds;
    uint32_t hitCount{0};
    uint32_t missCount{0};
    
    void evictLruIfNeeded();
};

} // namespace performance
