#include "OpenAPISpecBuilder.h"
#include <sstream>
#include <fstream>

namespace documentation {

OpenAPISpecBuilder::OpenAPISpecBuilder(const std::string& title, const std::string& version)
    : title(title), version(version) {}

std::string OpenAPISpecBuilder::buildSpec() {
    std::stringstream ss;
    
    ss << "{\n";
    ss << "  \"openapi\": \"3.0.0\",\n";
    ss << "  \"info\": {\n";
    ss << "    \"title\": \"" << title << "\",\n";
    ss << "    \"version\": \"" << version << "\",\n";
    ss << "    \"description\": \"Type-safe REST API for embedded UI control\"\n";
    ss << "  },\n";
    
    ss << "  \"servers\": [\n";
    ss << "    { \"url\": \"http://localhost:8080/api/v1\" },\n";
    ss << "    { \"url\": \"https://embedded-device.local/api/v1\" }\n";
    ss << "  ],\n";
    
    ss << "  \"paths\": {\n";
    
    bool first = true;
    for (const auto& [opId, spec] : endpoints) {
        if (!first) ss << ",\n";
        ss << "    " << spec;
        first = false;
    }
    
    ss << "\n  },\n";
    
    ss << "  \"components\": {\n";
    ss << "    \"schemas\": {\n";
    
    first = true;
    for (const auto& [name, schema] : schemas) {
        if (!first) ss << ",\n";
        ss << "      \"" << name << "\": " << schema;
        first = false;
    }
    
    ss << "\n    },\n";
    ss << "    \"securitySchemes\": {\n";
    ss << "      \"ApiKey\": { \"type\": \"apiKey\", \"in\": \"header\", \"name\": \"Authorization\" },\n";
    ss << "      \"BearerToken\": { \"type\": \"http\", \"scheme\": \"bearer\" },\n";
    ss << "      \"HmacSignature\": { \"type\": \"apiKey\", \"in\": \"header\", \"name\": \"X-Signature\" }\n";
    ss << "    }\n";
    ss << "  }\n";
    ss << "}\n";
    
    return ss.str();
}

void OpenAPISpecBuilder::addEndpoint(const std::string& operationId,
                                     const std::string& method,
                                     const std::string& path,
                                     const std::string& description,
                                     const std::map<std::string, std::string>& parameters) {
    
    std::stringstream ss;
    ss << "\"" << path << "\": {\n";
    
    std::string lowerMethod = method;
    for (auto& c : lowerMethod) c = static_cast<char>(::tolower(c));
    
    ss << "      \"" << lowerMethod << "\": {\n";
    ss << "        \"operationId\": \"" << operationId << "\",\n";
    ss << "        \"summary\": \"" << description << "\",\n";
    
    if (!parameters.empty()) {
        ss << "        \"parameters\": [\n";
        bool firstParam = true;
        for (const auto& [paramName, paramDesc] : parameters) {
            if (!firstParam) ss << ",\n";
            ss << "          {\n";
            ss << "            \"name\": \"" << paramName << "\",\n";
            ss << "            \"in\": \"query\",\n";
            ss << "            \"description\": \"" << paramDesc << "\",\n";
            ss << "            \"required\": true,\n";
            ss << "            \"schema\": { \"type\": \"string\" }\n";
            ss << "          }";
            firstParam = false;
        }
        ss << "\n        ],\n";
    }
    
    ss << "        \"responses\": {\n";
    ss << "          \"200\": {\n";
    ss << "            \"description\": \"Successful operation\"\n";
    ss << "          }\n";
    ss << "        },\n";
    ss << "        \"security\": [{ \"ApiKey\": [] }]\n";
    ss << "      }\n";
    ss << "    }";
    
    endpoints[operationId] = ss.str();
}

void OpenAPISpecBuilder::addSchema(const std::string& schemaName, const std::string& schemaJson) {
    schemas[schemaName] = schemaJson;
}

bool OpenAPISpecBuilder::exportToFile(const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) return false;
    
    file << buildSpec();
    return file.good();
}

} // namespace documentation
