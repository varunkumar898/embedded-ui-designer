#include "CodegenEngine.h"
#include "model/TypeSystem.h"
#if defined(QT_CORE_LIB)
#include "hardware/BoardRegistry.h"
#include "driverpack/DriverPackRegistry.h"
#endif
#include <sstream>
#include <algorithm>

namespace EmbeddedUI {

// Helper to convert operationId or string to PascalCase or snake_case
static std::string toPascalCase(const std::string& input) {
    if (input.empty()) return "";
    std::string result;
    bool capitalizeNext = true;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '_' || c == '-' || c == ' ') {
            capitalizeNext = true;
        } else if (capitalizeNext) {
            result += (char)::toupper(c);
            capitalizeNext = false;
        } else if (::isupper(c) && i > 0 && !::isupper(input[i - 1])) {
            result += (char)::toupper(c);
            capitalizeNext = false;
        } else {
            result += (char)::tolower(c);
        }
    }
    return result;
}

static std::string toSnakeCase(const std::string& input) {
    if (input.empty()) return "";
    std::string result;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (::isupper(c)) {
            if (i > 0 && input[i - 1] != '_') {
                result += '_';
            }
            result += (char)::tolower(c);
        } else {
            result += c;
        }
    }
    return result;
}

// ============================================================================
// FACTORY
// ============================================================================

std::unique_ptr<CodegenEngine> createCodegen(const std::string& language) {
    std::string lowerLang = language;
    std::transform(lowerLang.begin(), lowerLang.end(), lowerLang.begin(), ::tolower);
    
    if (lowerLang == "c") {
        return std::make_unique<CCodegen>();
    } else if (lowerLang == "c++" || lowerLang == "cpp") {
        return std::make_unique<CppCodegen>();
    } else if (lowerLang == "rust" || lowerLang == "rs") {
        return std::make_unique<RustCodegen>();
    }
    return nullptr;
}

// ============================================================================
// C CODE GENERATOR
// ============================================================================

bool CCodegen::validate(const Project& project, const GenerationConfig& config, std::string& errorMessage) const {
    if (config.targetLanguage != "C" && config.targetLanguage != "c" && !config.targetLanguage.empty()) {
        errorMessage = "Invalid target language for CCodegen: " + config.targetLanguage;
        return false;
    }
    if (project.getApiSchema().getAllEndpoints().empty() && project.getTypeRegistry().getAllTypeIds().empty()) {
        errorMessage = "Project contains no endpoints or types to generate";
        return false;
    }
    return true;
}

