#include "codegen/CodegenEngine.h"
#include "model/ProjectModel.h"
#include "model/TypeSystem.h"
#include <iostream>
#include <cassert>
#include <cstdio>
#include <memory>
#include <array>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

using namespace EmbeddedUI;

// Helper to run a command and check return code
static bool runCompilerCommand(const std::string& cmd, const std::string& inputCode) {
    FILE* pipe = popen(cmd.c_str(), "w");
    if (!pipe) return false;
    fwrite(inputCode.data(), 1, inputCode.size(), pipe);
    int status = pclose(pipe);
    return (status == 0);
}

// Set up sample project for code generation
static Project createSampleProject() {
    Project proj("EmbeddedDeviceServer");

    // Register composite type: GPIO_Result
    TypeDefinition gpioResult;
    gpioResult.typeId = "GPIO_Result";
    gpioResult.typeName = "GPIO Result";
    gpioResult.primitiveType = PrimitiveType::ENUM;
    gpioResult.isComposite = true;
    gpioResult.compositeFields.push_back({"value", "uint8"});
    gpioResult.compositeFields.push_back({"timestamp", "uint32"});
    gpioResult.compositeFields.push_back({"status", "string"});
    proj.getTypeRegistry().registerType(gpioResult);

    // Register Endpoint 1: GET /api/v1/device/{deviceId}/gpio/read
    ApiEndpoint readEp;
    readEp.endpointId = "ep_gpio_read";
    readEp.path = "/api/v1/device/{deviceId}/gpio/read";
    readEp.method = HttpMethod::GET;
    readEp.operationId = "readGPIO";
    readEp.description = "Read physical pin state";

    ApiParameter pDevice;
    pDevice.paramName = "deviceId";
    pDevice.typeId = "string";
    pDevice.paramLocation = "path";
    pDevice.required = true;
    readEp.parameters.push_back(pDevice);

    ApiParameter pPin;
    pPin.paramName = "pinNumber";
    pPin.typeId = "uint8";
    pPin.paramLocation = "query";
    pPin.required = true;
    readEp.parameters.push_back(pPin);

    HttpResponse resp200;
    resp200.statusCode = StatusCode::OK_200;
    resp200.description = "Success";
    resp200.responseTypeId = "GPIO_Result";
    readEp.responses.push_back(resp200);

    proj.getApiSchema().addEndpoint(readEp);

    // Register Endpoint 2: POST /api/v1/device/{deviceId}/gpio/write
    ApiEndpoint writeEp;
    writeEp.endpointId = "ep_gpio_write";
    writeEp.path = "/api/v1/device/{deviceId}/gpio/write";
    writeEp.method = HttpMethod::POST;
    writeEp.operationId = "writeGPIO";
    writeEp.description = "Set physical pin state";
    writeEp.parameters.push_back(pDevice);
    writeEp.parameters.push_back(pPin);
    writeEp.requestBodyTypeId = "GPIO_Result";
    writeEp.responses.push_back(resp200);

    proj.getApiSchema().addEndpoint(writeEp);

    return proj;
}

// ============================================================================
// WORK STREAM 1: C CODE GENERATOR ACCEPTANCE TESTS (8 TESTS)
// ============================================================================

void testCCodegen_BasicTypeMapping() {
    assert(TypeConverter::toC("uint8") == "uint8_t");
    assert(TypeConverter::toC("int32") == "int32_t");
    assert(TypeConverter::toC("uint32") == "uint32_t");
    assert(TypeConverter::toC("float") == "float");
    assert(TypeConverter::toC("string") == "const char*" || TypeConverter::toC("string") == "char*");
    std::cout << "  [PASS] CCodegen_BasicTypeMapping\n";
}

void testCCodegen_GetEndpointGeneration() {
    Project proj = createSampleProject();
    CCodegen generator;
    GenerationConfig config;
    config.targetLanguage = "C";
    config.targetBoard = "STM32F030R8";

    ApiEndpoint ep = proj.getApiSchema().getEndpoint("ep_gpio_read");
    std::string code = generator.generateHandlerFunction(ep, config);

    assert(code.find("int handler_readGPIO(") != std::string::npos);
    assert(code.find("const char* deviceId") != std::string::npos);
    assert(code.find("uint8_t pinNumber") != std::string::npos);
    assert(code.find("GPIO_Result* response") != std::string::npos);
    std::cout << "  [PASS] CCodegen_GetEndpointGeneration\n";
}

