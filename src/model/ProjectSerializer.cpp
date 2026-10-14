#include "ProjectSerializer.h"
#include <fstream>
#include <sstream>

namespace EmbeddedUI {

const int ProjectSerializer::CURRENT_FORMAT_VERSION = 1;

Json::Value ProjectSerializer::serialize(const Project& project) {
    Json::Value root;

    root["formatVersion"] = CURRENT_FORMAT_VERSION;
    root["metadata"] = project.getMetadata().toJson();

    // Serialize API schema
    root["apiSchema"] = project.getApiSchema().toJson();

    // Serialize pages
    Json::Value pagesArray(Json::arrayValue);
    for (const auto& page : project.getAllPages()) {
        pagesArray.append(page.toJson());
    }
    root["pages"] = pagesArray;

    // Serialize device operations
    Json::Value opsArray(Json::arrayValue);
    for (const auto& op : project.getAllDeviceOperations()) {
        opsArray.append(op.toJson());
    }
    root["deviceOperations"] = opsArray;

    // Serialize build configs
    Json::Value configsArray(Json::arrayValue);
    for (const auto& config : project.getAllBuildConfigs()) {
        configsArray.append(config.toJson());
    }
    root["buildConfigs"] = configsArray;

    return root;
}

bool ProjectSerializer::writeToFile(const Project& project, const std::string& filePath,
                                    std::string& errorMessage) {
    try {
        Json::Value projectJson = serialize(project);
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "  ";
        std::ofstream outFile(filePath);

        if (!outFile.is_open()) {
            errorMessage = "Cannot open file for writing: " + filePath;
            return false;
        }

        std::unique_ptr<Json::StreamWriter> jsonWriter(writer.newStreamWriter());
        jsonWriter->write(projectJson, &outFile);
        outFile.close();

        return true;
    } catch (const std::exception& e) {
        errorMessage = std::string("Serialization error: ") + e.what();
        return false;
    }
}

Project ProjectSerializer::deserialize(const Json::Value& json, std::string& errorMessage) {
    try {
        int formatVersion = detectFormatVersion(json);

        // Migrate if necessary
        Json::Value migratedJson = json;
        if (formatVersion < CURRENT_FORMAT_VERSION) {
            if (!migrateFormat(migratedJson, formatVersion, CURRENT_FORMAT_VERSION, errorMessage)) {
                return Project("error");
            }
        }

        ProjectMetadata metadata;
        if (migratedJson.isMember("metadata")) {
            metadata = ProjectMetadata::fromJson(migratedJson["metadata"]);
        } else {
            metadata.projectName = migratedJson.get("projectName", "Untitled Project").asString();
            metadata.version = "1.0.0";
        }

        Project project(metadata.projectName);
        project.setMetadata(metadata);

        // Deserialize API Schema
        if (migratedJson.isMember("apiSchema")) {
            ApiSchema schema = ApiSchema::fromJson(migratedJson["apiSchema"]);
            project.getApiSchema() = schema;
        }

        // Deserialize pages
        if (migratedJson.isMember("pages") && migratedJson["pages"].isArray()) {
            for (const auto& pageJson : migratedJson["pages"]) {
                project.addPage(UiPage::fromJson(pageJson));
            }
        }

        // Deserialize device operations
        if (migratedJson.isMember("deviceOperations") && migratedJson["deviceOperations"].isArray()) {
            for (const auto& opJson : migratedJson["deviceOperations"]) {
                project.registerDeviceOperation(DeviceOperation::fromJson(opJson));
            }
        }

        // Deserialize build configs
        if (migratedJson.isMember("buildConfigs") && migratedJson["buildConfigs"].isArray()) {
            for (const auto& configJson : migratedJson["buildConfigs"]) {
                project.addBuildConfig(BuildConfig::fromJson(configJson));
            }
        }

        return project;
    } catch (const std::exception& e) {
        errorMessage = std::string("Deserialization error: ") + e.what();
        return Project("error");
    }
}

Project ProjectSerializer::readFromFile(const std::string& filePath, std::string& errorMessage) {
    try {
        std::ifstream inFile(filePath);
        if (!inFile.is_open()) {
            errorMessage = "Cannot open file for reading: " + filePath;
            return Project("error");
        }

        Json::Value json;
        inFile >> json;
        inFile.close();

        return deserialize(json, errorMessage);
    } catch (const std::exception& e) {
        errorMessage = std::string("File read error: ") + e.what();
        return Project("error");
    }
}

int ProjectSerializer::detectFormatVersion(const Json::Value& json) {
    if (json.isMember("formatVersion")) {
        return json["formatVersion"].asInt();
    }
    return 0; // Legacy format
}

bool ProjectSerializer::migrateFormat(Json::Value& json, int fromVersion, int toVersion,
                                      std::string& errorMessage) {
    if (fromVersion >= toVersion) {
        return true;
    }

    if (fromVersion == 0 && toVersion >= 1) {
        if (!MigrationV0_1_to_V0_2::migrate(json, errorMessage)) {
            return false;
        }
    }

    if (fromVersion <= 1 && toVersion >= 2) {
        if (!MigrationV0_2_to_V1_0::migrate(json, errorMessage)) {
            return false;
        }
    }

    return true;
}

bool ProjectSerializer::isBackwardsCompatible(int fromVersion, int toVersion) {
    if (fromVersion == 0 && toVersion == 1) return true;
    if (fromVersion == 1 && toVersion == 2) return true;
    return fromVersion == toVersion;
}

// ============================================================================
// MIGRATION IMPLEMENTATIONS
// ============================================================================

bool MigrationV0_1_to_V0_2::migrate(Json::Value& projectJson, std::string& errorMessage) {
    try {
        addNewBuildConfigSchema(projectJson);
        projectJson["formatVersion"] = 1;
        return true;
    } catch (const std::exception& e) {
        errorMessage = std::string("Migration 0.1->0.2 failed: ") + e.what();
        return false;
    }
}

void MigrationV0_1_to_V0_2::addNewBuildConfigSchema(Json::Value& projectJson) {
    if (!projectJson.isMember("buildConfigs")) {
        projectJson["buildConfigs"] = Json::arrayValue;
    }
}

bool MigrationV0_2_to_V1_0::migrate(Json::Value& projectJson, std::string& errorMessage) {
    try {
        upgradeTypeSystem(projectJson);
        upgradeApiSchema(projectJson);
        projectJson["formatVersion"] = 2;
        return true;
    } catch (const std::exception& e) {
        errorMessage = std::string("Migration 0.2->1.0 failed: ") + e.what();
        return false;
    }
}

void MigrationV0_2_to_V1_0::upgradeTypeSystem(Json::Value& /*projectJson*/) {
}

void MigrationV0_2_to_V1_0::upgradeApiSchema(Json::Value& /*projectJson*/) {
}

} // namespace EmbeddedUI