std::string CCodegen::selectHalImports(const std::string& targetBoard) {
    std::ostringstream ss;
    if (targetBoard == "STM32F030R8" || targetBoard == "STM32F0") {
        ss << "#include <stdint.h>\n"
           << "#include <stdbool.h>\n"
           << "#include <string.h>\n"
           << "#include <stdio.h>\n"
           << "#include \"stm32f0xx_hal.h\"\n";
    } else if (targetBoard.find("STM32F4") != std::string::npos) {
        ss << "#include <stdint.h>\n"
           << "#include <stdbool.h>\n"
           << "#include <string.h>\n"
           << "#include <stdio.h>\n"
           << "#include \"stm32f4xx_hal.h\"\n";
    } else if (targetBoard.find("STM32H7") != std::string::npos) {
        ss << "#include <stdint.h>\n"
           << "#include <stdbool.h>\n"
           << "#include <string.h>\n"
           << "#include <stdio.h>\n"
           << "#include \"stm32h7xx_hal.h\"\n";
    } else if (targetBoard.find("STM32F0") != std::string::npos || targetBoard.find("F030") != std::string::npos || targetBoard.empty()) {
        ss << "#include <stdint.h>\n"
           << "#include <stdbool.h>\n"
           << "#include <string.h>\n"
           << "#include <stdio.h>\n"
           << "#include \"stm32f0xx_hal.h\"\n";
    } else if (targetBoard.find("STM32") != std::string::npos) {
        ss << "#include <stdint.h>\n"
           << "#include <stdbool.h>\n"
           << "#include <string.h>\n"
           << "#include <stdio.h>\n"
           << "#include \"stm32_hal.h\"\n";
    } else if (targetBoard == "RP2040" || targetBoard.find("RP20") != std::string::npos || targetBoard.find("Pico") != std::string::npos) {
        ss << "#include <stdint.h>\n"
           << "#include <stdbool.h>\n"
           << "#include <string.h>\n"
           << "#include <stdio.h>\n"
           << "#include \"pico/stdlib.h\"\n"
           << "#include \"hardware/gpio.h\"\n";
    } else if (targetBoard == "ESP32" || targetBoard.find("ESP32") != std::string::npos) {
        ss << "#include <stdint.h>\n"
           << "#include <stdbool.h>\n"
           << "#include <string.h>\n"
           << "#include <stdio.h>\n"
           << "#include \"driver/gpio.h\"\n"
           << "#include \"esp_timer.h\"\n";
    } else if (targetBoard.find("RT10") != std::string::npos || targetBoard.find("MIMX") != std::string::npos) {
        ss << "#include <stdint.h>\n"
           << "#include <stdbool.h>\n"
           << "#include <string.h>\n"
           << "#include <stdio.h>\n"
           << "#include \"fsl_gpio.h\"\n"
           << "#include \"fsl_common.h\"\n";
    } else if (targetBoard.find("Nordic") != std::string::npos || targetBoard.find("nRF") != std::string::npos || targetBoard.find("52840") != std::string::npos) {
        ss << "#include <stdint.h>\n"
           << "#include <stdbool.h>\n"
           << "#include <string.h>\n"
           << "#include <stdio.h>\n"
           << "#include <zephyr/kernel.h>\n"
           << "#include <zephyr/drivers/gpio.h>\n";
    } else {
        ss << "#include <stdint.h>\n"
           << "#include <stdbool.h>\n"
           << "#include <string.h>\n"
           << "#include <stdio.h>\n"
           << "#include \"hal/gpio_hal.h\"\n";
    }
    return ss.str();
}

