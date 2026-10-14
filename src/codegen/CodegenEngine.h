#pragma once

#include "model/ProjectModel.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace EmbeddedUI {

struct GeneratedFile {
    std::string fileName;      // e.g. "gpio_read_handler.c"
    std::string fileContent;   // Complete source code
    std::string language;      // "C", "C++", "Rust"
};

struct GenerationConfig {
    std::string targetLanguage;    // "C", "C++", "Rust"
    std::string targetBoard;       // "STM32F030R8", "ESP32", "RP2040"
    std::string halLibrary;        // "stm32cube", "esp-idf", "rp2040-hal", "embedded-hal"
    bool generateTests = false;
    bool generateMocks = false;
    bool enableAsyncAwait = false;
    std::vector<std::string> customDefines;
};

class CodegenEngine {
public:
    virtual ~CodegenEngine() = default;
    
    virtual std::vector<GeneratedFile> generate(
        const Project& project,
        const GenerationConfig& config,
        std::string& errorMessage
    ) = 0;
    
    // Validation before generation
    virtual bool validate(
        const Project& project,
        const GenerationConfig& config,
        std::string& errorMessage
    ) const = 0;
};

class CCodegen : public CodegenEngine {
public:
    std::vector<GeneratedFile> generate(
        const Project& project,
        const GenerationConfig& config,
        std::string& errorMessage
    ) override;
    
    bool validate(
        const Project& project,
        const GenerationConfig& config,
        std::string& errorMessage
    ) const override;
    
    std::string generateHandlerFunction(const ApiEndpoint& endpoint, 
                                        const GenerationConfig& config);
    std::string generateSerializationFunctions(const TypeDefinition& type);
    std::string selectHalImports(const std::string& targetBoard);
};

class CppCodegen : public CodegenEngine {
public:
    std::vector<GeneratedFile> generate(
        const Project& project,
        const GenerationConfig& config,
        std::string& errorMessage
    ) override;
    
    bool validate(
        const Project& project,
        const GenerationConfig& config,
        std::string& errorMessage
    ) const override;
    
    std::string generateDtoClass(const TypeDefinition& type);
    std::string generateHandlerClass(const ApiEndpoint& endpoint,
                                     const GenerationConfig& config);
    std::string generateJsonSerializationHooks(const TypeDefinition& type);
};

class RustCodegen : public CodegenEngine {
public:
    std::vector<GeneratedFile> generate(
        const Project& project,
        const GenerationConfig& config,
        std::string& errorMessage
    ) override;
    
    bool validate(
        const Project& project,
        const GenerationConfig& config,
        std::string& errorMessage
    ) const override;
    
    std::string generateDtoStruct(const TypeDefinition& type);
    std::string generateHandlerImpl(const ApiEndpoint& endpoint,
                                   const GenerationConfig& config);
    std::string generateHalTraitImpl(const std::string& targetBoard);
};

// Factory
std::unique_ptr<CodegenEngine> createCodegen(const std::string& language);

} // namespace EmbeddedUI
