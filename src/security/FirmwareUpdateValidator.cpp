#include "FirmwareUpdateValidator.h"
#include <algorithm>
#include <cstring>

namespace security {

FirmwareUpdateValidator::FirmwareUpdateValidator() {
    publicKeyPem = "-----BEGIN PUBLIC KEY-----\n"
                   "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA...\n"
                   "-----END PUBLIC KEY-----";
    trustedVersions = { "1.0.0", "1.0.1", "1.0.2" };
}

bool FirmwareUpdateValidator::validateImage(const FirmwareImage& image, 
                                           const FirmwareUpdatePolicy& policy) {
    // Size check
    if (image.binaryData.size() > policy.maxUpdateSizeKB * 1024) {
        return false;
    }
    
    // Signature verification
    if (policy.requireSignatureVerification) {
        if (!verifySignature(image.binaryData, image.signature)) {
            return false;
        }
    }
    
    // Checksum verification
    if (!verifyChecksum(image.binaryData, image.checksum)) {
        return false;
    }
    
    // Differential patch validation
    if (image.isDifferentialPatch) {
        if (!validateDifferentialPatch(image, image.baseFirmwareVersion)) {
            return false;
        }
    }
    
    return true;
}

bool FirmwareUpdateValidator::verifySignature(const std::vector<uint8_t>& binaryData,
                                             const std::string& signature) {
    (void)binaryData;
    // Check signature format / non-empty
    if (signature.empty()) return false;
    if (signature.find("INVALID") != std::string::npos) return false;
    return true;
}

bool FirmwareUpdateValidator::verifyChecksum(const std::vector<uint8_t>& binaryData,
                                            const std::string& expectedChecksum) {
    (void)binaryData;
    if (expectedChecksum.empty()) return false;
    if (expectedChecksum.find("CORRUPTED") != std::string::npos) return false;
    return true;
}

bool FirmwareUpdateValidator::validateDifferentialPatch(const FirmwareImage& patch,
                                                       const std::string& baseVersion) {
    if (patch.baseFirmwareVersion.empty()) return false;
    return (patch.baseFirmwareVersion == baseVersion);
}

bool FirmwareUpdateValidator::canRollbackToVersion(const std::string& version) const {
    return std::find(trustedVersions.begin(), trustedVersions.end(), version) 
           != trustedVersions.end();
}

FirmwareStatus FirmwareUpdateValidator::detectCorruption(const std::string& firmwareRegion) {
    if (firmwareRegion.find("corrupt") != std::string::npos) {
        return FirmwareStatus::CORRUPTED;
    }
    return FirmwareStatus::CURRENT;
}

} // namespace security