std::string CCodegen::generateHandlerFunction(const ApiEndpoint& endpoint, const GenerationConfig& config) {
    std::ostringstream ss;
    std::string opName = endpoint.operationId.empty() ? "endpoint_handler" : endpoint.operationId;
    std::string responseType = "void*";
    for (const auto& resp : endpoint.responses) {
        if (!resp.responseTypeId.empty()) {
            responseType = resp.responseTypeId + "*";
            break;
        }
    }

    ss << "// Generated HTTP Handler: " << endpoint.path << "\n";
    ss << "#define HTTP_200_OK 200\n";
    ss << "#define HTTP_400_BAD_REQUEST 400\n";
    ss << "#define HTTP_404_NOT_FOUND 404\n";
    ss << "#define HTTP_500_INTERNAL_ERROR 500\n\n";

    ss << "int handler_" << opName << "(\n";
    
    // Parameters
    for (const auto& p : endpoint.parameters) {
        std::string cType = TypeConverter::toC(p.typeId);
        if (cType == "char*") {
            ss << "    const char* " << p.paramName << ",\n";
        } else {
            ss << "    " << cType << " " << p.paramName << ",\n";
        }
    }
    ss << "    " << responseType << " response,\n";
    ss << "    char* error_message\n";
    ss << ") {\n";

    // 1. Parameter Validation
    for (const auto& p : endpoint.parameters) {
        if (p.typeId == "string") {
            ss << "    if (!" << p.paramName << " || strlen(" << p.paramName << ") > 32) {\n";
            ss << "        if (error_message) snprintf(error_message, 256, \"Invalid " << p.paramName << "\");\n";
            ss << "        return HTTP_400_BAD_REQUEST;\n";
            ss << "    }\n";
        }
    }

    // 2. HAL invocation using active board pin map & protocols
    ss << "\n    // Hardware HAL invocation\n";
    std::string defaultGpioPin = "PA0";
    std::string fam = "STM32";

#if defined(QT_CORE_LIB)
    std::string target = config.targetBoard;
    QString targetQ = QString::fromStdString(target).trimmed();
    BoardDef activeBoard;
    if (!target.empty()) {
        BoardDef b = BoardRegistry::instance().board(targetQ);
        if (!b.isValid()) {
            for (const auto& brd : BoardRegistry::instance().boards()) {
                if (brd.mcu.compare(targetQ, Qt::CaseInsensitive) == 0 ||
                    brd.displayName.compare(targetQ, Qt::CaseInsensitive) == 0) {
                    b = brd;
                    break;
                }
            }
        }
        if (b.isValid()) {
            activeBoard = b;
            fam = activeBoard.family.toStdString();
        } else {
            auto* pack = driverpack::DriverPackRegistry::instance().findPackForDevice(target);
            if (pack) {
                fam = pack->familyId();
            } else if (targetQ.startsWith("STM32", Qt::CaseInsensitive)) {
                fam = "STM32";
            } else if (targetQ.startsWith("ESP32", Qt::CaseInsensitive)) {
                fam = "ESP32";
            } else if (targetQ.contains("RP2040", Qt::CaseInsensitive) || targetQ.contains("Pico", Qt::CaseInsensitive)) {
                fam = "RP2040";
            } else if (targetQ.contains("RT10", Qt::CaseInsensitive) || targetQ.contains("MIMX", Qt::CaseInsensitive)) {
                fam = "i.MX RT";
            } else if (targetQ.contains("nRF", Qt::CaseInsensitive) || targetQ.contains("52840", Qt::CaseInsensitive)) {
                fam = "Nordic";
            } else {
                fam = "Generic";
            }
            BoardDef synth;
            synth.family = QString::fromStdString(fam);
            BoardRegistry::instance().ensureFamilyPinDefaults(synth);
            activeBoard = synth;
        }
    } else {
        BoardDef nucleo = BoardRegistry::instance().board("nucleo_f030r8");
        if (nucleo.isValid()) activeBoard = nucleo;
        else activeBoard = BoardRegistry::instance().activeBoard();
        fam = activeBoard.family.toStdString();
    }

    for (auto it = activeBoard.pinMap.begin(); it != activeBoard.pinMap.end(); ++it) {
        if (!it.value().reserved && it.value().function == "GPIO") {
            defaultGpioPin = it.key().toStdString();
            break;
        }
    }
#endif

    if (fam == "STM32") {
        if (config.targetBoard.empty() || config.targetBoard.find("F030") != std::string::npos) {
            ss << "    #if defined(TARGET_STM32) || defined(TARGET_STM32F030R8)\n";
        } else {
            ss << "    #if defined(TARGET_STM32)\n";
        }
        ss << "    uint8_t hal_val = hal_read_digital_pin(\"" << defaultGpioPin << "\");\n";
        ss << "    uint32_t hal_tick = HAL_GetTick();\n";
        ss << "    #else\n";
        ss << "    uint8_t hal_val = 1;\n";
        ss << "    uint32_t hal_tick = 100;\n";
        ss << "    #endif\n";
    } else if (fam == "RP2040") {
        ss << "    #ifdef TARGET_RP2040\n";
        ss << "    uint8_t hal_val = hal_read_digital_pin(\"" << defaultGpioPin << "\");\n";
        ss << "    uint32_t hal_tick = 100;\n";
        ss << "    #else\n";
        ss << "    uint8_t hal_val = 1;\n";
        ss << "    uint32_t hal_tick = 100;\n";
        ss << "    #endif\n";
    } else if (fam == "ESP32") {
        ss << "    #ifdef TARGET_ESP32\n";
        ss << "    uint8_t hal_val = hal_read_digital_pin(\"" << defaultGpioPin << "\");\n";
        ss << "    uint32_t hal_tick = 100;\n";
        ss << "    #else\n";
        ss << "    uint8_t hal_val = 1;\n";
        ss << "    uint32_t hal_tick = 100;\n";
        ss << "    #endif\n";
    } else {
        ss << "    uint8_t hal_val = hal_read_digital_pin(\"" << defaultGpioPin << "\");\n";
        ss << "    uint32_t hal_tick = 100;\n";
    }

    // 3. Response population
    ss << "    if (response) {\n";
    ss << "        // Populate response object\n";
    ss << "        ((uint8_t*)response)[0] = hal_val;\n";
    ss << "    }\n";
    ss << "    return HTTP_200_OK;\n";
    ss << "}\n";

    return ss.str();
}

