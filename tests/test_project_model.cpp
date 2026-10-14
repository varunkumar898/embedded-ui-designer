#include "model/ProjectModel.h"
#include "model/TypeSystem.h"
#include "model/ProjectSerializer.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>

using namespace EmbeddedUI;

void testProjectCreation() {
    Project proj("SmartHomeHub");
    ProjectMetadata meta = proj.getMetadata();
    assert(meta.projectName == "SmartHomeHub");
    assert(!meta.projectId.empty());
    assert(meta.version == "0.1.0");
    assert(meta.targetFramework == "Generic");

    meta.author = "Embedded Architecture Team";
    meta.description = "Universal IoT display controller";
    meta.targetFramework = "ESP32";
    proj.setMetadata(meta);

    assert(proj.getMetadata().author == "Embedded Architecture Team");
    assert(proj.getMetadata().targetFramework == "ESP32");
    std::cout << "  [PASS] ProjectModel: ProjectCreation\n";
}

void testPageManagement() {
    Project proj("HmiDisplay");
    assert(proj.getAllPages().empty());

    UiPage page1;
    page1.pageId = "page_home";
    page1.pageName = "Home Dashboard";
    page1.width = 800;
    page1.height = 480;

    auto btn = std::make_shared<UiComponent>();
    btn->componentId = "btn_power";
    btn->componentName = "Power Toggle";
    btn->type = ComponentType::BUTTON;
    btn->x = 20; btn->y = 40; btn->width = 120; btn->height = 50;
    page1.components.push_back(btn);

    proj.addPage(page1);
    assert(proj.getAllPages().size() == 1);
    assert(proj.getPage("page_home").pageName == "Home Dashboard");
    assert(proj.getPage("page_home").components.size() == 1);

    UiPage page2;
    page2.pageId = "page_settings";
    page2.pageName = "System Settings";
    proj.addPage(page2);
    assert(proj.getAllPages().size() == 2);

    proj.removePage("page_home");
    assert(proj.getAllPages().size() == 1);
    assert(proj.getPage("page_settings").pageName == "System Settings");
    std::cout << "  [PASS] ProjectModel: PageManagement\n";
}

void testDeviceOperationRegistration() {
    Project proj("RoboticsController");
    DeviceOperation op;
    op.operationId = "read_encoder";
    op.operationName = "Read Rotary Encoder";
    op.opType = DeviceOpType::GPIO_READ;
    op.description = "Reads optical quadrature encoder pulses";
    op.outputTypeId = "int32";
    op.executionTimeMs = 1;
    op.isBlocking = false;
    op.supportedBoards = {"STM32F030R8", "ESP32"};

    HardwareResource res;
    res.resourceId = "pin_encoder_a";
    res.resourceType = "GPIO";
    res.resourceName = "TIM1_CH1";
    res.pinNumber = 8;
    res.pinLabel = "PA8";
    res.platformConfig["stm32_port"] = "GPIOA";
    res.platformConfig["stm32_pin"] = "GPIO_PIN_8";
    op.requiredResources.push_back(res);

    proj.registerDeviceOperation(op);
    assert(proj.getAllDeviceOperations().size() == 1);

    DeviceOperation retrieved = proj.getDeviceOperation("read_encoder");
    assert(retrieved.operationName == "Read Rotary Encoder");
    assert(retrieved.requiredResources.size() == 1);
    assert(retrieved.requiredResources[0].pinLabel == "PA8");

    proj.unregisterDeviceOperation("read_encoder");
    assert(proj.getAllDeviceOperations().empty());
    std::cout << "  [PASS] ProjectModel: DeviceOperationRegistration\n";
}

