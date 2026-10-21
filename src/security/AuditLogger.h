#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <chrono>

namespace security {

struct AuditEntry {
    std::chrono::system_clock::time_point timestamp;
    std::string severity; // "INFO", "WARNING", "CRITICAL"
    std::string eventType; // "AUTH", "FIRMWARE_UPDATE", "RATE_LIMIT", "CONFIG_CHANGE"
    std::string deviceId;
    std::string ipAddress;
    std::string action;
    std::string result; // "SUCCESS", "FAILURE"
    std::string details;
    std::string userId;
};

class AuditLogger {
public:
    /// Cross-platform default log location (see AuditLogger.cpp):
    /// $EUIDESIGNER_AUDIT_LOG override -> <AppLocalDataLocation>/embedded-ui-audit.log
    /// -> OS temp dir.
    static std::string defaultLogFilePath();

    explicit AuditLogger(const std::string& logFilePath = defaultLogFilePath());
    
    void log(const AuditEntry& entry);
    bool flush();
    std::vector<AuditEntry> getEntries(const std::string& deviceId, uint32_t limit = 100) const;
    std::vector<AuditEntry> getEntriesByEventType(const std::string& eventType, uint32_t limit = 100) const;
    bool verifyIntegrity() const;
    
private:
    std::string logFilePath;
    std::vector<AuditEntry> buffer;
    uint32_t maxBufferSize{500};
    
    std::string entryToJson(const AuditEntry& entry) const;
};

} // namespace security