std::string CCodegen::generateSerializationFunctions(const TypeDefinition& type) {
    std::ostringstream ss;
    std::string typeName = type.typeId;

    if (type.isComposite) {
        ss << "typedef struct {\n";
        for (const auto& field : type.compositeFields) {
            ss << "    " << TypeConverter::toC(field.second) << " " << field.first << ";\n";
        }
        ss << "} " << typeName << ";\n\n";

        // To cJSON
        ss << "cJSON* " << toSnakeCase(typeName) << "_to_json(const " << typeName << "* resp) {\n";
        ss << "    if (!resp) return NULL;\n";
        ss << "    cJSON* obj = cJSON_CreateObject();\n";
        for (const auto& field : type.compositeFields) {
            std::string cType = TypeConverter::toC(field.second);
            if (cType.find("char*") != std::string::npos || field.second == "string") {
                ss << "    cJSON_AddStringToObject(obj, \"" << field.first << "\", resp->" << field.first << ");\n";
            } else {
                ss << "    cJSON_AddNumberToObject(obj, \"" << field.first << "\", resp->" << field.first << ");\n";
            }
        }
        ss << "    return obj;\n";
        ss << "}\n\n";

        // From cJSON
        ss << "int " << toSnakeCase(typeName) << "_from_json(const cJSON* json, " << typeName << "* req) {\n";
        ss << "    if (!json || !req) return -1;\n";
        for (const auto& field : type.compositeFields) {
            std::string cType = TypeConverter::toC(field.second);
            ss << "    cJSON* item_" << field.first << " = cJSON_GetObjectItem(json, \"" << field.first << "\");\n";
            if (cType.find("char*") != std::string::npos || field.second == "string") {
                ss << "    if (item_" << field.first << " && cJSON_IsString(item_" << field.first << ")) {\n";
                ss << "        req->" << field.first << " = item_" << field.first << "->valuestring;\n";
                ss << "    }\n";
            } else {
                ss << "    if (item_" << field.first << " && cJSON_IsNumber(item_" << field.first << ")) {\n";
                ss << "        req->" << field.first << " = (" << cType << ")item_" << field.first << "->valuedouble;\n";
                ss << "    }\n";
            }
        }
        ss << "    return 0;\n";
        ss << "}\n";
    }

    return ss.str();
}

std::vector<GeneratedFile> CCodegen::generate(const Project& project, const GenerationConfig& config, std::string& errorMessage) {
    if (!validate(project, config, errorMessage)) {
        return {};
    }

    std::vector<GeneratedFile> files;

    // 1. Generate types header and cJSON serialization
    std::ostringstream typesH;
    typesH << "/* Auto-generated by EmbeddedUI C Code Generator */\n";
    typesH << "#pragma once\n\n";
    typesH << "#include <stdint.h>\n#include <stdbool.h>\n#include <stddef.h>\n\n";
    typesH << "/* Forward declaration of cJSON */\ntypedef struct cJSON cJSON;\n\n";

    std::ostringstream typesC;
    typesC << "/* Auto-generated by EmbeddedUI C Code Generator */\n";
    typesC << "#include \"types.h\"\n";
    typesC << "#include \"cJSON.h\"\n\n";

    for (const auto& typeId : project.getTypeRegistry().getAllTypeIds()) {
        try {
            TypeDefinition t = project.getTypeRegistry().getType(typeId);
            if (t.isComposite) {
                typesH << generateSerializationFunctions(t) << "\n";
            }
        } catch (...) {}
    }

    files.push_back({"types.h", typesH.str(), "C"});

    // 2. Generate Handlers per endpoint
    for (const auto& endpoint : project.getApiSchema().getAllEndpoints()) {
        std::ostringstream handlerSrc;
        handlerSrc << "/* Auto-generated by EmbeddedUI C Code Generator */\n";
        handlerSrc << selectHalImports(config.targetBoard) << "\n";
        handlerSrc << "#include \"types.h\"\n\n";
        handlerSrc << generateHandlerFunction(endpoint, config);

        std::string fileName = "handler_" + (endpoint.operationId.empty() ? endpoint.endpointId : endpoint.operationId) + ".c";
        files.push_back({fileName, handlerSrc.str(), "C"});
    }

    // 3. API Server Router
    std::ostringstream serverSrc;
    serverSrc << "/* Auto-generated API Server Router */\n";
    serverSrc << "#include \"types.h\"\n";
    serverSrc << "#include <stdio.h>\n\n";
    serverSrc << "void init_api_server(void) {\n";
    serverSrc << "    printf(\"API Server Initialized for board: " << config.targetBoard << "\\n\");\n";
    serverSrc << "}\n";
    files.push_back({"api_server.c", serverSrc.str(), "C"});

    return files;
}

