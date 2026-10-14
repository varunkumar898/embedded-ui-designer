#pragma once

#include "ProjectModel.h"
#include <string>
#include <memory>

namespace EmbeddedUI {

// ============================================================================
// PROJECT SERIALIZER - With Versioning & Migration
// ============================================================================

class ProjectSerializer {
public:
    // Latest format version
    static const int CURRENT_FORMAT_VERSION;

    // Serialization (Project -> JSON)
    static Json::Value serialize(const Project& project);
    static bool writeToFile(const Project& project, const std::string& filePath, std::string& errorMessage);

    // Deserialization (JSON -> Project) with automatic migration
    static Project deserialize(const Json::Value& json, std::string& errorMessage);
    static Project readFromFile(const std::string& filePath, std::string& errorMessage);

    // Version detection & migration
    static int detectFormatVersion(const Json::Value& json);
    static bool migrateFormat(Json::Value& json, int fromVersion, int toVersion,
                             std::string& errorMessage);

    // Backwards compatibility check
    static bool isBackwardsCompatible(int fromVersion, int toVersion);
};

// ============================================================================
// MIGRATION STRATEGIES
// ============================================================================

class MigrationV0_1_to_V0_2 {
public:
    static bool migrate(Json::Value& projectJson, std::string& errorMessage);
private:
    static void addNewBuildConfigSchema(Json::Value& projectJson);
};

class MigrationV0_2_to_V1_0 {
public:
    static bool migrate(Json::Value& projectJson, std::string& errorMessage);
private:
    static void upgradeTypeSystem(Json::Value& projectJson);
    static void upgradeApiSchema(Json::Value& projectJson);
};

} // namespace EmbeddedUI