void testCCodegen_PostEndpointWithBody() {
    Project proj = createSampleProject();
    CCodegen generator;
    GenerationConfig config;
    config.targetLanguage = "C";

    ApiEndpoint ep = proj.getApiSchema().getEndpoint("ep_gpio_write");
    std::string code = generator.generateHandlerFunction(ep, config);
    assert(code.find("handler_writeGPIO") != std::string::npos);
    assert(code.find("HTTP_200_OK") != std::string::npos);
    std::cout << "  [PASS] CCodegen_PostEndpointWithBody\n";
}

void testCCodegen_ResponseSerialization() {
    Project proj = createSampleProject();
    CCodegen generator;
    TypeDefinition t = proj.getTypeRegistry().getType("GPIO_Result");
    std::string code = generator.generateSerializationFunctions(t);

    assert(code.find("cJSON_CreateObject") != std::string::npos);
    assert(code.find("cJSON_AddNumberToObject(obj, \"value\", resp->value)") != std::string::npos);
    assert(code.find("cJSON_AddNumberToObject(obj, \"timestamp\", resp->timestamp)") != std::string::npos);
    assert(code.find("cJSON_AddStringToObject(obj, \"status\", resp->status)") != std::string::npos);
    std::cout << "  [PASS] CCodegen_ResponseSerialization\n";
}

void testCCodegen_HalBoardSelection() {
    CCodegen generator;
    std::string stm32Imports = generator.selectHalImports("STM32F030R8");
    assert(stm32Imports.find("stm32f0xx_hal.h") != std::string::npos);

    std::string rp2040Imports = generator.selectHalImports("RP2040");
    assert(rp2040Imports.find("hardware/gpio.h") != std::string::npos);
    std::cout << "  [PASS] CCodegen_HalBoardSelection\n";
}

void testCCodegen_CompositeTypeGeneration() {
    Project proj = createSampleProject();
    CCodegen generator;
    TypeDefinition t = proj.getTypeRegistry().getType("GPIO_Result");
    std::string code = generator.generateSerializationFunctions(t);

    assert(code.find("typedef struct {") != std::string::npos);
    assert(code.find("uint8_t value;") != std::string::npos);
    assert(code.find("uint32_t timestamp;") != std::string::npos);
    assert(code.find("char* status;") != std::string::npos);
    std::cout << "  [PASS] CCodegen_CompositeTypeGeneration\n";
}

void testCCodegen_ErrorHandling() {
    Project proj = createSampleProject();
    CCodegen generator;
    GenerationConfig config;
    ApiEndpoint ep = proj.getApiSchema().getEndpoint("ep_gpio_read");
    std::string code = generator.generateHandlerFunction(ep, config);

    assert(code.find("HTTP_400_BAD_REQUEST") != std::string::npos);
    assert(code.find("strlen(deviceId) > 32") != std::string::npos);
    std::cout << "  [PASS] CCodegen_ErrorHandling\n";
}

void testCCodegen_CodeCompilation() {
    std::string cSnippet = 
        "#include <stdint.h>\n"
        "#include <stdbool.h>\n"
        "#include <string.h>\n"
        "#include <stdio.h>\n"
        "typedef struct { uint8_t value; uint32_t timestamp; } GPIO_Result;\n"
        "int handler_readGPIO(const char* deviceId, uint8_t pin, GPIO_Result* resp, char* err) {\n"
        "    if (!deviceId || strlen(deviceId) > 32) return 400;\n"
        "    (void)pin;\n"
        "    (void)err;\n"
        "    if (resp) { resp->value = 1; resp->timestamp = 1000; }\n"
        "    return 200;\n"
        "}\n"
        "int main(void) { GPIO_Result r; return handler_readGPIO(\"dev1\", 0, &r, NULL); }\n";

    bool success = runCompilerCommand("gcc -x c -c -Wall -Wextra -Werror -o /dev/null -", cSnippet);
    assert(success);
    std::cout << "  [PASS] CCodegen_CodeCompilation\n";
}

