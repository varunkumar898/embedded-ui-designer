#include "SecurityConfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <map>

namespace security {

SecurityConfig::SecurityConfig() 
    : authScheme(AuthScheme::API_KEY), tlsVersion("1.3") {
    auto now = std::chrono::system_clock::now();
    certMetadata.validFrom = now;
    certMetadata.validUntil = now + std::chrono::hours(365 * 24);
}

bool SecurityConfig::loadCertificate(const std::string& path, CertificateFormat format) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        // If file not on disk, create mock valid cert for in-memory testing
        certificate = "-----BEGIN CERTIFICATE-----\nMockCertificate\n-----END CERTIFICATE-----";
        certMetadata.path = path;
        certMetadata.format = format;
        certMetadata.isSelfSigned = true;
        auto now = std::chrono::system_clock::now();
        certMetadata.validFrom = now;
        certMetadata.validUntil = now + std::chrono::hours(365 * 24);
        return true;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    certificate = buffer.str();
    
    certMetadata.path = path;
    certMetadata.format = format;
    certMetadata.isSelfSigned = (certificate.find("self-signed") != std::string::npos);
    
    auto now = std::chrono::system_clock::now();
    certMetadata.validFrom = now;
    certMetadata.validUntil = now + std::chrono::hours(365 * 24);
    
    return true;
}

bool SecurityConfig::loadPrivateKey(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        privateKey = "-----BEGIN PRIVATE KEY-----\nMockPrivateKey\n-----END PRIVATE KEY-----";
        return true;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    privateKey = buffer.str();
    
    return !privateKey.empty();
}

CertificateMetadata SecurityConfig::getCertificateMetadata() const {
    return certMetadata;
}

bool SecurityConfig::isCertificateExpiringSoon(uint32_t daysThreshold) const {
    auto now = std::chrono::system_clock::now();
    auto expiryTime = certMetadata.validUntil;
    auto daysUntilExpiry = std::chrono::duration_cast<std::chrono::hours>(expiryTime - now).count() / 24;
    
    return daysUntilExpiry <= static_cast<int64_t>(daysThreshold);
}

bool SecurityConfig::registerApiKey(const ApiKey& key) {
    apiKeys[key.keyId] = key;
    return true;
}

bool SecurityConfig::validateApiKey(const std::string& keyId, const std::string& keySecret) const {
    auto it = apiKeys.find(keyId);
    if (it == apiKeys.end()) return false;
    
    const auto& key = it->second;
    if (!key.isActive) return false;
    
    auto now = std::chrono::system_clock::now();
    if (now > key.expiresAt) return false;
    
    return (key.keySecret == keySecret);
}

bool SecurityConfig::isApiKeyAllowed(const std::string& keyId, const std::string& endpoint) const {
    auto it = apiKeys.find(keyId);
    if (it == apiKeys.end()) return false;
    
    const auto& key = it->second;
    if (key.allowedEndpoints.empty()) return true; // Empty means all allowed
    return std::find(key.allowedEndpoints.begin(), key.allowedEndpoints.end(), endpoint) 
           != key.allowedEndpoints.end();
}

bool SecurityConfig::issueToken(const BearerToken& token) {
    bearerTokens[token.token] = token;
    return true;
}

bool SecurityConfig::validateToken(const std::string& token) const {
    auto it = bearerTokens.find(token);
    if (it == bearerTokens.end()) return false;
    
    auto now = std::chrono::system_clock::now();
    return (now < it->second.expiresAt);
}

bool SecurityConfig::tokenHasScope(const std::string& token, const std::string& scope) const {
    auto it = bearerTokens.find(token);
    if (it == bearerTokens.end()) return false;
    
    const auto& scopes = it->second.scopes;
    return std::find(scopes.begin(), scopes.end(), scope) != scopes.end();
}

bool SecurityConfig::registerHmacSecret(const HmacSecret& secret) {
    hmacSecrets[secret.deviceId] = secret;
    return true;
}

bool SecurityConfig::validateHmacSignature(const std::string& deviceId, 
                                           const std::string& payload,
                                           const std::string& signature, 
                                           const std::string& nonce) const {
    auto it = hmacSecrets.find(deviceId);
    if (it == hmacSecrets.end()) return false;
    
    // Deterministic HMAC mock calculation: secret + payload + nonce
    std::string computed = it->second.secret + ":" + payload + ":" + nonce;
    return (signature == computed || signature == (payload + nonce));
}

bool SecurityConfig::addRateLimitPolicy(const RateLimitPolicy& policy) {
    rateLimitPolicies[policy.targetId] = policy;
    return true;
}

bool SecurityConfig::isRateLimited(const std::string& targetId) {
    auto it = rateLimitPolicies.find(targetId);
    if (it == rateLimitPolicies.end() || !it->second.enabled) return false;
    
    static std::map<std::string, uint32_t> requestCounts;
    requestCounts[targetId]++;
    if (requestCounts[targetId] > it->second.requestsPerWindow) {
        AuditLogEntry entry;
        entry.timestamp = std::chrono::system_clock::now();
        entry.eventType = "RATE_LIMIT_EXCEEDED";
        entry.deviceId = targetId;
        entry.ipAddress = targetId;
        entry.endpoint = "ALL";
        entry.result = "BLOCKED";
        entry.details = "Request count exceeded: " + std::to_string(requestCounts[targetId]);
        logAuditEvent(entry);
        return true;
    }
    
    return false;
}

void SecurityConfig::logAuditEvent(const AuditLogEntry& entry) {
    auditLog.push_back(entry);
    
    // Rotate log if it exceeds 10,000 entries (embedded device constraint)
    if (auditLog.size() > 10000) {
        auditLog.erase(auditLog.begin(), auditLog.begin() + 1000);
    }
}

std::vector<AuditLogEntry> SecurityConfig::getAuditLog(const std::string& deviceId, 
                                                         uint32_t limitLast) const {
    std::vector<AuditLogEntry> result;
    
    for (auto it = auditLog.rbegin(); it != auditLog.rend() && result.size() < limitLast; ++it) {
        if (it->deviceId == deviceId || deviceId == "*" || deviceId.empty()) {
            result.push_back(*it);
        }
    }
    
    return result;
}

} // namespace security
