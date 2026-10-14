#include "TypeSystem.h"
#include <regex>
#include <stdexcept>

namespace EmbeddedUI {

// ============================================================================
// VALIDATION RULE
// ============================================================================

bool ValidationRule::validate(const std::string& value) const {
    try {
        switch (ruleType) {
            case ValidationRuleType::MIN_LENGTH:
                return value.length() >= static_cast<size_t>(std::stoi(ruleExpression));
            case ValidationRuleType::MAX_LENGTH:
                return value.length() <= static_cast<size_t>(std::stoi(ruleExpression));
            case ValidationRuleType::REGEX_PATTERN:
                return std::regex_match(value, std::regex(ruleExpression));
            case ValidationRuleType::MIN_VALUE: {
                double val = std::stod(value);
                return val >= std::stod(ruleExpression);
            }
            case ValidationRuleType::MAX_VALUE: {
                double val = std::stod(value);
                return val <= std::stod(ruleExpression);
            }
            default:
                return true;
        }
    } catch (...) {
        return false;
    }
}

Json::Value ValidationRule::toJson() const {
    Json::Value json;
    json["ruleType"] = static_cast<int>(ruleType);
    json["description"] = description;
    json["ruleExpression"] = ruleExpression;
    return json;
}

ValidationRule ValidationRule::fromJson(const Json::Value& json) {
    ValidationRule rule;
    rule.ruleType = static_cast<ValidationRuleType>(json.get("ruleType", 0).asInt());
    rule.description = json.get("description", "").asString();
    rule.ruleExpression = json.get("ruleExpression", "").asString();
    return rule;
}

// ============================================================================
// TYPE CONVERTER
// ============================================================================

std::string TypeConverter::toCpp(const std::string& typeId) {
    if (typeId == "bool") return "bool";
    if (typeId == "int8") return "int8_t";
    if (typeId == "int16") return "int16_t";
    if (typeId == "int32") return "int32_t";
    if (typeId == "int64") return "int64_t";
    if (typeId == "uint8") return "uint8_t";
    if (typeId == "uint16") return "uint16_t";
    if (typeId == "uint32") return "uint32_t";
    if (typeId == "uint64") return "uint64_t";
    if (typeId == "float") return "float";
    if (typeId == "double") return "double";
    if (typeId == "string") return "std::string";
    if (typeId == "bytes") return "std::vector<uint8_t>";
    return typeId;
}

std::string TypeConverter::toC(const std::string& typeId) {
    if (typeId == "bool") return "bool";
    if (typeId == "int8") return "int8_t";
    if (typeId == "int16") return "int16_t";
    if (typeId == "int32") return "int32_t";
    if (typeId == "int64") return "int64_t";
    if (typeId == "uint8") return "uint8_t";
    if (typeId == "uint16") return "uint16_t";
    if (typeId == "uint32") return "uint32_t";
    if (typeId == "uint64") return "uint64_t";
    if (typeId == "float") return "float";
    if (typeId == "double") return "double";
    if (typeId == "string") return "const char*";
    if (typeId == "bytes") return "uint8_t*";
    return typeId;
}

std::string TypeConverter::toRust(const std::string& typeId) {
    if (typeId == "bool") return "bool";
    if (typeId == "int8") return "i8";
    if (typeId == "int16") return "i16";
    if (typeId == "int32") return "i32";
    if (typeId == "int64") return "i64";
    if (typeId == "uint8") return "u8";
    if (typeId == "uint16") return "u16";
    if (typeId == "uint32") return "u32";
    if (typeId == "uint64") return "u64";
    if (typeId == "float") return "f32";
    if (typeId == "double") return "f64";
    if (typeId == "string") return "String";
    if (typeId == "bytes") return "Vec<u8>";
    return typeId;
}

std::string TypeConverter::toPython(const std::string& typeId) {
    if (typeId == "bool") return "bool";
    if (typeId == "int8" || typeId == "int16" || typeId == "int32" || typeId == "int64") return "int";
    if (typeId == "uint8" || typeId == "uint16" || typeId == "uint32" || typeId == "uint64") return "int";
    if (typeId == "float" || typeId == "double") return "float";
    if (typeId == "string") return "str";
    if (typeId == "bytes") return "bytes";
    return "Any";
}

std::string TypeConverter::getDefaultValue(const std::string& typeId) {
    if (typeId == "bool") return "false";
    if (typeId == "int8" || typeId == "int16" || typeId == "int32" || typeId == "int64" ||
        typeId == "uint8" || typeId == "uint16" || typeId == "uint32" || typeId == "uint64") {
        return "0";
    }
    if (typeId == "float" || typeId == "double") return "0.0";
    if (typeId == "string") return "\"\"";
    return "";
}

bool TypeConverter::areCompatible(const std::string& typeId1, const std::string& typeId2) {
    if (typeId1 == typeId2) return true;

    // Allow int32 <-> uint32 compatibility
    if ((typeId1 == "int32" && typeId2 == "uint32") ||
        (typeId1 == "uint32" && typeId2 == "int32")) {
        return true;
    }

    // Allow float <-> double compatibility
    if ((typeId1 == "float" && typeId2 == "double") ||
        (typeId1 == "double" && typeId2 == "float")) {
        return true;
    }

    return false;
}

// ============================================================================
// STRUCTURED DTO BUILDER
// ============================================================================

StructuredDtoBuilder::StructuredDtoBuilder(const std::string& dtoName)
    : dtoName(dtoName) {}

void StructuredDtoBuilder::addField(const std::string& fieldName, const std::string& typeId,
                                    bool required, const std::string& description) {
    fields.push_back(std::make_tuple(fieldName, typeId, required, description));
}

void StructuredDtoBuilder::addValidationRule(const std::string& fieldName,
                                             const ValidationRule& rule) {
    validationRules[fieldName].push_back(rule);
}

std::string StructuredDtoBuilder::generateCppStruct() const {
    std::string code = "struct " + dtoName + " {\n";
    for (const auto& field : fields) {
        std::string fieldName = std::get<0>(field);
        std::string typeId = std::get<1>(field);
        code += "    " + TypeConverter::toCpp(typeId) + " " + fieldName + ";\n";
    }
    code += "};\n";
    return code;
}

std::string StructuredDtoBuilder::generateCppSerialization() const {
    std::string code = "Json::Value " + dtoName + "::toJson() const {\n";
    code += "    Json::Value json;\n";
    for (const auto& field : fields) {
        std::string fieldName = std::get<0>(field);
        code += "    json[\"" + fieldName + "\"] = " + fieldName + ";\n";
    }
    code += "    return json;\n}\n";
    return code;
}

std::string StructuredDtoBuilder::generateCCode() const {
    std::string code = "typedef struct {\n";
    for (const auto& field : fields) {
        std::string fieldName = std::get<0>(field);
        std::string typeId = std::get<1>(field);
        code += "    " + TypeConverter::toC(typeId) + " " + fieldName + ";\n";
    }
    code += "} " + dtoName + ";\n";
    return code;
}

std::string StructuredDtoBuilder::generateRustStruct() const {
    std::string code = "#[derive(Serialize, Deserialize, Clone, Debug)]\npub struct " + dtoName + " {\n";
    for (const auto& field : fields) {
        std::string fieldName = std::get<0>(field);
        std::string typeId = std::get<1>(field);
        code += "    pub " + fieldName + ": " + TypeConverter::toRust(typeId) + ",\n";
    }
    code += "}\n";
    return code;
}

Json::Value StructuredDtoBuilder::toJson() const {
    Json::Value json;
    json["dtoName"] = dtoName;

    Json::Value fieldsArray(Json::arrayValue);
    for (const auto& field : fields) {
        Json::Value fieldJson;
        fieldJson["name"] = std::get<0>(field);
        fieldJson["type"] = std::get<1>(field);
        fieldJson["required"] = std::get<2>(field);
        fieldJson["description"] = std::get<3>(field);
        fieldsArray.append(fieldJson);
    }
    json["fields"] = fieldsArray;

    return json;
}

// ============================================================================
// ERROR MODEL
// ============================================================================

Json::Value ErrorModel::toJson() const {
    Json::Value json;
    json["errorCode"] = errorCode;
    json["severity"] = static_cast<int>(severity);
    json["message"] = message;
    json["description"] = description;

    Json::Value fieldsArray(Json::arrayValue);
    for (const auto& field : affectedFields) {
        fieldsArray.append(field);
    }
    json["affectedFields"] = fieldsArray;

    return json;
}

ErrorModel ErrorModel::fromJson(const Json::Value& json) {
    ErrorModel error;
    error.errorCode = json.get("errorCode", "").asString();
    error.severity = static_cast<ErrorSeverity>(json.get("severity", 2).asInt());
    error.message = json.get("message", "").asString();
    error.description = json.get("description", "").asString();
    if (json.isMember("affectedFields") && json["affectedFields"].isArray()) {
        for (const auto& f : json["affectedFields"]) {
            error.affectedFields.push_back(f.asString());
        }
    }
    return error;
}

ApiErrorCatalog ApiErrorCatalog::createDefault() {
    ApiErrorCatalog catalog;

    ErrorModel badRequest;
    badRequest.errorCode = "BAD_REQUEST";
    badRequest.severity = ErrorSeverity::ERROR;
    badRequest.message = "Invalid request parameters";
    catalog.registerError(badRequest);

    ErrorModel notFound;
    notFound.errorCode = "NOT_FOUND";
    notFound.severity = ErrorSeverity::ERROR;
    notFound.message = "Resource not found";
    catalog.registerError(notFound);

    ErrorModel internalError;
    internalError.errorCode = "INTERNAL_ERROR";
    internalError.severity = ErrorSeverity::CRITICAL;
    internalError.message = "Internal server error";
    catalog.registerError(internalError);

    return catalog;
}

void ApiErrorCatalog::registerError(const ErrorModel& error) {
    errors[error.errorCode] = error;
}

ErrorModel ApiErrorCatalog::getError(const std::string& errorCode) const {
    auto it = errors.find(errorCode);
    if (it != errors.end()) {
        return it->second;
    }
    throw std::runtime_error("Error not found: " + errorCode);
}

std::vector<ErrorModel> ApiErrorCatalog::getAllErrors() const {
    std::vector<ErrorModel> result;
    for (const auto& pair : errors) {
        result.push_back(pair.second);
    }
    return result;
}

// ============================================================================
// REQUEST BUILDER
// ============================================================================

RequestBuilder::RequestBuilder(const std::string& requestName) : requestName(requestName) {}

void RequestBuilder::addPathParameter(const std::string& paramName, const std::string& typeId) {
    pathParams[paramName] = typeId;
}

void RequestBuilder::addQueryParameter(const std::string& paramName, const std::string& typeId,
                                       bool required) {
    queryParams[paramName] = {typeId, required};
}

void RequestBuilder::addHeaderParameter(const std::string& paramName, const std::string& typeId) {
    headerParams[paramName] = typeId;
}

void RequestBuilder::setBodyType(const std::string& typeId) {
    bodyTypeId = typeId;
}

std::string RequestBuilder::getRequestName() const {
    return requestName;
}

Json::Value RequestBuilder::toJson() const {
    Json::Value json;
    json["requestName"] = requestName;
    json["bodyTypeId"] = bodyTypeId;

    Json::Value pObj(Json::objectValue);
    for (const auto& pair : pathParams) pObj[pair.first] = pair.second;
    json["pathParams"] = pObj;

    Json::Value qObj(Json::objectValue);
    for (const auto& pair : queryParams) {
        Json::Value item;
        item["typeId"] = pair.second.first;
        item["required"] = pair.second.second;
        qObj[pair.first] = item;
    }
    json["queryParams"] = qObj;

    Json::Value hObj(Json::objectValue);
    for (const auto& pair : headerParams) hObj[pair.first] = pair.second;
    json["headerParams"] = hObj;

    return json;
}

// ============================================================================
// RESPONSE BUILDER
// ============================================================================

ResponseBuilder::ResponseBuilder(const std::string& responseName, int statusCode)
    : responseName(responseName), statusCode(statusCode) {}

void ResponseBuilder::setBodyType(const std::string& typeId) {
    bodyTypeId = typeId;
}

void ResponseBuilder::addHeader(const std::string& headerName, const std::string& headerType) {
    headers[headerName] = headerType;
}

void ResponseBuilder::addPossibleError(const std::string& errorCode) {
    possibleErrors.push_back(errorCode);
}

std::string ResponseBuilder::getResponseName() const {
    return responseName;
}

int ResponseBuilder::getStatusCode() const {
    return statusCode;
}

Json::Value ResponseBuilder::toJson() const {
    Json::Value json;
    json["responseName"] = responseName;
    json["statusCode"] = statusCode;
    json["bodyTypeId"] = bodyTypeId;

    Json::Value hObj(Json::objectValue);
    for (const auto& pair : headers) hObj[pair.first] = pair.second;
    json["headers"] = hObj;

    Json::Value errArr(Json::arrayValue);
    for (const auto& err : possibleErrors) errArr.append(err);
    json["possibleErrors"] = errArr;

    return json;
}

} // namespace EmbeddedUI
