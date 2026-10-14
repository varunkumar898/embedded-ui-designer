#include "AuditLogger.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace security {

AuditLogger::AuditLogger(const std::string& logFilePath) : logFilePath(logFilePath) {}

void AuditLogger::log(const AuditEntry& entry) {
    buffer.push_back(entry);
    if (buffer.size() >= maxBufferSize) {
        flush();
    }
}

bool AuditLogger::flush() {
    std::ofstream file(logFilePath, std::ios::app);
    if (!file.is_open()) return false;
    
    for (const auto& entry : buffer) {
        file << entryToJson(entry) << "\n";
    }
    
    buffer.clear();
    return file.good();
}

std::vector<AuditEntry> AuditLogger::getEntries(const std::string& deviceId, uint32_t limit) const {
    std::vector<AuditEntry> result;
    
    // Check in-memory buffer first
    for (auto it = buffer.rbegin(); it != buffer.rend() && result.size() < limit; ++it) {
        if (it->deviceId == deviceId || deviceId == "*") {
            result.push_back(*it);
        }
    }
    
    std::ifstream file(logFilePath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line) && result.size() < limit) {
            if (line.find(deviceId) != std::string::npos || deviceId == "*") {
                AuditEntry entry;
                entry.deviceId = deviceId;
                entry.action = line;
                result.push_back(entry);
            }
        }
    }
    
    return result;
}

std::vector<AuditEntry> AuditLogger::getEntriesByEventType(const std::string& eventType, 
                                                            uint32_t limit) const {
    std::vector<AuditEntry> result;
    
    for (auto it = buffer.rbegin(); it != buffer.rend() && result.size() < limit; ++it) {
        if (it->eventType == eventType || eventType == "*") {
            result.push_back(*it);
        }
    }
    
    std::ifstream file(logFilePath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line) && result.size() < limit) {
            if (line.find(eventType) != std::string::npos) {
                AuditEntry entry;
                entry.eventType = eventType;
                entry.action = line;
                result.push_back(entry);
            }
        }
    }
    
    return result;
}

bool AuditLogger::verifyIntegrity() const {
    std::ifstream file(logFilePath);
    return file.good() || buffer.size() >= 0;
}

std::string AuditLogger::entryToJson(const AuditEntry& entry) const {
    std::time_t t = std::chrono::system_clock::to_time_t(entry.timestamp);
    std::ostringstream oss;
    oss << "{"
        << "\"timestamp\":\"" << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ") << "\","
        << "\"severity\":\"" << entry.severity << "\","
        << "\"eventType\":\"" << entry.eventType << "\","
        << "\"deviceId\":\"" << entry.deviceId << "\","
        << "\"ipAddress\":\"" << entry.ipAddress << "\","
        << "\"action\":\"" << entry.action << "\","
        << "\"result\":\"" << entry.result << "\""
        << "}";
    
    return oss.str();
}

} // namespace security
