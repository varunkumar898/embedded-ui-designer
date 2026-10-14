#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <utility>
#include "json/json.h"

namespace EmbeddedUI {

// ============================================================================
// VERSION & METADATA
// ============================================================================

struct ProjectMetadata {
    std::string projectName;
    std::string projectId;           // UUID
    std::string version;             // Semantic: "1.0.0"
    std::string createdDate;         // ISO 8601
    std::string lastModifiedDate;    // ISO 8601
    std::string author;
    std::string description;
    std::string targetFramework;     // "STM32", "ESP32", "RP2040", "NXP_iMX_RT", "Generic"

    Json::Value toJson() const;
    static ProjectMetadata fromJson(const Json::Value& json);
};

// ============================================================================
// TYPE SYSTEM - PRIMITIVE & COMPOSITE TYPES
// ============================================================================

enum class PrimitiveType {
    BOOL,
    INT8, INT16, INT32, INT64,
    UINT8, UINT16, UINT32, UINT64,
    FLOAT, DOUBLE,
    STRING,
    BYTES,
    ENUM
};

struct EnumValue {
    std::string name;
    int32_t value = 0;
    std::string description;

    Json::Value toJson() const;
    static EnumValue fromJson(const Json::Value& json);
};

struct TypeDefinition {
    std::string typeId;
    std::string typeName;
    PrimitiveType primitiveType = PrimitiveType::INT32;

    // For ENUM type:
    std::vector<EnumValue> enumValues;

    // For COMPOSITE type (struct-like):
    bool isComposite = false;
    std::vector<std::pair<std::string, std::string>> compositeFields; // { fieldName, typeId }

    // Serialization constraints:
    int maxLength = 0;       // For STRING/BYTES
    int64_t maxValue = 0;    // For numeric types
    int64_t minValue = 0;
    std::string pattern;     // Regex pattern for validation
    bool nullable = false;

    Json::Value toJson() const;
    static TypeDefinition fromJson(const Json::Value& json);
};

class TypeRegistry {
public:
    void registerType(const TypeDefinition& typeDef);
    TypeDefinition getType(const std::string& typeId) const;
    bool typeExists(const std::string& typeId) const;
    std::vector<std::string> getAllTypeIds() const;

    // Built-in types
    static TypeRegistry createDefault();

private:
    std::map<std::string, TypeDefinition> types;
};

// ============================================================================
// API SCHEMA - ENDPOINTS & OPERATIONS
// ============================================================================

enum class HttpMethod {
    GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS
};

enum class StatusCode {
    OK_200 = 200,
    CREATED_201 = 201,
    BAD_REQUEST_400 = 400,
    UNAUTHORIZED_401 = 401,
    FORBIDDEN_403 = 403,
    NOT_FOUND_404 = 404,
    CONFLICT_409 = 409,
    INTERNAL_ERROR_500 = 500,
    SERVICE_UNAVAILABLE_503 = 503
};

struct HttpResponse {
    StatusCode statusCode = StatusCode::OK_200;
    std::string description;
    std::string responseTypeId;  // References TypeRegistry

    Json::Value toJson() const;
    static HttpResponse fromJson(const Json::Value& json);
};

struct ApiParameter {
    std::string paramName;
    std::string typeId;
    std::string paramLocation;  // "path", "query", "header", "body"
    std::string description;
    bool required = true;
    std::string defaultValue;

    Json::Value toJson() const;
    static ApiParameter fromJson(const Json::Value& json);
};

struct ApiEndpoint {
    std::string endpointId;
    std::string path;              // "/api/v1/device/gpio/read"
    HttpMethod method = HttpMethod::GET;
    std::string operationId;       // "readGPIO"
    std::string description;

    std::vector<ApiParameter> parameters;
    std::string requestBodyTypeId; // TypeRegistry ID or empty if no body
    std::vector<HttpResponse> responses;

    // Device operation binding:
    std::string boundDeviceOperationId;  // Empty if pure compute

    // Security/metadata:
    std::vector<std::string> requiredCapabilities;  // ["gpio_read", "debug"]
    bool requiresAuthentication = false;

    Json::Value toJson() const;
    static ApiEndpoint fromJson(const Json::Value& json);
};

class ApiSchema {
public:
    void addEndpoint(const ApiEndpoint& endpoint);
    void removeEndpoint(const std::string& endpointId);
    ApiEndpoint getEndpoint(const std::string& endpointId) const;
    bool endpointExists(const std::string& endpointId) const;
    std::vector<ApiEndpoint> getAllEndpoints() const;

    void setBasePath(const std::string& basePath);  // "/api/v1"
    std::string getBasePath() const;

    // Validation
    bool validate(std::string& errorMessage) const;

    Json::Value toJson() const;
    static ApiSchema fromJson(const Json::Value& json);

private:
    std::map<std::string, ApiEndpoint> endpoints;
    std::string basePath;
};

// ============================================================================
// UI PAGES & COMPONENTS
// ============================================================================

enum class ComponentType {
    TEXT_INPUT,
    BUTTON,
    LABEL,
    TOGGLE,
    SLIDER,
    DROPDOWN,
    CHECKBOX,
    TEXT_DISPLAY,
    PROGRESS_BAR,
    CONTAINER,
    CUSTOM
};

struct ComponentBinding {
    std::string componentId;
    std::string boundParameterId;  // References ApiParameter
    std::string boundEndpointId;
    std::string updateEventName;   // "onClick", "onChange", etc.

