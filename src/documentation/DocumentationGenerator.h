#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace documentation {

enum class DocumentFormat {
    MARKDOWN = 0,
    HTML = 1,
    PDF = 2,
    DOCX = 3,
};

struct ApiEndpointDoc {
    std::string operationId;
    std::string method; // GET, POST, PUT, DELETE
    std::string path;
    std::string summary;
    std::string description;
    std::vector<std::string> tags; // ["GPIO", "ADC", "PWM"]
    std::map<std::string, std::string> parameters; // name -> description
    std::string requestBodySchema; // JSON schema
    std::string responseSchema;
    std::vector<std::string> examples; // Code samples
};

struct TypeDoc {
    std::string typeName;
    std::string typeId;
    std::string baseType; // uint8, int32, float, struct
    std::string description;
    std::map<std::string, std::string> fields; // field name -> type
    std::vector<std::string> examples;
};

struct HardwareCompatibility {
    std::string boardName; // STM32F030R8, ESP32, RP2040
    std::string architecture;
    std::vector<std::string> supportedFeatures;
    std::map<std::string, std::string> pinMappings;
    bool fullSupport;
};

class DocumentationGenerator {
public:
    DocumentationGenerator();
    
    // Generate API documentation
    std::string generateApiDocumentation(const std::vector<ApiEndpointDoc>& endpoints,
                                         DocumentFormat format = DocumentFormat::MARKDOWN);
    
    // Generate type definitions documentation
    std::string generateTypeDocumentation(const std::vector<TypeDoc>& types,
                                          DocumentFormat format = DocumentFormat::MARKDOWN);
    
    // Generate hardware compatibility matrix
    std::string generateCompatibilityMatrix(const std::vector<HardwareCompatibility>& boards,
                                            DocumentFormat format = DocumentFormat::MARKDOWN);
    
    // Generate integration guide
    std::string generateIntegrationGuide(const std::vector<ApiEndpointDoc>& endpoints,
                                         DocumentFormat format = DocumentFormat::MARKDOWN);
    
    // Generate troubleshooting guide
    std::string generateTroubleshootingGuide(DocumentFormat format = DocumentFormat::MARKDOWN);
    
    // Generate code examples
    std::string generateCodeExamples(const std::vector<ApiEndpointDoc>& endpoints,
                                     const std::string& language); // "C", "C++", "Rust", "Python", "JS"
    
    // Export to file
    bool exportToFile(const std::string& content, const std::string& filePath);
    
private:
    std::string toMarkdown(const ApiEndpointDoc& endpoint);
    std::string toMarkdown(const TypeDoc& type);
    std::string toHtml(const std::string& markdown);
};

} // namespace documentation