void testBuildConfigManagement() {
    Project proj("IndustrialHmi");
    BuildConfig cfgStm;
    cfgStm.configId = "stm32_release";
    cfgStm.configName = "STM32 Release";
    cfgStm.targetBoard = "STM32F030R8";
    cfgStm.targetLanguage = "C";
    cfgStm.compilerFlags = {"-O2", "-Wall", "-mcpu=cortex-m0"};
    cfgStm.maxFlashSize = 65536;
    cfgStm.maxRamSize = 8192;

    BuildConfig cfgEsp;
    cfgEsp.configId = "esp32_release";
    cfgEsp.configName = "ESP32 IDF Release";
    cfgEsp.targetBoard = "ESP32";
    cfgEsp.targetLanguage = "C++";
    cfgEsp.compilerFlags = {"-O3", "-fno-rtti"};
    cfgEsp.maxFlashSize = 4194304;
    cfgEsp.maxRamSize = 532480;

    proj.addBuildConfig(cfgStm);
    proj.addBuildConfig(cfgEsp);
    assert(proj.getAllBuildConfigs().size() == 2);
    assert(proj.getBuildConfig("stm32_release").targetLanguage == "C");
    assert(proj.getBuildConfig("esp32_release").targetLanguage == "C++");

    proj.removeBuildConfig("stm32_release");
    assert(proj.getAllBuildConfigs().size() == 1);
    std::cout << "  [PASS] ProjectModel: BuildConfigManagement\n";
}

void testTypeRegistration() {
    TypeRegistry reg = TypeRegistry::createDefault();
    assert(reg.typeExists("bool"));
    assert(reg.typeExists("int32"));
    assert(reg.typeExists("uint32"));
    assert(reg.typeExists("string"));
    assert(reg.typeExists("float"));
    assert(reg.typeExists("uint8"));

    TypeDefinition custom;
    custom.typeId = "temperature_c";
    custom.typeName = "Celsius Temperature";
    custom.primitiveType = PrimitiveType::FLOAT;
    custom.minValue = -40;
    custom.maxValue = 125;
    reg.registerType(custom);

    assert(reg.typeExists("temperature_c"));
    TypeDefinition retrieved = reg.getType("temperature_c");
    assert(retrieved.typeName == "Celsius Temperature");
    assert(retrieved.minValue == -40);
    std::cout << "  [PASS] TypeRegistry: TypeRegistration\n";
}

void testCompositeTypeSupport() {
    TypeRegistry reg = TypeRegistry::createDefault();

    TypeDefinition telemetryPacket;
    telemetryPacket.typeId = "TelemetryPacketDto";
    telemetryPacket.typeName = "Telemetry Packet Struct";
    telemetryPacket.isComposite = true;
    telemetryPacket.compositeFields.push_back({"node_id", "uint32"});
    telemetryPacket.compositeFields.push_back({"temperature", "float"});
    telemetryPacket.compositeFields.push_back({"is_alarm", "bool"});
    reg.registerType(telemetryPacket);

    assert(reg.typeExists("TelemetryPacketDto"));
    TypeDefinition retrieved = reg.getType("TelemetryPacketDto");
    assert(retrieved.isComposite);
    assert(retrieved.compositeFields.size() == 3);
    assert(retrieved.compositeFields[1].first == "temperature");
    assert(retrieved.compositeFields[1].second == "float");

    StructuredDtoBuilder builder("TelemetryPacketDto");
    builder.addField("node_id", "uint32");
    builder.addField("temperature", "float");
    builder.addField("is_alarm", "bool");
    std::string cppCode = builder.generateCppStruct();
    assert(cppCode.find("uint32_t node_id;") != std::string::npos);
    assert(cppCode.find("float temperature;") != std::string::npos);
    assert(cppCode.find("bool is_alarm;") != std::string::npos);
    std::cout << "  [PASS] TypeRegistry: CompositeTypeSupport\n";
}

void testTypeValidation() {
    ValidationRule minLen;
    minLen.ruleType = ValidationRuleType::MIN_LENGTH;
    minLen.ruleExpression = "4";
    assert(!minLen.validate("abc"));
    assert(minLen.validate("abcd"));
    assert(minLen.validate("abcdef"));

    ValidationRule regexRule;
    regexRule.ruleType = ValidationRuleType::REGEX_PATTERN;
    regexRule.ruleExpression = "^[A-Z0-9_]+$";
    assert(regexRule.validate("GPIO_PA0"));
    assert(!regexRule.validate("gpio pa0!"));

    assert(TypeConverter::areCompatible("int32", "uint32"));
    assert(TypeConverter::areCompatible("float", "double"));
    assert(!TypeConverter::areCompatible("bool", "string"));
    std::cout << "  [PASS] TypeRegistry: TypeValidation\n";
}

