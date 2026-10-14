#include "AuthenticationMiddleware.h"
#include <algorithm>
#include <sstream>

namespace security {

AuthenticationMiddleware::AuthenticationMiddleware(SecurityConfig& config) 
    : securityConfig(config) {}

AuthContext AuthenticationMiddleware::validateRequest(const std::string& method,
                                                       const std::string& path,
                                                       const std::map<std::string, std::string>& headers,
                                                       const std::string& body) {
    (void)method;
    AuthContext ctx = { false, "", "", {}, "" };
    
    // Check HMAC headers first if present
    if (headers.count("X-Signature") && headers.count("X-Device-Id")) {
        ctx = validateHmacSignature(headers, body);
        ctx.authMethod = "HMAC";
    } else {
        auto authHeader = headers.find("Authorization");
        if (authHeader == headers.end()) {
            AuditLogEntry entry;
            entry.timestamp = std::chrono::system_clock::now();
            entry.eventType = "AUTH_FAILURE";
            entry.deviceId = "UNKNOWN";
            entry.ipAddress = headers.count("X-Forwarded-For") ? headers.at("X-Forwarded-For") : "0.0.0.0";
            entry.endpoint = path;
            entry.result = "FAILURE";
            entry.details = "Missing Authorization header";
            securityConfig.logAuditEvent(entry);
            return ctx;
        }
        
        const auto& authValue = authHeader->second;
        if (authValue.find("Bearer ") == 0) {
            ctx = validateBearerToken(headers);
            ctx.authMethod = "BEARER_TOKEN";
        } else if (authValue.find("ApiKey ") == 0) {
            ctx = validateApiKey(headers);
            ctx.authMethod = "API_KEY";
        }
    }
    
    if (ctx.isAuthenticated) {
        AuditLogEntry entry;
        entry.timestamp = std::chrono::system_clock::now();
        entry.eventType = "AUTH_SUCCESS";
        entry.deviceId = ctx.deviceId;
        entry.ipAddress = headers.count("X-Forwarded-For") ? headers.at("X-Forwarded-For") : "0.0.0.0";
        entry.endpoint = path;
        entry.result = "SUCCESS";
        entry.details = "Authenticated with " + ctx.authMethod;
        securityConfig.logAuditEvent(entry);
    } else {
        AuditLogEntry entry;
        entry.timestamp = std::chrono::system_clock::now();
        entry.eventType = "AUTH_FAILURE";
        entry.deviceId = ctx.deviceId.empty() ? "UNKNOWN" : ctx.deviceId;
        entry.ipAddress = headers.count("X-Forwarded-For") ? headers.at("X-Forwarded-For") : "0.0.0.0";
        entry.endpoint = path;
        entry.result = "FAILURE";
        entry.details = "Invalid credentials";
        securityConfig.logAuditEvent(entry);
    }
    
    return ctx;
}

bool AuthenticationMiddleware::checkScope(const AuthContext& ctx, const std::string& requiredScope) const {
    return std::find(ctx.scopes.begin(), ctx.scopes.end(), requiredScope) != ctx.scopes.end();
}

AuthContext AuthenticationMiddleware::validateApiKey(const std::map<std::string, std::string>& headers) {
    AuthContext ctx = { false, "", "", {}, "" };
    
    auto authHeader = headers.find("Authorization");
    if (authHeader == headers.end()) return ctx;
    
    std::string authValue = authHeader->second;
    size_t spacePos = authValue.find(' ');
    if (spacePos == std::string::npos) return ctx;
    
    std::string keyId = authValue.substr(spacePos + 1);
    
    auto keySecretHeader = headers.find("X-API-Secret");
    if (keySecretHeader == headers.end()) return ctx;
    
    std::string keySecret = keySecretHeader->second;
    
    if (securityConfig.validateApiKey(keyId, keySecret)) {
        ctx.isAuthenticated = true;
        ctx.deviceId = keyId;
        ctx.scopes = { "gpio.read", "adc.read", "pwm.write" };
    }
    
    return ctx;
}

AuthContext AuthenticationMiddleware::validateBearerToken(const std::map<std::string, std::string>& headers) {
    AuthContext ctx = { false, "", "", {}, "" };
    
    auto authHeader = headers.find("Authorization");
    if (authHeader == headers.end()) return ctx;
    
    std::string authValue = authHeader->second;
    size_t spacePos = authValue.find(' ');
    if (spacePos == std::string::npos) return ctx;
    
    std::string token = authValue.substr(spacePos + 1);
    
    if (securityConfig.validateToken(token)) {
        ctx.isAuthenticated = true;
        ctx.deviceId = "token-bearer";
        ctx.scopes = { "gpio.read", "adc.read", "pwm.write" };
    }
    
    return ctx;
}

AuthContext AuthenticationMiddleware::validateHmacSignature(const std::map<std::string, std::string>& headers,
                                                             const std::string& body) {
    AuthContext ctx = { false, "", "", {}, "" };
    
    auto deviceIdHeader = headers.find("X-Device-Id");
    auto signatureHeader = headers.find("X-Signature");
    auto nonceHeader = headers.find("X-Nonce");
    
    if (deviceIdHeader == headers.end() || signatureHeader == headers.end() || nonceHeader == headers.end()) {
        return ctx;
    }
    
    std::string deviceId = deviceIdHeader->second;
    std::string signature = signatureHeader->second;
    std::string nonce = nonceHeader->second;
    
    if (securityConfig.validateHmacSignature(deviceId, body, signature, nonce)) {
        ctx.isAuthenticated = true;
        ctx.deviceId = deviceId;
        ctx.scopes = { "gpio.read", "adc.read", "pwm.write" };
    }
    
    return ctx;
}

} // namespace security
