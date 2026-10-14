#include "PerformanceMetrics.h"
#include <algorithm>
#include <numeric>

namespace performance {

PerformanceMetrics::PerformanceMetrics() {}

void PerformanceMetrics::recordLatency(const std::string& endpointId, 
                                       uint32_t latencyMs, bool success) {
    latencies[endpointId].push_back(latencyMs);
    
    if (success) {
        successCounts[endpointId]++;
    } else {
        errorCounts[endpointId]++;
    }
}

void PerformanceMetrics::recordMemoryUsage(uint32_t heapUsedBytes, uint32_t heapFreeBytes) {
    (void)heapFreeBytes;
    if (heapUsedBytes > memoryWatermark) {
        memoryWatermark = heapUsedBytes;
    }
}

PerformanceStats PerformanceMetrics::getStats(const std::string& endpointId) const {
    PerformanceStats stats;
    stats.endpointId = endpointId;
    
    auto it = latencies.find(endpointId);
    if (it == latencies.end()) return stats;
    
    const auto& lats = it->second;
    if (lats.empty()) return stats;
    
    stats.requestCount = static_cast<uint32_t>(lats.size());
    stats.successCount = successCounts.count(endpointId) ? successCounts.at(endpointId) : 0;
    stats.errorCount = errorCounts.count(endpointId) ? errorCounts.at(endpointId) : 0;
    
    stats.totalLatencyMs = std::accumulate(lats.begin(), lats.end(), 0ULL);
    stats.minLatencyMs = *std::min_element(lats.begin(), lats.end());
    stats.maxLatencyMs = *std::max_element(lats.begin(), lats.end());
    stats.avgLatencyMs = static_cast<uint32_t>(stats.totalLatencyMs / stats.requestCount);
    
    std::vector<uint32_t> sorted = lats;
    std::sort(sorted.begin(), sorted.end());
    size_t p99Index = (sorted.size() * 99) / 100;
    if (p99Index >= sorted.size()) p99Index = sorted.size() - 1;
    stats.p99LatencyMs = sorted[p99Index];
    
    return stats;
}

std::map<std::string, PerformanceStats> PerformanceMetrics::getAllStats() const {
    std::map<std::string, PerformanceStats> results;
    for (const auto& pair : latencies) {
        results[pair.first] = getStats(pair.first);
    }
    return results;
}

uint32_t PerformanceMetrics::getMemoryWatermark() const {
    return memoryWatermark;
}

void PerformanceMetrics::reset() {
    latencies.clear();
    successCounts.clear();
    errorCounts.clear();
    memoryWatermark = 0;
}

} // namespace performance
