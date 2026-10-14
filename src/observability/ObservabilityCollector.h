#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace observability {

enum class MetricType {
    COUNTER = 0,      // Monotonically increasing
    GAUGE = 1,        // Can go up or down
    HISTOGRAM = 2,    // Distribution of values
    SUMMARY = 3,      // Percentiles
};

struct Metric {
    std::string name;
    MetricType type;
    double value;
    std::map<std::string, std::string> labels; // e.g., {"endpoint": "/api/v1/readGpio", "method": "GET"}
    std::chrono::system_clock::time_point timestamp;
};

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    std::string level; // "DEBUG", "INFO", "WARN", "ERROR"
    std::string message;
    std::map<std::string, std::string> fields;
};

class ObservabilityCollector {
public:
    ObservabilityCollector();
    
    // Record metric
    void recordMetric(const Metric& metric);
    
    // Record log
    void logEvent(const LogEntry& entry);
    
    // Export metrics in Prometheus format
    std::string exportPrometheus();
    
    // Export logs in JSON format
    std::string exportJsonLogs();
    
    // Flush to backend (Prometheus, ELK, etc.)
    bool flushMetrics(const std::string& prometheusPushUrl);
    bool flushLogs(const std::string& elasticsearchUrl);
    
    // Get current metrics snapshot
    std::vector<Metric> getMetrics() const;
    
    // Clear metrics and logs
    void clear();
    
private:
    std::vector<Metric> metrics;
    std::vector<LogEntry> logs;
    
    std::string metricToPrometheus(const Metric& m) const;
    std::string logToJson(const LogEntry& l) const;
};

} // namespace observability