// ============================================================================
// C++ CODE GENERATOR
// ============================================================================

bool CppCodegen::validate(const Project& project, const GenerationConfig& config, std::string& errorMessage) const {
    if (config.targetLanguage != "C++" && config.targetLanguage != "cpp" && config.targetLanguage != "c++" && !config.targetLanguage.empty()) {
        errorMessage = "Invalid target language for CppCodegen: " + config.targetLanguage;
        return false;
    }
    if (project.getApiSchema().getAllEndpoints().empty() && project.getTypeRegistry().getAllTypeIds().empty()) {
        errorMessage = "Project contains no endpoints or types to generate";
        return false;
    }
    return true;
}

std::string CppCodegen::generateDtoClass(const TypeDefinition& type) {
    std::ostringstream ss;
    std::string typeName = toPascalCase(type.typeId);

    ss << "struct " << typeName << " {\n";
    if (type.isComposite) {
        for (const auto& field : type.compositeFields) {
            ss << "    " << TypeConverter::toCpp(field.second) << " " << field.first << "{};\n";
        }
    }
    ss << "\n";

    // toJson
    ss << "    nlohmann::json toJson() const {\n";
    ss << "        nlohmann::json j;\n";
    if (type.isComposite) {
        for (const auto& field : type.compositeFields) {
            ss << "        j[\"" << field.first << "\"] = " << field.first << ";\n";
        }
    }
    ss << "        return j;\n";
    ss << "    }\n\n";

    // fromJson
    ss << "    static " << typeName << " fromJson(const nlohmann::json& j) {\n";
    ss << "        " << typeName << " obj;\n";
    if (type.isComposite) {
        for (const auto& field : type.compositeFields) {
            ss << "        if (j.contains(\"" << field.first << "\")) obj." << field.first 
               << " = j.at(\"" << field.first << "\").get<" << TypeConverter::toCpp(field.second) << ">();\n";
        }
    }
    ss << "        return obj;\n";
    ss << "    }\n";
    ss << "};\n";

    return ss.str();
}

std::string CppCodegen::generateJsonSerializationHooks(const TypeDefinition& type) {
    std::ostringstream ss;
    std::string typeName = toPascalCase(type.typeId);
    ss << "inline void to_json(nlohmann::json& j, const " << typeName << "& p) { j = p.toJson(); }\n";
    ss << "inline void from_json(const nlohmann::json& j, " << typeName << "& p) { p = " << typeName << "::fromJson(j); }\n";
    return ss.str();
}

