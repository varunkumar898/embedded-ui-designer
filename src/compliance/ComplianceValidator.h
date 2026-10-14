#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace compliance {

enum class ComplianceFramework {
    OWASP_TOP_10 = 0,
    CIS_BENCHMARKS = 1,
    CERT_SECURE_CODING = 2,
};

struct ComplianceCheck {
    std::string checkId;
    std::string checkName;
    ComplianceFramework framework;
    std::string description;
    std::string remediation;
    bool passed;
};

struct ComplianceReport {
    std::string reportId;
    std::string timestamp;
    uint32_t totalChecks;
    uint32_t passedChecks;
    uint32_t failedChecks;
    float complianceScore; // 0-100%
    std::vector<ComplianceCheck> failedItems;
};

class ComplianceValidator {
public:
    ComplianceValidator();
    
    // Validate against OWASP Top 10
    ComplianceReport validateOWASP();
    
    // Validate against CIS benchmarks
    ComplianceReport validateCIS();
    
    // Generate full report
    ComplianceReport generateFullReport();
    
    // Check for injection vulnerabilities
    bool checkInjectionVulnerabilities();
    
    // Check for broken authentication
    bool checkAuthenticationStrength();
    
    // Check for sensitive data exposure
    bool checkDataEncryption();
    
    // Check for XML external entities (XXE)
    bool checkXXEPrevention();
    
    // Export report to file
    bool exportReport(const ComplianceReport& report, const std::string& filePath);
    
private:
    std::vector<ComplianceCheck> checks;
    void initializeChecks();
};

} // namespace compliance
