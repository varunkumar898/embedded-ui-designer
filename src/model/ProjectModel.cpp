#include "ProjectModel.h"
#include "ProjectSerializer.h"
#include <algorithm>
#include <sstream>
#include <ctime>
#include <stdexcept>

namespace EmbeddedUI {

// ============================================================================
// PROJECTMETADATA
// ============================================================================

Json::Value ProjectMetadata::toJson() const {
    Json::Value json;
    json["projectName"] = projectName;
    json["projectId"] = projectId;
    json["version"] = version;
    json["createdDate"] = createdDate;
    json["lastModifiedDate"] = lastModifiedDate;
    json["author"] = author;
    json["description"] = description;
    json["targetFramework"] = targetFramework;
    return json;
}

ProjectMetadata ProjectMetadata::fromJson(const Json::Value& json) {
    ProjectMetadata metadata;
    metadata.projectName = json.get("projectName", "Untitled Project").asString();
    metadata.projectId = json.get("projectId", "").asString();
    metadata.version = json.get("version", "0.1.0").asString();
    metadata.createdDate = json.get("createdDate", "").asString();
    metadata.lastModifiedDate = json.get("lastModifiedDate", "").asString();
    metadata.author = json.get("author", "").asString();
    metadata.description = json.get("description", "").asString();
    metadata.targetFramework = json.get("targetFramework", "Generic").asString();
    return metadata;
}

// ============================================================================
// ENUMVALUE & TYPEDEFINITION
// ============================================================================

Json::Value EnumValue::toJson() const {
    Json::Value json;
    json["name"] = name;
    json["value"] = value;
    json["description"] = description;
    return json;
}

EnumValue EnumValue::fromJson(const Json::Value& json) {
    EnumValue ev;
    ev.name = json.get("name", "").asString();
    ev.value = json.get("value", 0).asInt();
    ev.description = json.get("description", "").asString();
    return ev;
}

Json::Value TypeDefinition::toJson() const {
    Json::Value json;
    json["typeId"] = typeId;
    json["typeName"] = typeName;
    json["primitiveType"] = static_cast<int>(primitiveType);
    json["isComposite"] = isComposite;
    json["maxLength"] = maxLength;
    json["maxValue"] = static_cast<Json::Int64>(maxValue);
    json["minValue"] = static_cast<Json::Int64>(minValue);
    json["pattern"] = pattern;
    json["nullable"] = nullable;

    Json::Value enumArr(Json::arrayValue);
    for (const auto& ev : enumValues) {
        enumArr.append(ev.toJson());
    }
    json["enumValues"] = enumArr;

    Json::Value compArr(Json::arrayValue);
    for (const auto& field : compositeFields) {
        Json::Value f;
        f["fieldName"] = field.first;
        f["typeId"] = field.second;
        compArr.append(f);
    }
    json["compositeFields"] = compArr;

    return json;
}

TypeDefinition TypeDefinition::fromJson(const Json::Value& json) {
    TypeDefinition td;
    td.typeId = json.get("typeId", "").asString();
    td.typeName = json.get("typeName", "").asString();
    td.primitiveType = static_cast<PrimitiveType>(json.get("primitiveType", 0).asInt());
    td.isComposite = json.get("isComposite", false).asBool();
    td.maxLength = json.get("maxLength", 0).asInt();
    td.maxValue = json.get("maxValue", 0).asInt64();
    td.minValue = json.get("minValue", 0).asInt64();
    td.pattern = json.get("pattern", "").asString();
    td.nullable = json.get("nullable", false).asBool();

    if (json.isMember("enumValues") && json["enumValues"].isArray()) {
        for (const auto& ev : json["enumValues"]) {
            td.enumValues.push_back(EnumValue::fromJson(ev));
        }
    }

    if (json.isMember("compositeFields") && json["compositeFields"].isArray()) {
        for (const auto& f : json["compositeFields"]) {
            td.compositeFields.push_back({f.get("fieldName", "").asString(), f.get("typeId", "").asString()});
        }
    }

    return td;
}

// ============================================================================
// TYPEREGISTRY
// ============================================================================

void TypeRegistry::registerType(const TypeDefinition& typeDef) {
    types[typeDef.typeId] = typeDef;
}

TypeDefinition TypeRegistry::getType(const std::string& typeId) const {
    auto it = types.find(typeId);
    if (it != types.end()) {
        return it->second;
    }
    throw std::runtime_error("Type not found: " + typeId);
}

bool TypeRegistry::typeExists(const std::string& typeId) const {
    return types.find(typeId) != types.end();
}

std::vector<std::string> TypeRegistry::getAllTypeIds() const {
    std::vector<std::string> ids;
    for (const auto& pair : types) {
        ids.push_back(pair.first);
    }
    return ids;
}

TypeRegistry TypeRegistry::createDefault() {
    TypeRegistry registry;

    TypeDefinition boolType;
    boolType.typeId = "bool";
    boolType.typeName = "Boolean";
    boolType.primitiveType = PrimitiveType::BOOL;
    registry.registerType(boolType);

    TypeDefinition int32Type;
    int32Type.typeId = "int32";
    int32Type.typeName = "32-bit Integer";
    int32Type.primitiveType = PrimitiveType::INT32;
    int32Type.maxValue = 2147483647;
    int32Type.minValue = -2147483648LL;
    registry.registerType(int32Type);

    TypeDefinition uint32Type;
    uint32Type.typeId = "uint32";
    uint32Type.typeName = "32-bit Unsigned Integer";
    uint32Type.primitiveType = PrimitiveType::UINT32;
    uint32Type.maxValue = 4294967295ULL;
    uint32Type.minValue = 0;
    registry.registerType(uint32Type);

    TypeDefinition stringType;
    stringType.typeId = "string";
    stringType.typeName = "String";
    stringType.primitiveType = PrimitiveType::STRING;
    stringType.maxLength = 256;
    registry.registerType(stringType);

    TypeDefinition floatType;
    floatType.typeId = "float";
    floatType.typeName = "Single-Precision Float";
    floatType.primitiveType = PrimitiveType::FLOAT;
    registry.registerType(floatType);

    TypeDefinition uint8Type;
    uint8Type.typeId = "uint8";
    uint8Type.typeName = "8-bit Unsigned Integer";
    uint8Type.primitiveType = PrimitiveType::UINT8;
    uint8Type.maxValue = 255;
    uint8Type.minValue = 0;
    registry.registerType(uint8Type);

    return registry;
}

// ============================================================================
// HTTPRESPONSE, APIPARAMETER, APIENDPOINT
// ============================================================================

Json::Value HttpResponse::toJson() const {
    Json::Value json;
    json["statusCode"] = static_cast<int>(statusCode);
    json["description"] = description;
    json["responseTypeId"] = responseTypeId;
    return json;
}

HttpResponse HttpResponse::fromJson(const Json::Value& json) {
    HttpResponse resp;
    resp.statusCode = static_cast<StatusCode>(json.get("statusCode", 200).asInt());
    resp.description = json.get("description", "").asString();
    resp.responseTypeId = json.get("responseTypeId", "").asString();
    return resp;
}

Json::Value ApiParameter::toJson() const {
    Json::Value json;
    json["paramName"] = paramName;
    json["typeId"] = typeId;
    json["paramLocation"] = paramLocation;
    json["description"] = description;
    json["required"] = required;
    json["defaultValue"] = defaultValue;
    return json;
}

ApiParameter ApiParameter::fromJson(const Json::Value& json) {
    ApiParameter p;
    p.paramName = json.get("paramName", "").asString();
    p.typeId = json.get("typeId", "").asString();
    p.paramLocation = json.get("paramLocation", "query").asString();
    p.description = json.get("description", "").asString();
    p.required = json.get("required", true).asBool();
    p.defaultValue = json.get("defaultValue", "").asString();
    return p;
}

Json::Value ApiEndpoint::toJson() const {
    Json::Value json;
    json["endpointId"] = endpointId;
    json["path"] = path;
    json["method"] = static_cast<int>(method);
    json["operationId"] = operationId;
    json["description"] = description;
    json["requestBodyTypeId"] = requestBodyTypeId;
    json["boundDeviceOperationId"] = boundDeviceOperationId;
    json["requiresAuthentication"] = requiresAuthentication;

    Json::Value paramsArr(Json::arrayValue);
    for (const auto& p : parameters) {
        paramsArr.append(p.toJson());
    }
    json["parameters"] = paramsArr;

    Json::Value respsArr(Json::arrayValue);
    for (const auto& r : responses) {
        respsArr.append(r.toJson());
    }
    json["responses"] = respsArr;

    Json::Value capsArr(Json::arrayValue);
    for (const auto& cap : requiredCapabilities) {
        capsArr.append(cap);
    }
    json["requiredCapabilities"] = capsArr;

    return json;
}

ApiEndpoint ApiEndpoint::fromJson(const Json::Value& json) {
    ApiEndpoint ep;
    ep.endpointId = json.get("endpointId", "").asString();
    ep.path = json.get("path", "").asString();
    ep.method = static_cast<HttpMethod>(json.get("method", 0).asInt());
    ep.operationId = json.get("operationId", "").asString();
    ep.description = json.get("description", "").asString();
    ep.requestBodyTypeId = json.get("requestBodyTypeId", "").asString();
    ep.boundDeviceOperationId = json.get("boundDeviceOperationId", "").asString();
    ep.requiresAuthentication = json.get("requiresAuthentication", false).asBool();

    if (json.isMember("parameters") && json["parameters"].isArray()) {
        for (const auto& p : json["parameters"]) {
            ep.parameters.push_back(ApiParameter::fromJson(p));
        }
    }
    if (json.isMember("responses") && json["responses"].isArray()) {
        for (const auto& r : json["responses"]) {
            ep.responses.push_back(HttpResponse::fromJson(r));
        }
    }
    if (json.isMember("requiredCapabilities") && json["requiredCapabilities"].isArray()) {
        for (const auto& c : json["requiredCapabilities"]) {
            ep.requiredCapabilities.push_back(c.asString());
        }
    }
    return ep;
}

// ============================================================================
// APISCHEMA
// ============================================================================

void ApiSchema::addEndpoint(const ApiEndpoint& endpoint) {
    endpoints[endpoint.endpointId] = endpoint;
}

void ApiSchema::removeEndpoint(const std::string& endpointId) {
    endpoints.erase(endpointId);
}

ApiEndpoint ApiSchema::getEndpoint(const std::string& endpointId) const {
    auto it = endpoints.find(endpointId);
    if (it != endpoints.end()) {
        return it->second;
    }
    throw std::runtime_error("Endpoint not found: " + endpointId);
}

bool ApiSchema::endpointExists(const std::string& endpointId) const {
    return endpoints.find(endpointId) != endpoints.end();
}

std::vector<ApiEndpoint> ApiSchema::getAllEndpoints() const {
    std::vector<ApiEndpoint> result;
    for (const auto& pair : endpoints) {
        result.push_back(pair.second);
    }
    return result;
}

void ApiSchema::setBasePath(const std::string& basePath) {
    this->basePath = basePath;
}

std::string ApiSchema::getBasePath() const {
    return basePath;
}

bool ApiSchema::validate(std::string& errorMessage) const {
    if (endpoints.empty()) {
        errorMessage = "API schema contains no endpoints";
        return false;
    }

    for (const auto& pair : endpoints) {
        if (pair.second.path.empty()) {
            errorMessage = "Endpoint has empty path: " + pair.first;
            return false;
        }
    }

    return true;
}

Json::Value ApiSchema::toJson() const {
    Json::Value json;
    json["basePath"] = basePath;

    Json::Value endpointsArr(Json::arrayValue);
    for (const auto& pair : endpoints) {
        endpointsArr.append(pair.second.toJson());
    }
    json["endpoints"] = endpointsArr;
    return json;
}

ApiSchema ApiSchema::fromJson(const Json::Value& json) {
    ApiSchema schema;
    schema.setBasePath(json.get("basePath", "").asString());
    if (json.isMember("endpoints") && json["endpoints"].isArray()) {
        for (const auto& ep : json["endpoints"]) {
            schema.addEndpoint(ApiEndpoint::fromJson(ep));
        }
    }
    return schema;
}

// ============================================================================
// UI COMPONENTS & PAGES
// ============================================================================

Json::Value ComponentBinding::toJson() const {
    Json::Value json;
    json["componentId"] = componentId;
    json["boundParameterId"] = boundParameterId;
    json["boundEndpointId"] = boundEndpointId;
    json["updateEventName"] = updateEventName;
    return json;
}

ComponentBinding ComponentBinding::fromJson(const Json::Value& json) {
    ComponentBinding b;
    b.componentId = json.get("componentId", "").asString();
    b.boundParameterId = json.get("boundParameterId", "").asString();
    b.boundEndpointId = json.get("boundEndpointId", "").asString();
    b.updateEventName = json.get("updateEventName", "onClick").asString();
    return b;
}

Json::Value UiComponent::toJson() const {
    Json::Value json;
    json["componentId"] = componentId;
    json["componentName"] = componentName;
    json["type"] = static_cast<int>(type);
    json["x"] = x;
    json["y"] = y;
    json["width"] = width;
    json["height"] = height;

    Json::Value props(Json::objectValue);
    for (const auto& pair : properties) {
        props[pair.first] = pair.second;
    }
    json["properties"] = props;

    Json::Value bArr(Json::arrayValue);
    for (const auto& b : bindings) {
        bArr.append(b.toJson());
    }
    json["bindings"] = bArr;

    Json::Value childArr(Json::arrayValue);
    for (const auto& child : children) {
        if (child) childArr.append(child->toJson());
    }
    json["children"] = childArr;

    return json;
}

std::shared_ptr<UiComponent> UiComponent::fromJson(const Json::Value& json) {
    auto c = std::make_shared<UiComponent>();
    c->componentId = json.get("componentId", "").asString();
    c->componentName = json.get("componentName", "").asString();
    c->type = static_cast<ComponentType>(json.get("type", 0).asInt());
    c->x = json.get("x", 0).asInt();
    c->y = json.get("y", 0).asInt();
    c->width = json.get("width", 100).asInt();
    c->height = json.get("height", 40).asInt();

    if (json.isMember("properties") && json["properties"].isObject()) {
        for (const auto& name : json["properties"].getMemberNames()) {
            c->properties[name] = json["properties"][name].asString();
        }
    }
    if (json.isMember("bindings") && json["bindings"].isArray()) {
        for (const auto& b : json["bindings"]) {
            c->bindings.push_back(ComponentBinding::fromJson(b));
        }
    }
    if (json.isMember("children") && json["children"].isArray()) {
        for (const auto& ch : json["children"]) {
            c->children.push_back(UiComponent::fromJson(ch));
        }
    }
    return c;
}

Json::Value UiPage::toJson() const {
    Json::Value json;
    json["pageId"] = pageId;
    json["pageName"] = pageName;
    json["description"] = description;
    json["width"] = width;
    json["height"] = height;

    Json::Value compArr(Json::arrayValue);
    for (const auto& comp : components) {
        if (comp) compArr.append(comp->toJson());
    }
    json["components"] = compArr;

    Json::Value transArr(Json::arrayValue);
    for (const auto& t : transitions) {
        transArr.append(t);
    }
    json["transitions"] = transArr;

    return json;
}

UiPage UiPage::fromJson(const Json::Value& json) {
    UiPage page;
    page.pageId = json.get("pageId", "").asString();
    page.pageName = json.get("pageName", "").asString();
    page.description = json.get("description", "").asString();
    page.width = json.get("width", 800).asInt();
    page.height = json.get("height", 480).asInt();

    if (json.isMember("components") && json["components"].isArray()) {
        for (const auto& c : json["components"]) {
            page.components.push_back(UiComponent::fromJson(c));
        }
    }
    if (json.isMember("transitions") && json["transitions"].isArray()) {
        for (const auto& t : json["transitions"]) {
            page.transitions.push_back(t.asString());
        }
    }
    return page;
}

// ============================================================================
// HARDWARE RESOURCE & DEVICE OPERATION
// ============================================================================

Json::Value HardwareResource::toJson() const {
    Json::Value json;
    json["resourceId"] = resourceId;
    json["resourceType"] = resourceType;
    json["resourceName"] = resourceName;
    json["pinNumber"] = pinNumber;
    json["pinLabel"] = pinLabel;

    Json::Value plat(Json::objectValue);
    for (const auto& pair : platformConfig) {
        plat[pair.first] = pair.second;
    }
    json["platformConfig"] = plat;

    return json;
}

HardwareResource HardwareResource::fromJson(const Json::Value& json) {
    HardwareResource res;
    res.resourceId = json.get("resourceId", "").asString();
    res.resourceType = json.get("resourceType", "GPIO").asString();
    res.resourceName = json.get("resourceName", "").asString();
    res.pinNumber = json.get("pinNumber", 0).asInt();
    res.pinLabel = json.get("pinLabel", "").asString();

    if (json.isMember("platformConfig") && json["platformConfig"].isObject()) {
        for (const auto& key : json["platformConfig"].getMemberNames()) {
            res.platformConfig[key] = json["platformConfig"][key].asString();
        }
    }
    return res;
}

Json::Value DeviceOperation::toJson() const {
    Json::Value json;
    json["operationId"] = operationId;
    json["operationName"] = operationName;
    json["opType"] = static_cast<int>(opType);
    json["description"] = description;
    json["outputTypeId"] = outputTypeId;
    json["executionTimeMs"] = executionTimeMs;
    json["isBlocking"] = isBlocking;

    Json::Value resArr(Json::arrayValue);
    for (const auto& r : requiredResources) {
        resArr.append(r.toJson());
    }
    json["requiredResources"] = resArr;

    Json::Value paramsArr(Json::arrayValue);
    for (const auto& p : inputParameters) {
        paramsArr.append(p.toJson());
    }
    json["inputParameters"] = paramsArr;

    Json::Value boardsArr(Json::arrayValue);
    for (const auto& b : supportedBoards) {
        boardsArr.append(b);
    }
    json["supportedBoards"] = boardsArr;

    return json;
}

DeviceOperation DeviceOperation::fromJson(const Json::Value& json) {
    DeviceOperation op;
    op.operationId = json.get("operationId", "").asString();
    op.operationName = json.get("operationName", "").asString();
    op.opType = static_cast<DeviceOpType>(json.get("opType", 0).asInt());
    op.description = json.get("description", "").asString();
    op.outputTypeId = json.get("outputTypeId", "").asString();
    op.executionTimeMs = json.get("executionTimeMs", 10).asInt();
    op.isBlocking = json.get("isBlocking", false).asBool();

    if (json.isMember("requiredResources") && json["requiredResources"].isArray()) {
        for (const auto& r : json["requiredResources"]) {
            op.requiredResources.push_back(HardwareResource::fromJson(r));
        }
    }
    if (json.isMember("inputParameters") && json["inputParameters"].isArray()) {
        for (const auto& p : json["inputParameters"]) {
            op.inputParameters.push_back(ApiParameter::fromJson(p));
        }
    }
    if (json.isMember("supportedBoards") && json["supportedBoards"].isArray()) {
        for (const auto& b : json["supportedBoards"]) {
            op.supportedBoards.push_back(b.asString());
        }
    }
    return op;
}

// ============================================================================
// BUILD CONFIGURATION
// ============================================================================

Json::Value BuildConfig::toJson() const {
    Json::Value json;
    json["configId"] = configId;
    json["configName"] = configName;
    json["targetBoard"] = targetBoard;
    json["targetLanguage"] = targetLanguage;
    json["enableDebugOutput"] = enableDebugOutput;
    json["enableOTA"] = enableOTA;
    json["enableSecurity"] = enableSecurity;
    json["maxFlashSize"] = maxFlashSize;
    json["maxRamSize"] = maxRamSize;

    Json::Value cfArr(Json::arrayValue);
    for (const auto& cf : compilerFlags) cfArr.append(cf);
    json["compilerFlags"] = cfArr;

    Json::Value optArr(Json::arrayValue);
    for (const auto& opt : optimizationFlags) optArr.append(opt);
    json["optimizationFlags"] = optArr;

    return json;
}

BuildConfig BuildConfig::fromJson(const Json::Value& json) {
    BuildConfig cfg;
    cfg.configId = json.get("configId", "").asString();
    cfg.configName = json.get("configName", "").asString();
    cfg.targetBoard = json.get("targetBoard", "ESP32").asString();
    cfg.targetLanguage = json.get("targetLanguage", "C").asString();
    cfg.enableDebugOutput = json.get("enableDebugOutput", false).asBool();
    cfg.enableOTA = json.get("enableOTA", false).asBool();
    cfg.enableSecurity = json.get("enableSecurity", false).asBool();
    cfg.maxFlashSize = json.get("maxFlashSize", 0).asInt();
    cfg.maxRamSize = json.get("maxRamSize", 0).asInt();

    if (json.isMember("compilerFlags") && json["compilerFlags"].isArray()) {
        for (const auto& cf : json["compilerFlags"]) cfg.compilerFlags.push_back(cf.asString());
    }
    if (json.isMember("optimizationFlags") && json["optimizationFlags"].isArray()) {
        for (const auto& opt : json["optimizationFlags"]) cfg.optimizationFlags.push_back(opt.asString());
    }
    return cfg;
}

// ============================================================================
// PROJECT
// ============================================================================

Project::Project(const std::string& projectName) : typeRegistry(TypeRegistry::createDefault()) {
    metadata.projectName = projectName;
    metadata.projectId = "proj_" + std::to_string(std::time(nullptr));
    metadata.version = "0.1.0";
    metadata.targetFramework = "Generic";
}

void Project::setMetadata(const ProjectMetadata& metadata) {
    this->metadata = metadata;
}

ProjectMetadata Project::getMetadata() const {
    return metadata;
}

TypeRegistry& Project::getTypeRegistry() {
    return typeRegistry;
}

const TypeRegistry& Project::getTypeRegistry() const {
    return typeRegistry;
}

ApiSchema& Project::getApiSchema() {
    return apiSchema;
}

const ApiSchema& Project::getApiSchema() const {
    return apiSchema;
}

void Project::addPage(const UiPage& page) {
    pages[page.pageId] = page;
}

void Project::removePage(const std::string& pageId) {
    pages.erase(pageId);
}

UiPage Project::getPage(const std::string& pageId) const {
    auto it = pages.find(pageId);
    if (it != pages.end()) {
        return it->second;
    }
    throw std::runtime_error("Page not found: " + pageId);
}

std::vector<UiPage> Project::getAllPages() const {
    std::vector<UiPage> result;
    for (const auto& pair : pages) {
        result.push_back(pair.second);
    }
    return result;
}

void Project::registerDeviceOperation(const DeviceOperation& operation) {
    deviceOperations[operation.operationId] = operation;
}

void Project::unregisterDeviceOperation(const std::string& operationId) {
    deviceOperations.erase(operationId);
}

DeviceOperation Project::getDeviceOperation(const std::string& operationId) const {
    auto it = deviceOperations.find(operationId);
    if (it != deviceOperations.end()) {
        return it->second;
    }
    throw std::runtime_error("Device operation not found: " + operationId);
}

std::vector<DeviceOperation> Project::getAllDeviceOperations() const {
    std::vector<DeviceOperation> result;
    for (const auto& pair : deviceOperations) {
        result.push_back(pair.second);
    }
    return result;
}

void Project::addBuildConfig(const BuildConfig& config) {
    buildConfigs[config.configId] = config;
}

void Project::removeBuildConfig(const std::string& configId) {
    buildConfigs.erase(configId);
}

BuildConfig Project::getBuildConfig(const std::string& configId) const {
    auto it = buildConfigs.find(configId);
    if (it != buildConfigs.end()) {
        return it->second;
    }
    throw std::runtime_error("Build config not found: " + configId);
}

std::vector<BuildConfig> Project::getAllBuildConfigs() const {
    std::vector<BuildConfig> result;
    for (const auto& pair : buildConfigs) {
        result.push_back(pair.second);
    }
    return result;
}

bool Project::validate(std::string& errorMessage) const {
    std::string schemaError;
    if (!apiSchema.validate(schemaError)) {
        errorMessage = "API Schema validation failed: " + schemaError;
        return false;
    }

    if (pages.empty()) {
        errorMessage = "Project contains no pages";
        return false;
    }

    return true;
}

Json::Value Project::toJson() const {
    return ProjectSerializer::serialize(*this);
}

Project Project::fromJson(const Json::Value& json) {
    std::string err;
    return ProjectSerializer::deserialize(json, err);
}

std::string Project::getProjectVersion() const {
    return metadata.version;
}

void Project::setProjectVersion(const std::string& version) {
    metadata.version = version;
}

} // namespace EmbeddedUI
