#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace security {

enum class FirmwareStatus {
    CURRENT = 0,
    AVAILABLE = 1,
    INSTALLING = 2,
    VERIFICATION_FAILED = 3,
    CORRUPTED = 4,
};

struct FirmwareImage {
    std::string version; // "1.0.5"
    std::string checksum; // SHA-256 hex
    std::string signature; // RSA-2048 signature
    std::vector<uint8_t> binaryData;
    std::string releaseNotes;
    bool isDifferentialPatch{false};
    std::string baseFirmwareVersion; // For patches
};

struct FirmwareUpdatePolicy {
    bool allowDowngrades{false};
    bool requireSignatureVerification{true};
    bool enableDifferentialPatches{true};
    uint32_t maxUpdateSizeKB{4096};
};

class FirmwareUpdateValidator {
public:
    FirmwareUpdateValidator();
    
    // Validate firmware image
    bool validateImage(const FirmwareImage& image, const FirmwareUpdatePolicy& policy);
    
    // Verify cryptographic signature
    bool verifySignature(const std::vector<uint8_t>& binaryData, 
                        const std::string& signature);
    
    // Verify checksum
    bool verifyChecksum(const std::vector<uint8_t>& binaryData, 
                       const std::string& expectedChecksum);
    
    // Check differential patch integrity
    bool validateDifferentialPatch(const FirmwareImage& patch, 
                                   const std::string& baseVersion);
    
    // Get firmware rollback capability
    bool canRollbackToVersion(const std::string& version) const;
    void addTrustedVersion(const std::string& version) { trustedVersions.push_back(version); }
    
    // Detect firmware corruption on device
    FirmwareStatus detectCorruption(const std::string& firmwareRegion);
    
private:
    std::string publicKeyPem;
    std::vector<std::string> trustedVersions;
};

} // namespace security
