#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include "json/json.h"

namespace EmbeddedUI {

// ============================================================================
// DTO - Data Transfer Object Base Classes
// ============================================================================

class BaseDto {
public:
    virtual ~BaseDto() = default;
    virtual Json::Value toJson() const = 0;
    virtual std::string getTypeName() const = 0;
};

// ============================================================================
// VALIDATION RULES
// ============================================================================

enum class ValidationRuleType {
    MIN_LENGTH,
    MAX_LENGTH,
    REGEX_PATTERN,
    MIN_VALUE,
    MAX_VALUE,
    ENUM_VALUES,
    CUSTOM
};

struct ValidationRule {
    ValidationRuleType ruleType = ValidationRuleType::CUSTOM;
    std::string description;
    std::string ruleExpression;  // Pattern or expression

    bool validate(const std::string& value) const;
    Json::Value toJson() const;
    static ValidationRule fromJson(const Json::Value& json);
};

// ============================================================================
// TYPE CONVERTERS - For Multi-Language Code Generation
// ============================================================================

class TypeConverter {
public:
    // Convert type to language-specific representation
    static std::string toCpp(const std::string& typeId);
    static std::string toC(const std::string& typeId);
    static std::string toRust(const std::string& typeId);
    static std::string toPython(const std::string& typeId);

    // Get default value for type
    static std::string getDefaultValue(const std::string& typeId);

    // Check if types are compatible
    static bool areCompatible(const std::string& typeId1, const std::string& typeId2);
};

// ============================================================================
// STRUCTURED DTO GENERATOR
// ============================================================================

class StructuredDtoBuilder {
public:
    explicit StructuredDtoBuilder(const std::string& dtoName);

    void addField(const std::string& fieldName, const std::string& typeId,
                  bool required = true, const std::string& description = "");

    void addValidationRule(const std::string& fieldName, const ValidationRule& rule);

    // Generate code for different languages
    std::string generateCppStruct() const;
    std::string generateCppSerialization() const;
    std::string generateCCode() const;
    std::string generateRustStruct() const;

    Json::Value toJson() const;

private:
    std::string dtoName;
    std::vector<std::tuple<std::string, std::string, bool, std::string>> fields;
    std::map<std::string, std::vector<ValidationRule>> validationRules;
};

// ============================================================================
// ERROR MODEL
// ============================================================================

enum class ErrorSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

struct ErrorModel {
    std::string errorCode;
    ErrorSeverity severity = ErrorSeverity::ERROR;
    std::string message;
    std::string description;
    std::vector<std::string> affectedFields;

    Json::Value toJson() const;
    static ErrorModel fromJson(const Json::Value& json);
};

class ApiErrorCatalog {
public:
    void registerError(const ErrorModel& error);
    ErrorModel getError(const std::string& errorCode) const;
    std::vector<ErrorModel> getAllErrors() const;

    // Standard HTTP errors
    static ApiErrorCatalog createDefault();

private:
    std::map<std::string, ErrorModel> errors;
};

// ============================================================================
// REQUEST/RESPONSE BUILDERS
// ============================================================================

class RequestBuilder {
public:
    explicit RequestBuilder(const std::string& requestName);

    void addPathParameter(const std::string& paramName, const std::string& typeId);
    void addQueryParameter(const std::string& paramName, const std::string& typeId, bool required = false);
    void addHeaderParameter(const std::string& paramName, const std::string& typeId);
    void setBodyType(const std::string& typeId);

    std::string getRequestName() const;
    Json::Value toJson() const;

private:
    std::string requestName;
    std::map<std::string, std::string> pathParams;
    std::map<std::string, std::pair<std::string, bool>> queryParams;
    std::map<std::string, std::string> headerParams;
    std::string bodyTypeId;
};

class ResponseBuilder {
public:
    ResponseBuilder(const std::string& responseName, int statusCode);

    void setBodyType(const std::string& typeId);
    void addHeader(const std::string& headerName, const std::string& headerType);
    void addPossibleError(const std::string& errorCode);

    std::string getResponseName() const;
    int getStatusCode() const;
    Json::Value toJson() const;

private:
    std::string responseName;
    int statusCode = 200;
    std::string bodyTypeId;
    std::map<std::string, std::string> headers;
    std::vector<std::string> possibleErrors;
};

} // namespace EmbeddedUI