std::string CppCodegen::generateHandlerClass(const ApiEndpoint& endpoint, const GenerationConfig& config) {
    std::ostringstream ss;
    std::string opName = toPascalCase(endpoint.operationId.empty() ? endpoint.endpointId : endpoint.operationId);
    std::string responseType = "void";
    for (const auto& resp : endpoint.responses) {
        if (!resp.responseTypeId.empty()) {
            responseType = toPascalCase(resp.responseTypeId);
            break;
        }
    }

    ss << "namespace EmbeddedUI::Handlers {\n\n";
    
    // HalInterface with dependency injection
    ss << "class HalInterface {\n";
    ss << "public:\n";
    ss << "    virtual ~HalInterface() = default;\n";
    ss << "    virtual std::optional<uint8_t> gpio_read(uint8_t pin) = 0;\n";
    ss << "    virtual uint32_t get_tick() const = 0;\n";
    ss << "};\n\n";

    // Handler Class
    ss << "class " << opName << "Handler {\n";
    ss << "private:\n";
    ss << "    HalInterface& hal;\n\n";
    ss << "public:\n";
    ss << "    explicit " << opName << "Handler(HalInterface& hal) : hal(hal) {}\n\n";

    if (config.enableAsyncAwait) {
        ss << "    // Async handler mode\n";
        ss << "    std::future<Result<" << responseType << ">> handleAsync(const " << opName << "Request& req) {\n";
        ss << "        return std::async(std::launch::async, [this, req]() {\n";
        ss << "            return handle(req);\n";
        ss << "        });\n";
        ss << "    }\n\n";
    }

    ss << "    Result<" << responseType << "> handle(const " << opName << "Request& req) {\n";
    ss << "        if (req.deviceId.length() > 32) {\n";
    ss << "            return Result<" << responseType << ">::Error(\"Invalid deviceId\");\n";
    ss << "        }\n";
    ss << "        auto val = hal.gpio_read(req.pinNumber);\n";
    ss << "        if (!val) {\n";
    ss << "            return Result<" << responseType << ">::Error(\"HAL read failed\");\n";
    ss << "        }\n";
    if (responseType != "void") {
        ss << "        " << responseType << " resp;\n";
        ss << "        resp.value = *val;\n";
        ss << "        resp.timestamp = hal.get_tick();\n";
        ss << "        return Result<" << responseType << ">::Ok(resp);\n";
    } else {
        ss << "        return Result<void>::Ok();\n";
    }
    ss << "    }\n";
    ss << "};\n\n";
    ss << "} // namespace EmbeddedUI::Handlers\n";

    return ss.str();
}

std::vector<GeneratedFile> CppCodegen::generate(const Project& project, const GenerationConfig& config, std::string& errorMessage) {
    if (!validate(project, config, errorMessage)) {
        return {};
    }

    std::vector<GeneratedFile> files;

    // 1. Types.hpp
    std::ostringstream typesH;
    typesH << "/* Auto-generated by EmbeddedUI C++ Code Generator */\n";
    typesH << "#pragma once\n\n";
    typesH << "#include <string>\n#include <vector>\n#include <optional>\n#include <cstdint>\n";
    typesH << "#include <nlohmann/json.hpp>\n\n";
    typesH << "template<typename T>\n";
    typesH << "struct Result {\n";
    typesH << "    bool isOk{true};\n";
    typesH << "    T value{};\n";
    typesH << "    std::string errorMessage{};\n";
    typesH << "    static Result Ok(T val) { Result r; r.isOk = true; r.value = val; return r; }\n";
    typesH << "    static Result Error(const std::string& err) { Result r; r.isOk = false; r.errorMessage = err; return r; }\n";
    typesH << "};\n\n";

    for (const auto& typeId : project.getTypeRegistry().getAllTypeIds()) {
        try {
            TypeDefinition t = project.getTypeRegistry().getType(typeId);
            if (t.isComposite) {
                typesH << generateDtoClass(t) << "\n";
                typesH << generateJsonSerializationHooks(t) << "\n";
            }
        } catch (...) {}
    }

    files.push_back({"Types.hpp", typesH.str(), "C++"});

    // 2. Handlers per endpoint
    for (const auto& endpoint : project.getApiSchema().getAllEndpoints()) {
        std::ostringstream handlerSrc;
        handlerSrc << "/* Auto-generated C++ Handler */\n";
        handlerSrc << "#pragma once\n";
        handlerSrc << "#include \"Types.hpp\"\n";
        if (config.enableAsyncAwait) {
            handlerSrc << "#include <future>\n";
        }
        if (config.targetBoard == "ESP32") {
            handlerSrc << "// ESP-IDF HAL wrappers\n";
            handlerSrc << "#include <driver/gpio.h>\n";
        } else if (config.targetBoard.find("STM32") != std::string::npos) {
            handlerSrc << "// STM32 HAL wrappers\n";
            handlerSrc << "#include <stm32_hal.h>\n";
        }

        std::string opName = toPascalCase(endpoint.operationId.empty() ? endpoint.endpointId : endpoint.operationId);
        // Request DTO
        handlerSrc << "struct " << opName << "Request {\n";
        for (const auto& p : endpoint.parameters) {
            handlerSrc << "    " << TypeConverter::toCpp(p.typeId) << " " << p.paramName << "{};\n";
        }
        handlerSrc << "};\n\n";

        handlerSrc << generateHandlerClass(endpoint, config);

        std::string fileName = opName + "Handler.hpp";
        files.push_back({fileName, handlerSrc.str(), "C++"});
    }

    return files;
}