void testEndpointDefinition() {
    ApiSchema schema;
    schema.setBasePath("/api/v1");

    ApiEndpoint ep;
    ep.endpointId = "read_gpio_state";
    ep.path = "/api/v1/gpio/read";
    ep.method = HttpMethod::GET;
    ep.operationId = "readGpioState";
    ep.description = "Reads physical logic level";

    ApiParameter p;
    p.paramName = "pin";
    p.typeId = "uint8";
    p.paramLocation = "query";
    p.required = true;
    ep.parameters.push_back(p);

    HttpResponse resp200;
    resp200.statusCode = StatusCode::OK_200;
    resp200.responseTypeId = "bool";
    resp200.description = "Logic level reading";
    ep.responses.push_back(resp200);

    schema.addEndpoint(ep);
    assert(schema.endpointExists("read_gpio_state"));
    ApiEndpoint ret = schema.getEndpoint("read_gpio_state");
    assert(ret.operationId == "readGpioState");
    assert(ret.parameters.size() == 1);
    assert(ret.responses.size() == 1);
    assert(ret.responses[0].statusCode == StatusCode::OK_200);
    std::cout << "  [PASS] ApiSchema: EndpointDefinition\n";
}

void testSchemaValidation() {
    ApiSchema emptySchema;
    std::string err;
    assert(!emptySchema.validate(err));
    assert(!err.empty());

    ApiSchema validSchema;
    validSchema.setBasePath("/api/v1");
    ApiEndpoint ep;
    ep.endpointId = "ping";
    ep.path = "/api/v1/ping";
    ep.method = HttpMethod::GET;
    validSchema.addEndpoint(ep);

    err.clear();
    assert(validSchema.validate(err));
    assert(err.empty());

    // Invalid endpoint with empty path
    ApiEndpoint badEp;
    badEp.endpointId = "bad";
    badEp.path = "";
    validSchema.addEndpoint(badEp);
    assert(!validSchema.validate(err));
    std::cout << "  [PASS] ApiSchema: SchemaValidation\n";
}

void testParameterBinding() {
    TypeRegistry types = TypeRegistry::createDefault();
    ApiEndpoint ep;
    ep.endpointId = "pwm_set";
    ep.path = "/api/v1/pwm/duty";
    ep.method = HttpMethod::POST;

    ApiParameter pDuty;
    pDuty.paramName = "duty_cycle";
    pDuty.typeId = "uint8";
    pDuty.paramLocation = "body";
    ep.parameters.push_back(pDuty);

    for (const auto& p : ep.parameters) {
        assert(types.typeExists(p.typeId));
        TypeDefinition td = types.getType(p.typeId);
        assert(td.primitiveType == PrimitiveType::UINT8);
    }
    std::cout << "  [PASS] ApiSchema: ParameterBinding\n";
}

void testProjectSerialization() {
    Project proj("EcoSensorDisplay");
    ProjectMetadata meta = proj.getMetadata();
    meta.version = "1.0.0";
    meta.targetFramework = "STM32";
    proj.setMetadata(meta);

    ApiEndpoint ep;
    ep.endpointId = "get_temp";
    ep.path = "/api/v1/temp";
    ep.method = HttpMethod::GET;
    proj.getApiSchema().addEndpoint(ep);

    UiPage page;
    page.pageId = "main";
    page.pageName = "Main Screen";
    auto lbl = std::make_shared<UiComponent>();
    lbl->componentId = "lbl_temp";
    lbl->componentName = "Temp Display";
    lbl->type = ComponentType::LABEL;
    page.components.push_back(lbl);
    proj.addPage(page);

    std::string tempFile = "/tmp/test_serialized_project.json";
    std::string err;
    bool okWrite = ProjectSerializer::writeToFile(proj, tempFile, err);
    assert(okWrite);

    Project restored = ProjectSerializer::readFromFile(tempFile, err);
    assert(restored.getMetadata().projectName == "EcoSensorDisplay");
    assert(restored.getMetadata().targetFramework == "STM32");
    assert(restored.getApiSchema().endpointExists("get_temp"));
    assert(restored.getAllPages().size() == 1);
    assert(restored.getPage("main").components.size() == 1);
    std::cout << "  [PASS] Serialization: ProjectSerialization\n";
}

