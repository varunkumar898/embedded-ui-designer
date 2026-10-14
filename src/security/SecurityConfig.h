#pragma once
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>

namespace security {

enum class AuthScheme {
    NONE = 0,
    API_KEY = 1,
    BEARER_TOKEN = 2,
    HMAC_SHA256 = 3,
};

enum class CertificateFormat {
    PEM = 0,
    DER = 1,
};

struct ApiKey {
    std::string keyId;
    std::string keySecret;
    std::vector<std::string> allowedEndpoints;
    std::chrono::system_clock::time_point expiresAt;
    bool isActive{true};
};

struct BearerToken {
    std::string token;
    std::string deviceId;
    std::chrono::system_clock::time_point issuedAt;
    std::chrono::system_clock::time_point expiresAt;
    std::vector<std::string> scopes; // ["gpio.read", "adc.read", "pwm.write"]
};

struct HmacSecret {
    std::string deviceId;
    std::string secret; // Base64-encoded
    std::string algorithm; // "HMAC-SHA256"
    uint32_t nonceSize{16}; // 16 bytes
};

struct CertificateMetadata {
    std::string path;
    CertificateFormat format{CertificateFormat::PEM};
    std::chrono::system_clock::time_point validFrom;
    std::chrono::system_clock::time_point validUntil;
    std::string subjectDN;
    std::string issuerDN;
    bool isSelfSigned{false};
};

struct RateLimitPolicy {
    std::string policyId;
    std::string targetId; // IP address or device ID
    uint32_t requestsPerWindow{1000};
    std::chrono::seconds windowDuration{60};
    bool enabled{true};
};

struct AuditLogEntry {
    std::chrono::system_clock::time_point timestamp;
    std::string eventType; // "AUTH_SUCCESS", "AUTH_FAILURE", "FIRMWARE_UPDATE", "RATE_LIMIT_EXCEEDED"
    std::string deviceId;
    std::string ipAddress;
    std::string endpoint;
    std::string result; // "SUCCESS", "FAILURE", "BLOCKED"
    std::string details;
};

class SecurityConfig {
public:
    SecurityConfig();
    
    // Certificate management
    bool loadCertificate(const std::string& path, CertificateFormat format = CertificateFormat::PEM);
    bool loadPrivateKey(const std::string& path);
    CertificateMetadata getCertificateMetadata() const;
    bool isCertificateExpiringSoon(uint32_t daysThreshold = 30) const;
    void setCertificateMetadata(const CertificateMetadata& meta) { certMetadata = meta; }
    
    // Authentication
    bool registerApiKey(const ApiKey& key);
    bool validateApiKey(const std::string& keyId, const std::string& keySecret) const;
    bool isApiKeyAllowed(const std::string& keyId, const std::string& endpoint) const;
    
    bool issueToken(const BearerToken& token);
    bool validateToken(const std::string& token) const;
    bool tokenHasScope(const std::string& token, const std::string& scope) const;
    
    bool registerHmacSecret(const HmacSecret& secret);
    bool validateHmacSignature(const std::string& deviceId, const std::string& payload, 
                                const std::string& signature, const std::string& nonce) const;
    
    // Rate limiting
    bool addRateLimitPolicy(const RateLimitPolicy& policy);
    bool isRateLimited(const std::string& targetId);
    
    // Audit logging
    void logAuditEvent(const AuditLogEntry& entry);
    std::vector<AuditLogEntry> getAuditLog(const std::string& deviceId, 
                                            uint32_t limitLast = 100) const;
    
    // Configuration
    void setAuthScheme(AuthScheme scheme) { authScheme = scheme; }
    AuthScheme getAuthScheme() const { return authScheme; }
    
    void setTlsVersion(const std::string& version) { tlsVersion = version; } // "1.2", "1.3"
    std::string getTlsVersion() const { return tlsVersion; }
    
private:
    std::string certificate;
    std::string privateKey;
    CertificateMetadata certMetadata;
    
    std::map<std::string, ApiKey> apiKeys;
    std::map<std::string, BearerToken> bearerTokens;
    std::map<std::string, HmacSecret> hmacSecrets;
    std::map<std::string, RateLimitPolicy> rateLimitPolicies;
    std::vector<AuditLogEntry> auditLog;
    
    AuthScheme authScheme;
    std::string tlsVersion;
};

} // namespace security