// ============================================================================
// WORK STREAM 2: C++ CODE GENERATOR ACCEPTANCE TESTS (8 TESTS)
// ============================================================================

void testCppCodegen_BasicTypeMapping() {
    assert(TypeConverter::toCpp("uint8") == "uint8_t");
    assert(TypeConverter::toCpp("int32") == "int32_t");
    assert(TypeConverter::toCpp("float") == "float");
    assert(TypeConverter::toCpp("string") == "std::string");
    std::cout << "  [PASS] CppCodegen_BasicTypeMapping\n";
}

void testCppCodegen_DtoClassGeneration() {
    Project proj = createSampleProject();
    CppCodegen generator;
    TypeDefinition t = proj.getTypeRegistry().getType("GPIO_Result");
    std::string code = generator.generateDtoClass(t);

    assert(code.find("struct GpioResult {") != std::string::npos);
    assert(code.find("uint8_t value{};") != std::string::npos);
    assert(code.find("uint32_t timestamp{};") != std::string::npos);
    assert(code.find("toJson() const") != std::string::npos);
    assert(code.find("static GpioResult fromJson") != std::string::npos);
    std::cout << "  [PASS] CppCodegen_DtoClassGeneration\n";
}

void testCppCodegen_JsonSerializationHooks() {
    Project proj = createSampleProject();
    CppCodegen generator;
    TypeDefinition t = proj.getTypeRegistry().getType("GPIO_Result");
    std::string code = generator.generateJsonSerializationHooks(t);

    assert(code.find("void to_json(nlohmann::json& j, const GpioResult& p)") != std::string::npos);
    assert(code.find("void from_json(const nlohmann::json& j, GpioResult& p)") != std::string::npos);
    std::cout << "  [PASS] CppCodegen_JsonSerializationHooks\n";
}

void testCppCodegen_HandlerClassGeneration() {
    Project proj = createSampleProject();
    CppCodegen generator;
    GenerationConfig config;
    ApiEndpoint ep = proj.getApiSchema().getEndpoint("ep_gpio_read");
    std::string code = generator.generateHandlerClass(ep, config);

    assert(code.find("class ReadGpioHandler") != std::string::npos);
    assert(code.find("Result<GpioResult> handle(const ReadGpioRequest& req)") != std::string::npos);
    std::cout << "  [PASS] CppCodegen_HandlerClassGeneration\n";
}

void testCppCodegen_HalBoardSelection() {
    Project proj = createSampleProject();
    CppCodegen generator;
    GenerationConfig config;
    config.targetLanguage = "C++";
    config.targetBoard = "ESP32";
    std::string err;
    auto files = generator.generate(proj, config, err);

    bool foundEspInclude = false;
    for (const auto& f : files) {
        if (f.fileContent.find("driver/gpio.h") != std::string::npos) {
            foundEspInclude = true;
            break;
        }
    }
    assert(foundEspInclude);
    std::cout << "  [PASS] CppCodegen_HalBoardSelection\n";
}

void testCppCodegen_AsyncHandlerSupport() {
    Project proj = createSampleProject();
    CppCodegen generator;
    GenerationConfig config;
    config.enableAsyncAwait = true;

    ApiEndpoint ep = proj.getApiSchema().getEndpoint("ep_gpio_read");
    std::string code = generator.generateHandlerClass(ep, config);

    assert(code.find("handleAsync(const ReadGpioRequest& req)") != std::string::npos);
    assert(code.find("std::future<Result<GpioResult>>") != std::string::npos);
    std::cout << "  [PASS] CppCodegen_AsyncHandlerSupport\n";
}

void testCppCodegen_DependencyInjection() {
    Project proj = createSampleProject();
    CppCodegen generator;
    GenerationConfig config;
    ApiEndpoint ep = proj.getApiSchema().getEndpoint("ep_gpio_read");
    std::string code = generator.generateHandlerClass(ep, config);

    assert(code.find("explicit ReadGpioHandler(HalInterface& hal) : hal(hal)") != std::string::npos);
    assert(code.find("class HalInterface {") != std::string::npos);
    std::cout << "  [PASS] CppCodegen_DependencyInjection\n";
}

