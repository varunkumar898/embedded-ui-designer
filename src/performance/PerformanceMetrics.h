#pragma once
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <cstdint>

namespace performance {

struct Histogram {
    std::string name;
    std::vector<uint32_t> buckets;
    uint32_t total{0};
};

struct PerformanceStats {
    std::string endpointId; // "/api/v1/readGpio"
    uint32_t requestCount{0};
    uint32_t successCount{0};
    uint32_t errorCount{0};
    uint64_t totalLatencyMs{0};
    uint32_t minLatencyMs{0};
    uint32_t maxLatencyMs{0};
    uint32_t avgLatencyMs{0};
    uint32_t p99LatencyMs{0};
};

class PerformanceMetrics {
public:
    PerformanceMetrics();
    
    // Record request latency
    void recordLatency(const std::string& endpointId, uint32_t latencyMs, bool success = true);
    
    // Record memory usage
    void recordMemoryUsage(uint32_t heapUsedBytes, uint32_t heapFreeBytes);
    
    // Get statistics for endpoint
    PerformanceStats getStats(const std::string& endpointId) const;
    
    // Get all statistics
    std::map<std::string, PerformanceStats> getAllStats() const;
    
    // Get memory watermark (peak usage)
    uint32_t getMemoryWatermark() const;
    
    // Reset metrics
    void reset();
    
private:
    std::map<std::string, std::vector<uint32_t>> latencies;
    std::map<std::string, uint32_t> successCounts;
    std::map<std::string, uint32_t> errorCounts;
    
    uint32_t memoryWatermark{0};
};

} // namespace performance