// ============================================================================
// RUST CODE GENERATOR
// ============================================================================

bool RustCodegen::validate(const Project& project, const GenerationConfig& config, std::string& errorMessage) const {
    if (config.targetLanguage != "Rust" && config.targetLanguage != "rust" && config.targetLanguage != "rs" && !config.targetLanguage.empty()) {
        errorMessage = "Invalid target language for RustCodegen: " + config.targetLanguage;
        return false;
    }
    if (project.getApiSchema().getAllEndpoints().empty() && project.getTypeRegistry().getAllTypeIds().empty()) {
        errorMessage = "Project contains no endpoints or types to generate";
        return false;
    }
    return true;
}

std::string RustCodegen::generateDtoStruct(const TypeDefinition& type) {
    std::ostringstream ss;
    std::string typeName = toPascalCase(type.typeId);

    ss << "#[derive(Debug, Serialize, Deserialize, Clone, PartialEq)]\n";
    ss << "pub struct " << typeName << " {\n";
    if (type.isComposite) {
        for (const auto& field : type.compositeFields) {
            ss << "    pub " << toSnakeCase(field.first) << ": " << TypeConverter::toRust(field.second) << ",\n";
        }
    }
    ss << "}\n";

    return ss.str();
}

std::string RustCodegen::generateHalTraitImpl(const std::string& targetBoard) {
    std::ostringstream ss;
    ss << "pub trait HalInterface {\n";
    ss << "    fn gpio_read(&mut self, pin: u8) -> Result<u8, &'static str>;\n";
    ss << "    fn get_tick(&self) -> u32;\n";
    ss << "}\n\n";

    if (targetBoard == "RP2040") {
        ss << "// Target: RP2040 using rp2040-hal\n";
        ss << "pub struct Rp2040HalImpl;\n";
        ss << "impl HalInterface for Rp2040HalImpl {\n";
        ss << "    fn gpio_read(&mut self, _pin: u8) -> Result<u8, &'static str> { Ok(1) }\n";
        ss << "    fn get_tick(&self) -> u32 { 1000 }\n";
        ss << "}\n";
    } else if (targetBoard == "ESP32" || targetBoard == "ESP32-C3") {
        ss << "// Target: ESP32 using esp-idf-hal\n";
        ss << "pub struct Esp32HalImpl;\n";
        ss << "impl HalInterface for Esp32HalImpl {\n";
        ss << "    fn gpio_read(&mut self, _pin: u8) -> Result<u8, &'static str> { Ok(1) }\n";
        ss << "    fn get_tick(&self) -> u32 { 1000 }\n";
        ss << "}\n";
    } else {
        ss << "// Generic embedded-hal trait wrapper\n";
        ss << "pub struct GenericHalImpl;\n";
        ss << "impl HalInterface for GenericHalImpl {\n";
        ss << "    fn gpio_read(&mut self, _pin: u8) -> Result<u8, &'static str> { Ok(1) }\n";
        ss << "    fn get_tick(&self) -> u32 { 1000 }\n";
        ss << "}\n";
    }

    return ss.str();
}

