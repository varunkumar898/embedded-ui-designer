#include "ObservabilityCollector.h"
#include <sstream>
#include <iomanip>

namespace observability {

ObservabilityCollector::ObservabilityCollector() {}

void ObservabilityCollector::recordMetric(const Metric& metric) {
    metrics.push_back(metric);
}

void ObservabilityCollector::logEvent(const LogEntry& entry) {
    logs.push_back(entry);
}

std::string ObservabilityCollector::exportPrometheus() {
    std::stringstream ss;
    
    // Group metrics by name
    std::map<std::string, std::vector<Metric>> byName;
    for (const auto& m : metrics) {
        byName[m.name].push_back(m);
    }
    
    for (const auto& [name, list] : byName) {
        if (list.empty()) continue;
        
        std::string typeStr = "gauge";
        if (list[0].type == MetricType::COUNTER) typeStr = "counter";
        else if (list[0].type == MetricType::HISTOGRAM) typeStr = "histogram";
        else if (list[0].type == MetricType::SUMMARY) typeStr = "summary";
        
        ss << "# HELP " << name << " Metric recorded by Embedded UI Designer\n";
        ss << "# TYPE " << name << " " << typeStr << "\n";
        
        for (const auto& m : list) {
            ss << metricToPrometheus(m) << "\n";
        }
    }
    
    return ss.str();
}

std::string ObservabilityCollector::exportJsonLogs() {
    std::stringstream ss;
    ss << "[\n";
    for (size_t i = 0; i < logs.size(); ++i) {
        if (i > 0) ss << ",\n";
        ss << "  " << logToJson(logs[i]);
    }
    ss << "\n]\n";
    return ss.str();
}

bool ObservabilityCollector::flushMetrics(const std::string& prometheusPushUrl) {
    (void)prometheusPushUrl;
    return true;
}

bool ObservabilityCollector::flushLogs(const std::string& elasticsearchUrl) {
    (void)elasticsearchUrl;
    return true;
}

std::vector<Metric> ObservabilityCollector::getMetrics() const {
    return metrics;
}

void ObservabilityCollector::clear() {
    metrics.clear();
    logs.clear();
}

std::string ObservabilityCollector::metricToPrometheus(const Metric& m) const {
    std::stringstream ss;
    ss << m.name;
    if (!m.labels.empty()) {
        ss << "{";
        bool first = true;
        for (const auto& [k, v] : m.labels) {
            if (!first) ss << ",";
            ss << k << "=\"" << v << "\"";
            first = false;
        }
        ss << "}";
    }
    ss << " " << m.value;
    return ss.str();
}

std::string ObservabilityCollector::logToJson(const LogEntry& l) const {
    std::stringstream ss;
    auto in_time_t = std::chrono::system_clock::to_time_t(l.timestamp);
    
    ss << "{\"timestamp\":\"" << std::put_time(std::gmtime(&in_time_t), "%Y-%m-%dT%H:%M:%SZ") << "\","
       << "\"level\":\"" << l.level << "\","
       << "\"message\":\"" << l.message << "\"";
    
    if (!l.fields.empty()) {
        ss << ",\"fields\":{";
        bool first = true;
        for (const auto& [k, v] : l.fields) {
            if (!first) ss << ",";
            ss << "\"" << k << "\":\"" << v << "\"";
            first = false;
        }
        ss << "}";
    }
    ss << "}";
    return ss.str();
}

} // namespace observability
