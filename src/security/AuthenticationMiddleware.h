#pragma once
#include "SecurityConfig.h"
#include <string>
#include <map>

namespace security {

struct AuthContext {
    bool isAuthenticated{false};
    std::string deviceId;
    std::string userId;
    std::vector<std::string> scopes;
    std::string authMethod; // "API_KEY", "BEARER_TOKEN", "HMAC"
};

class AuthenticationMiddleware {
public:
    AuthenticationMiddleware(SecurityConfig& config);
    
    AuthContext validateRequest(const std::string& method,
                                const std::string& path,
                                const std::map<std::string, std::string>& headers,
                                const std::string& body);
    
    bool checkScope(const AuthContext& ctx, const std::string& requiredScope) const;
    
private:
    SecurityConfig& securityConfig;
    
    AuthContext validateApiKey(const std::map<std::string, std::string>& headers);
    AuthContext validateBearerToken(const std::map<std::string, std::string>& headers);
    AuthContext validateHmacSignature(const std::map<std::string, std::string>& headers,
                                      const std::string& body);
};

} // namespace security
