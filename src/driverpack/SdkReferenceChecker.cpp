#include "SdkReferenceChecker.h"
#include <QDir>
#include <QFileInfo>

namespace driverpack {

QString SdkReferenceChecker::detectVendorSdkType(const QString& sdkPath) {
    QDir dir(sdkPath);
    if (!dir.exists()) return QString();

    if (dir.exists("Drivers/CMSIS") || dir.exists("Drivers")) {
        return "STM32Cube";
    }
    if (dir.exists("components/esp_hw_support") || dir.exists("components/driver") || dir.exists("tools/idf.py")) {
        return "ESP-IDF";
    }
    if (dir.exists("pico_sdk_init.cmake") || dir.exists("src/rp2_common")) {
        return "Raspberry Pi Pico SDK";
    }
    if (dir.exists("drivers/fsl_common.h") || dir.exists("devices/MIMXRT1062")) {
        return "NXP MCUXpresso";
    }
    if (dir.exists("zephyr/include/zephyr.h") || dir.exists("zephyr")) {
        return "Nordic nRF Connect / Zephyr";
    }
    return "Generic Vendor SDK";
}

SdkCheckResult SdkReferenceChecker::verifySdkDirectory(const QString& sdkPath, const QStringList& requiredFiles) {
    SdkCheckResult result;
    const QString trimmed = sdkPath.trimmed();

    if (trimmed.isEmpty()) {
        result.message = "SDK path is empty";
        return result;
    }

    QDir dir(trimmed);
    if (!dir.exists()) {
        result.message = QString("Directory does not exist: %1").arg(trimmed);
        return result;
    }

    result.detectedSdkType = detectVendorSdkType(trimmed);
    result.detected = !result.detectedSdkType.isEmpty() && result.detectedSdkType != "Generic Vendor SDK";

    for (const QString& req : requiredFiles) {
        if (!dir.exists(req)) {
            result.missingFiles.append(req);
        }
    }

    result.valid = result.missingFiles.isEmpty();
    if (result.valid) {
        result.message = QString("SDK verified: %1 (%2)").arg(result.detectedSdkType, trimmed);
    } else {
        result.message = QString("SDK missing %1 required files").arg(result.missingFiles.size());
    }

    return result;
}

} // namespace driverpack