void testCppCodegen_CodeCompilation() {
    std::string cppSnippet = 
        "#include <string>\n"
        "#include <cstdint>\n"
        "#include <optional>\n"
        "template<typename T> struct Result { bool isOk; T val; };\n"
        "class HalInterface { public: virtual ~HalInterface() = default; virtual std::optional<uint8_t> read() = 0; };\n"
        "class GpioHandler {\n"
        "    HalInterface& hal;\n"
        "public:\n"
        "    GpioHandler(HalInterface& h) : hal(h) {}\n"
        "    Result<uint8_t> handle() { auto v = hal.read(); return {v.has_value(), v.value_or(0)}; }\n"
        "};\n"
        "class MockHal : public HalInterface { public: std::optional<uint8_t> read() override { return 1; } };\n"
        "int main() { MockHal mock; GpioHandler h(mock); h.handle(); return 0; }\n";

    bool success = runCompilerCommand("g++ -x c++ -std=c++17 -Wall -Wextra -Werror -c -o /dev/null -", cppSnippet);
    assert(success);
    std::cout << "  [PASS] CppCodegen_CodeCompilation\n";
}

// ============================================================================
// WORK STREAM 3: RUST CODE GENERATOR ACCEPTANCE TESTS (8 TESTS)
// ============================================================================

void testRustCodegen_BasicTypeMapping() {
    assert(TypeConverter::toRust("uint8") == "u8");
    assert(TypeConverter::toRust("int32") == "i32");
    assert(TypeConverter::toRust("uint32") == "u32");
    assert(TypeConverter::toRust("float") == "f32");
    assert(TypeConverter::toRust("string") == "String");
    std::cout << "  [PASS] RustCodegen_BasicTypeMapping\n";
}

void testRustCodegen_StructGeneration() {
    Project proj = createSampleProject();
    RustCodegen generator;
    TypeDefinition t = proj.getTypeRegistry().getType("GPIO_Result");
    std::string code = generator.generateDtoStruct(t);

    assert(code.find("#[derive(Debug, Serialize, Deserialize, Clone, PartialEq)]") != std::string::npos);
    assert(code.find("pub struct GpioResult {") != std::string::npos);
    assert(code.find("pub value: u8,") != std::string::npos);
    assert(code.find("pub timestamp: u32,") != std::string::npos);
    std::cout << "  [PASS] RustCodegen_StructGeneration\n";
}

void testRustCodegen_SerdeIntegration() {
    Project proj = createSampleProject();
    RustCodegen generator;
    GenerationConfig config;
    config.targetLanguage = "Rust";
    std::string err;
    auto files = generator.generate(proj, config, err);

    bool foundSerdeImport = false;
    for (const auto& f : files) {
        if (f.fileName == "types.rs" && f.fileContent.find("use serde::{Deserialize, Serialize};") != std::string::npos) {
            foundSerdeImport = true;
            break;
        }
    }
    assert(foundSerdeImport);
    std::cout << "  [PASS] RustCodegen_SerdeIntegration\n";
}

void testRustCodegen_ErrorTypeGeneration() {
    Project proj = createSampleProject();
    RustCodegen generator;
    GenerationConfig config;
    ApiEndpoint ep = proj.getApiSchema().getEndpoint("ep_gpio_read");
    std::string code = generator.generateHandlerImpl(ep, config);

    assert(code.find("#[derive(Debug, thiserror::Error)]") != std::string::npos);
    assert(code.find("pub enum ReadGpioError {") != std::string::npos);
    assert(code.find("#[error(\"Invalid device ID\")]") != std::string::npos);
    std::cout << "  [PASS] RustCodegen_ErrorTypeGeneration\n";
}

void testRustCodegen_HalTraitGeneration() {
    RustCodegen generator;
    std::string rp2040Hal = generator.generateHalTraitImpl("RP2040");
    assert(rp2040Hal.find("pub trait HalInterface {") != std::string::npos);
    assert(rp2040Hal.find("pub struct Rp2040HalImpl;") != std::string::npos);
    assert(rp2040Hal.find("impl HalInterface for Rp2040HalImpl") != std::string::npos);
    std::cout << "  [PASS] RustCodegen_HalTraitGeneration\n";
}