std::string RustCodegen::generateHandlerImpl(const ApiEndpoint& endpoint, const GenerationConfig& config) {
    std::ostringstream ss;
    std::string opName = toPascalCase(endpoint.operationId.empty() ? endpoint.endpointId : endpoint.operationId);
    std::string responseType = "()";
    for (const auto& resp : endpoint.responses) {
        if (!resp.responseTypeId.empty()) {
            responseType = toPascalCase(resp.responseTypeId);
            break;
        }
    }

    // Error Enum
    ss << "#[derive(Debug, thiserror::Error)]\n";
    ss << "pub enum " << opName << "Error {\n";
    ss << "    #[error(\"Invalid device ID\")]\n";
    ss << "    InvalidDeviceId,\n";
    ss << "    #[error(\"HAL operation failed: {0}\")]\n";
    ss << "    HalError(String),\n";
    ss << "}\n\n";

    // Handler Struct
    ss << "pub struct " << opName << "Handler<H: HalInterface> {\n";
    ss << "    hal: H,\n";
    ss << "}\n\n";

    ss << "impl<H: HalInterface> " << opName << "Handler<H> {\n";
    ss << "    pub fn new(hal: H) -> Self {\n";
    ss << "        Self { hal }\n";
    ss << "    }\n\n";

    if (config.enableAsyncAwait) {
        ss << "    pub async fn handle(&mut self, req: " << opName << "Request) -> Result<" << responseType << ", " << opName << "Error> {\n";
    } else {
        ss << "    pub fn handle(&mut self, req: " << opName << "Request) -> Result<" << responseType << ", " << opName << "Error> {\n";
    }
    ss << "        if req.device_id.len() > 32 {\n";
    ss << "            return Err(" << opName << "Error::InvalidDeviceId);\n";
    ss << "        }\n";
    ss << "        let val = self.hal.gpio_read(req.pin_number)\n";
    ss << "            .map_err(|e| " << opName << "Error::HalError(e.to_string()))?;\n";
    ss << "        let tick = self.hal.get_tick();\n";
    if (responseType != "()") {
        ss << "        Ok(" << responseType << " {\n";
        ss << "            value: val,\n";
        ss << "            timestamp: tick,\n";
        ss << "        })\n";
    } else {
        ss << "        Ok(())\n";
    }
    ss << "    }\n";
    ss << "}\n";

    return ss.str();
}

std::vector<GeneratedFile> RustCodegen::generate(const Project& project, const GenerationConfig& config, std::string& errorMessage) {
    if (!validate(project, config, errorMessage)) {
        return {};
    }

    std::vector<GeneratedFile> files;

    // 1. types.rs
    std::ostringstream typesRs;
    typesRs << "//! Auto-generated by EmbeddedUI Rust Code Generator\n";
    typesRs << "use serde::{Deserialize, Serialize};\n\n";

    for (const auto& typeId : project.getTypeRegistry().getAllTypeIds()) {
        try {
            TypeDefinition t = project.getTypeRegistry().getType(typeId);
            if (t.isComposite) {
                typesRs << generateDtoStruct(t) << "\n";
            }
        } catch (...) {}
    }
    files.push_back({"types.rs", typesRs.str(), "Rust"});

    // 2. hal.rs
    std::ostringstream halRs;
    halRs << "//! Target HAL Trait & Implementation\n";
    halRs << generateHalTraitImpl(config.targetBoard);
    files.push_back({"hal.rs", halRs.str(), "Rust"});

    // 3. handlers.rs
    std::ostringstream handlersRs;
    handlersRs << "//! API Endpoint Handlers\n";
    handlersRs << "use crate::types::*;\n";
    handlersRs << "use crate::hal::*;\n\n";

    for (const auto& endpoint : project.getApiSchema().getAllEndpoints()) {
        std::string opName = toPascalCase(endpoint.operationId.empty() ? endpoint.endpointId : endpoint.operationId);

        // Request struct
        handlersRs << "#[derive(Debug, Serialize, Deserialize, Clone)]\n";
        handlersRs << "pub struct " << opName << "Request {\n";
        for (const auto& p : endpoint.parameters) {
            handlersRs << "    pub " << toSnakeCase(p.paramName) << ": " << TypeConverter::toRust(p.typeId) << ",\n";
        }
        handlersRs << "}\n\n";

        handlersRs << generateHandlerImpl(endpoint, config) << "\n";
    }
    files.push_back({"handlers.rs", handlersRs.str(), "Rust"});

    // 4. lib.rs
    std::ostringstream libRs;
    libRs << "pub mod types;\n";
    libRs << "pub mod hal;\n";
    libRs << "pub mod handlers;\n";
    files.push_back({"lib.rs", libRs.str(), "Rust"});

    return files;
}

} // namespace EmbeddedUI
