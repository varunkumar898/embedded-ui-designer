#pragma once

#include <QString>
#include <QStringList>

namespace driverpack {

struct SdkCheckResult {
    bool valid = false;
    bool detected = false;
    QString detectedSdkType;       // e.g. "STM32Cube", "ESP-IDF", "Raspberry Pi Pico SDK", "Generic Vendor SDK"
    QStringList missingFiles;
    QString message;
};

class SdkReferenceChecker {
public:
    static SdkCheckResult verifySdkDirectory(const QString& sdkPath, const QStringList& requiredFiles = QStringList());
    static QString detectVendorSdkType(const QString& sdkPath);
};

} // namespace driverpack