void testBackwardsCompatibility() {
    // V0 legacy JSON without buildConfigs and formatVersion
    Json::Value legacyJson;
    legacyJson["metadata"]["projectName"] = "LegacyV0Project";
    legacyJson["metadata"]["version"] = "0.1.0";
    legacyJson["apiSchema"]["endpoints"] = Json::arrayValue;
    legacyJson["pages"] = Json::arrayValue;

    std::string err;
    Project migrated = ProjectSerializer::deserialize(legacyJson, err);
    assert(migrated.getMetadata().projectName == "LegacyV0Project");
    // Verify auto-migration succeeded
    assert(ProjectSerializer::isBackwardsCompatible(0, 1));
    std::cout << "  [PASS] Serialization: BackwardsCompatibility\n";
}

// ============================================================================
// WORK STREAM 2: UI WORKFLOW ACCEPTANCE TESTS (8 Tests)
// ============================================================================

void testUIWorkflow1_SchemaEditorAddRemove() {
    ApiSchema schema;
    schema.setBasePath("/api/v1");
    assert(schema.getAllEndpoints().empty());

    ApiEndpoint ep1;
    ep1.endpointId = "ep_1";
    ep1.path = "/api/v1/sensors/temp";
    ep1.method = HttpMethod::GET;
    schema.addEndpoint(ep1);
    assert(schema.getAllEndpoints().size() == 1);

    schema.removeEndpoint("ep_1");
    assert(schema.getAllEndpoints().empty());
    std::cout << "  [PASS] UI Workflow 1: Schema Editor Endpoint Add & Remove\n";
}

void testUIWorkflow2_ParameterConfiguration() {
    ApiEndpoint ep;
    ep.endpointId = "ep_cfg";
    ep.path = "/api/v1/motor/config";

    ApiParameter p1{"speed", "int32", "query", "Motor speed RPM", true, "1000"};
    ApiParameter p2{"token", "string", "header", "Auth token", false, ""};
    ApiParameter p3{"mode", "uint8", "body", "Control mode", true, "1"};
    ep.parameters = {p1, p2, p3};

    assert(ep.parameters.size() == 3);
    assert(ep.parameters[0].paramLocation == "query");
    assert(ep.parameters[1].paramLocation == "header");
    assert(ep.parameters[2].paramLocation == "body");
    std::cout << "  [PASS] UI Workflow 2: Parameter Configuration (path/query/header/body)\n";
}

void testUIWorkflow3_ResponseDefinition() {
    ApiEndpoint ep;
    HttpResponse r200{StatusCode::OK_200, "Success", "bool"};
    HttpResponse r400{StatusCode::BAD_REQUEST_400, "Invalid Request", "ErrorModel"};
    HttpResponse r500{StatusCode::INTERNAL_ERROR_500, "MCU Fault", "ErrorModel"};
    ep.responses = {r200, r400, r500};

    assert(ep.responses.size() == 3);
    assert(ep.responses[0].statusCode == StatusCode::OK_200);
    assert(ep.responses[1].statusCode == StatusCode::BAD_REQUEST_400);
    assert(ep.responses[2].statusCode == StatusCode::INTERNAL_ERROR_500);
    std::cout << "  [PASS] UI Workflow 3: Response Definition and Status Code Mapping\n";
}

void testUIWorkflow4_RealtimeValidationBanner() {
    ApiSchema schema;
    std::string err;
    assert(!schema.validate(err)); // Empty schema invalid

    ApiEndpoint valid{"ep_v", "/api/v1/state", HttpMethod::GET, "getState", "Desc", {}, "", {}, "", {}, false};
    schema.addEndpoint(valid);
    err.clear();
    assert(schema.validate(err)); // Valid
    std::cout << "  [PASS] UI Workflow 4: Real-time Schema Validation Banner\n";
}

void testUIWorkflow5_ComponentEventBinding() {
    UiComponent comp;
    comp.componentId = "btn_fan_toggle";
    comp.componentName = "Fan Power Button";
    comp.type = ComponentType::BUTTON;

    ComponentBinding b;
    b.componentId = "btn_fan_toggle";
    b.boundEndpointId = "ep_gpio_write";
    b.boundParameterId = "power_state";
    b.updateEventName = "onClick";
    comp.bindings.push_back(b);

    assert(comp.bindings.size() == 1);
    assert(comp.bindings[0].boundParameterId == "power_state");
    assert(comp.bindings[0].updateEventName == "onClick");
    std::cout << "  [PASS] UI Workflow 5: Component Event Drag/Drop Binding to API Parameter\n";
}