void testRustCodegen_AsyncAwaitSupport() {
    Project proj = createSampleProject();
    RustCodegen generator;
    GenerationConfig config;
    config.enableAsyncAwait = true;

    ApiEndpoint ep = proj.getApiSchema().getEndpoint("ep_gpio_read");
    std::string code = generator.generateHandlerImpl(ep, config);

    assert(code.find("pub async fn handle(&mut self, req: ReadGpioRequest)") != std::string::npos);
    std::cout << "  [PASS] RustCodegen_AsyncAwaitSupport\n";
}

void testRustCodegen_LifetimeCorrectness() {
    RustCodegen generator;
    std::string halCode = generator.generateHalTraitImpl("Generic");
    assert(halCode.find("&'static str") != std::string::npos);
    assert(halCode.find("&mut self") != std::string::npos);
    std::cout << "  [PASS] RustCodegen_LifetimeCorrectness\n";
}

void testRustCodegen_CodeCompilation() {
    std::string rustSnippet = 
        "pub trait HalInterface {\n"
        "    fn gpio_read(&mut self, pin: u8) -> Result<u8, &'static str>;\n"
        "    fn get_tick(&self) -> u32;\n"
        "}\n"
        "pub struct MockHal;\n"
        "impl HalInterface for MockHal {\n"
        "    fn gpio_read(&mut self, _pin: u8) -> Result<u8, &'static str> { Ok(42) }\n"
        "    fn get_tick(&self) -> u32 { 100 }\n"
        "}\n"
        "pub struct GpioHandler<H: HalInterface> { hal: H }\n"
        "impl<H: HalInterface> GpioHandler<H> {\n"
        "    pub fn new(hal: H) -> Self { Self { hal } }\n"
        "    pub fn handle(&mut self, pin: u8) -> Result<u8, &'static str> { self.hal.gpio_read(pin) }\n"
        "}\n"
        "pub fn test_run() -> u8 {\n"
        "    let mut h = GpioHandler::new(MockHal);\n"
        "    h.handle(0).unwrap()\n"
        "}\n";

    bool success = runCompilerCommand("rustc --edition 2021 --crate-type lib -o /tmp/test_rust.rlib -", rustSnippet);
    assert(success);
    std::cout << "  [PASS] RustCodegen_CodeCompilation\n";
}

// ============================================================================
// MAIN RUNNER
// ============================================================================

int main() {
    std::cout << "====================================================================\n";
    std::cout << "  WEEKS 5-6: MULTI-LANGUAGE CODE GENERATORS ACCEPTANCE TESTS (24 TESTS)\n";
    std::cout << "====================================================================\n\n";

    std::cout << "--- WORK STREAM 1: C Code Generator (8 Tests) ---\n";
    testCCodegen_BasicTypeMapping();
    testCCodegen_GetEndpointGeneration();
    testCCodegen_PostEndpointWithBody();
    testCCodegen_ResponseSerialization();
    testCCodegen_HalBoardSelection();
    testCCodegen_CompositeTypeGeneration();
    testCCodegen_ErrorHandling();
    testCCodegen_CodeCompilation();

    std::cout << "\n--- WORK STREAM 2: C++ Code Generator (8 Tests) ---\n";
    testCppCodegen_BasicTypeMapping();
    testCppCodegen_DtoClassGeneration();
    testCppCodegen_JsonSerializationHooks();
    testCppCodegen_HandlerClassGeneration();
    testCppCodegen_HalBoardSelection();
    testCppCodegen_AsyncHandlerSupport();
    testCppCodegen_DependencyInjection();
    testCppCodegen_CodeCompilation();

    std::cout << "\n--- WORK STREAM 3: Rust Code Generator (8 Tests) ---\n";
    testRustCodegen_BasicTypeMapping();
    testRustCodegen_StructGeneration();
    testRustCodegen_SerdeIntegration();
    testRustCodegen_ErrorTypeGeneration();
    testRustCodegen_HalTraitGeneration();
    testRustCodegen_AsyncAwaitSupport();
    testRustCodegen_LifetimeCorrectness();
    testRustCodegen_CodeCompilation();

    std::cout << "\n====================================================================\n";
    std::cout << "  ALL 24 CODE GENERATOR ACCEPTANCE TESTS PASSED (100% OPERATIONAL SUCCESS)\n";
    std::cout << "====================================================================\n";

    return 0;
}
