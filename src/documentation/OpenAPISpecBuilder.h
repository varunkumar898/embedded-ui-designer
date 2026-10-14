#pragma once
#include <string>
#include <vector>
#include <map>

namespace documentation {

class OpenAPISpecBuilder {
public:
    OpenAPISpecBuilder(const std::string& title = "Embedded UI API",
                       const std::string& version = "1.0.0");
    
    // Build complete OpenAPI 3.0 spec
    std::string buildSpec();
    
    // Add endpoint to spec
    void addEndpoint(const std::string& operationId,
                     const std::string& method,
                     const std::string& path,
                     const std::string& description,
                     const std::map<std::string, std::string>& parameters);
    
    // Add schema definition
    void addSchema(const std::string& schemaName, const std::string& schemaJson);
    
    // Export to file
    bool exportToFile(const std::string& filePath);
    
private:
    std::string title;
    std::string version;
    std::map<std::string, std::string> endpoints; // operationId -> spec
    std::map<std::string, std::string> schemas;
};

} // namespace documentation
