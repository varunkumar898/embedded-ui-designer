#include "ResponseCache.h"
#include <algorithm>

namespace performance {

ResponseCache::ResponseCache(uint32_t maxEntries, uint32_t defaultTtlSeconds)
    : maxEntries(maxEntries), defaultTtlSeconds(defaultTtlSeconds) {}

void ResponseCache::put(const std::string& key, const std::string& value, uint32_t ttlSeconds) {
    auto it = entries.find(key);
    if (it != entries.end()) {
        it->second->value = value;
        it->second->lastAccessedAt = std::chrono::system_clock::now();
        it->second->accessCount++;
        return;
    }

    evictLruIfNeeded();
    
    auto entry = std::make_shared<CacheEntry>();
    entry->key = key;
    entry->value = value;
    entry->createdAt = std::chrono::system_clock::now();
    entry->lastAccessedAt = entry->createdAt;
    entry->accessCount = 0;
    entry->ttlSeconds = (ttlSeconds > 0) ? ttlSeconds : defaultTtlSeconds;
    
    entries[key] = entry;
}

bool ResponseCache::get(const std::string& key, std::string& outValue) {
    auto it = entries.find(key);
    if (it == entries.end()) {
        missCount++;
        return false;
    }
    
    auto entry = it->second;
    auto now = std::chrono::system_clock::now();
    
    // Check expiration
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - entry->createdAt).count() >= static_cast<int64_t>(entry->ttlSeconds * 1000)) {
        entries.erase(it);
        missCount++;
        return false;
    }
    
    outValue = entry->value;
    entry->lastAccessedAt = now;
    entry->accessCount++;
    hitCount++;
    
    return true;
}

void ResponseCache::remove(const std::string& key) {
    entries.erase(key);
}

void ResponseCache::clear() {
    entries.clear();
    hitCount = 0;
    missCount = 0;
}

uint32_t ResponseCache::evictExpired() {
    auto now = std::chrono::system_clock::now();
    uint32_t evicted = 0;
    
    auto it = entries.begin();
    while (it != entries.end()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second->createdAt).count() >= static_cast<int64_t>(it->second->ttlSeconds * 1000)) {
            it = entries.erase(it);
            evicted++;
        } else {
            ++it;
        }
    }
    
    return evicted;
}

uint32_t ResponseCache::getSize() const {
    return static_cast<uint32_t>(entries.size());
}

uint32_t ResponseCache::getHitCount() const {
    return hitCount;
}

uint32_t ResponseCache::getMissCount() const {
    return missCount;
}

void ResponseCache::evictLruIfNeeded() {
    if (entries.size() >= maxEntries && !entries.empty()) {
        auto lruIt = entries.begin();
        auto minTime = lruIt->second->lastAccessedAt;
        
        for (auto it = entries.begin(); it != entries.end(); ++it) {
            if (it->second->lastAccessedAt < minTime) {
                minTime = it->second->lastAccessedAt;
                lruIt = it;
            }
        }
        
        entries.erase(lruIt);
    }
}

} // namespace performance