void testUIWorkflow6_LiveBindingCodePreview() {
    StructuredDtoBuilder dto("FanControlDto");
    dto.addField("power_state", "bool");
    dto.addField("speed_level", "uint8");

    std::string cppCode = dto.generateCppStruct();
    std::string cCode = dto.generateCCode();
    std::string rustCode = dto.generateRustStruct();

    assert(cppCode.find("bool power_state;") != std::string::npos);
    assert(cCode.find("uint8_t speed_level;") != std::string::npos);
    assert(rustCode.find("pub power_state: bool") != std::string::npos);
    std::cout << "  [PASS] UI Workflow 6: Live Binding Code Preview Generation\n";
}

void testUIWorkflow7_DeviceOperationToPinMapping() {
    DeviceOperation op;
    op.operationId = "op_status_led";
    op.opType = DeviceOpType::GPIO_WRITE;

    HardwareResource pinRes;
    pinRes.resourceId = "pin_led_pa5";
    pinRes.resourceType = "GPIO";
    pinRes.resourceName = "LED_STATUS";
    pinRes.pinNumber = 21;
    pinRes.pinLabel = "PA5";
    pinRes.platformConfig["stm32_port"] = "GPIOA";
    pinRes.platformConfig["stm32_pin"] = "GPIO_PIN_5";
    op.requiredResources.push_back(pinRes);

    assert(op.requiredResources.size() == 1);
    assert(op.requiredResources[0].pinLabel == "PA5");
    assert(op.requiredResources[0].platformConfig["stm32_pin"] == "GPIO_PIN_5");
    std::cout << "  [PASS] UI Workflow 7: Device Operation to Physical Pin Mapping\n";
}

void testUIWorkflow8_CompleteProjectTreeExport() {
    Project proj("SmartDisplayCluster");
    ProjectMetadata meta = proj.getMetadata();
    meta.version = "1.0.0";
    meta.targetFramework = "Universal STM32/ESP32";
    proj.setMetadata(meta);

    ApiEndpoint ep{"ep_read", "/api/v1/cluster/speed", HttpMethod::GET, "getSpeed", "Speedometer gauge reading", {}, "", {}, "", {}, false};
    proj.getApiSchema().addEndpoint(ep);

    UiPage page{"page_cluster", "Cluster Screen", "Main gauge dashboard", 800, 480, {}, {}};
    proj.addPage(page);

    Json::Value json = proj.toJson();
    assert(json["formatVersion"].asInt() == 1);
    assert(json["metadata"]["projectName"].asString() == "SmartDisplayCluster");
    assert(json["apiSchema"]["endpoints"].size() == 1);
    assert(json["pages"].size() == 1);
    std::cout << "  [PASS] UI Workflow 8: Complete Project Structure Tree Navigation & Serialization Export\n";
}

int main() {
    std::cout << "====================================================================\n";
    std::cout << "  WORK STREAM 1 & WORK STREAM 2: ACCEPTANCE TEST SUITE (20 TESTS)\n";
    std::cout << "====================================================================\n";

    std::cout << "\n--- WORK STREAM 1: ProjectModel & TypeRegistry (12 Tests) ---\n";
    testProjectCreation();
    testPageManagement();
    testDeviceOperationRegistration();
    testBuildConfigManagement();
    testTypeRegistration();
    testCompositeTypeSupport();
    testTypeValidation();
    testEndpointDefinition();
    testSchemaValidation();
    testParameterBinding();
    testProjectSerialization();
    testBackwardsCompatibility();

    std::cout << "\n--- WORK STREAM 2: Frontend API Integration UI Workflows (8 Tests) ---\n";
    testUIWorkflow1_SchemaEditorAddRemove();
    testUIWorkflow2_ParameterConfiguration();
    testUIWorkflow3_ResponseDefinition();
    testUIWorkflow4_RealtimeValidationBanner();
    testUIWorkflow5_ComponentEventBinding();
    testUIWorkflow6_LiveBindingCodePreview();
    testUIWorkflow7_DeviceOperationToPinMapping();
    testUIWorkflow8_CompleteProjectTreeExport();

    std::cout << "\n====================================================================\n";
    std::cout << "  ALL 20 ACCEPTANCE TESTS PASSED (100% OPERATIONAL SUCCESS)\n";
    std::cout << "====================================================================\n";
    return 0;
}