    Json::Value toJson() const;
    static ComponentBinding fromJson(const Json::Value& json);
};

struct UiComponent {
    std::string componentId;
    std::string componentName;
    ComponentType type = ComponentType::CUSTOM;

    // Layout:
    int x = 0, y = 0, width = 100, height = 40;

    // Properties:
    std::map<std::string, std::string> properties; // {"text": "Click me", "color": "#FF0000"}

    // Data binding:
    std::vector<ComponentBinding> bindings;

    // Children (for CONTAINER):
    std::vector<std::shared_ptr<UiComponent>> children;

    Json::Value toJson() const;
    static std::shared_ptr<UiComponent> fromJson(const Json::Value& json);
};

struct UiPage {
    std::string pageId;
    std::string pageName;
    std::string description;

    // Dimensions:
    int width = 800, height = 480;

    // Content:
    std::vector<std::shared_ptr<UiComponent>> components;

    // Navigation:
    std::vector<std::string> transitions; // Can navigate to these page IDs

    Json::Value toJson() const;
    static UiPage fromJson(const Json::Value& json);
};

// ============================================================================
// DEVICE OPERATIONS - Hardware Abstraction
// ============================================================================

enum class DeviceOpType {
    GPIO_READ,
    GPIO_WRITE,
    ADC_READ,
    PWM_WRITE,
    SPI_TRANSFER,
    I2C_READ,
    I2C_WRITE,
    UART_READ,
    UART_WRITE,
    TIMER_START,
    TIMER_STOP,
    INTERRUPT_ENABLE,
    INTERRUPT_DISABLE,
    CUSTOM
};

struct HardwareResource {
    std::string resourceId;
    std::string resourceType;  // "GPIO", "ADC", "SPI", etc.
    std::string resourceName;  // "GPIO_PA0", "ADC_Channel_1"
    int pinNumber = 0;
    std::string pinLabel;      // From BoardDatabase

    // Platform-specific config:
    std::map<std::string, std::string> platformConfig; // {"stm32_port": "GPIOA", "stm32_pin": "GPIO_PIN_0"}

    Json::Value toJson() const;
    static HardwareResource fromJson(const Json::Value& json);
};

struct DeviceOperation {
    std::string operationId;
    std::string operationName;
    DeviceOpType opType = DeviceOpType::CUSTOM;
    std::string description;

    // Hardware resources involved:
    std::vector<HardwareResource> requiredResources;

    // Input parameters (what the API receives):
    std::vector<ApiParameter> inputParameters;

    // Output type (what the device returns):
    std::string outputTypeId;

    // Platform support:
    std::vector<std::string> supportedBoards;  // ["ESP32", "STM32F030R8"]

    // Timing:
    int executionTimeMs = 10;  // Expected execution time
    bool isBlocking = false;

    Json::Value toJson() const;
    static DeviceOperation fromJson(const Json::Value& json);
};

// ============================================================================
// BUILD CONFIGURATION
// ============================================================================

struct BuildConfig {
    std::string configId;
    std::string configName;

    std::string targetBoard;
    std::string targetLanguage;    // "C", "C++", "Rust"

    // Compilation flags:
    std::vector<std::string> compilerFlags;
    std::vector<std::string> optimizationFlags;

    // Feature toggles:
    bool enableDebugOutput = false;
    bool enableOTA = false;
    bool enableSecurity = false;

    // Memory constraints:
    int maxFlashSize = 0;
    int maxRamSize = 0;

    Json::Value toJson() const;
    static BuildConfig fromJson(const Json::Value& json);
};

// ============================================================================
// UNIFIED PROJECT MODEL
// ============================================================================

class Project {
public:
    explicit Project(const std::string& projectName);

    // Metadata accessors:
    void setMetadata(const ProjectMetadata& metadata);
    ProjectMetadata getMetadata() const;

    // Type registry:
    TypeRegistry& getTypeRegistry();
    const TypeRegistry& getTypeRegistry() const;

    // API schema:
    ApiSchema& getApiSchema();
    const ApiSchema& getApiSchema() const;

    // Pages & components:
    void addPage(const UiPage& page);
    void removePage(const std::string& pageId);
    UiPage getPage(const std::string& pageId) const;
    std::vector<UiPage> getAllPages() const;

    // Device operations:
    void registerDeviceOperation(const DeviceOperation& operation);
    void unregisterDeviceOperation(const std::string& operationId);
    DeviceOperation getDeviceOperation(const std::string& operationId) const;
    std::vector<DeviceOperation> getAllDeviceOperations() const;

    // Build configurations:
    void addBuildConfig(const BuildConfig& config);
    void removeBuildConfig(const std::string& configId);
    BuildConfig getBuildConfig(const std::string& configId) const;
    std::vector<BuildConfig> getAllBuildConfigs() const;

    // Validation:
    bool validate(std::string& errorMessage) const;

    // Serialization:
    Json::Value toJson() const;
    static Project fromJson(const Json::Value& json);

    // Version management:
    std::string getProjectVersion() const;
    void setProjectVersion(const std::string& version);

private:
    ProjectMetadata metadata;
    TypeRegistry typeRegistry;
    ApiSchema apiSchema;

    std::map<std::string, UiPage> pages;
    std::map<std::string, DeviceOperation> deviceOperations;
    std::map<std::string, BuildConfig> buildConfigs;
};

} // namespace EmbeddedUI
