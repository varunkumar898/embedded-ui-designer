#pragma once

#include "CustomPackModel.h"
#include <QString>
#include <QVector>

namespace driverpack {

struct SvdImportResult {
    bool success = false;
    QString errorMessage;
    QString deviceName;
    QString vendor;
    QString cpuName;
    uint32_t clockFrequencyHz = 0;
    QVector<CustomPeripheralDef> peripherals;
};

class CmsisSvdImporter {
public:
    static constexpr qint64 MAX_SVD_FILE_SIZE = 10 * 1024 * 1024; // 10 MB limit
    static constexpr int MAX_XML_DEPTH = 64;                       // Max element nesting depth

    static SvdImportResult importFromFile(const QString& filePath);
    static SvdImportResult importFromString(const QString& xmlContent);

private:
    static SvdImportResult parseXml(const QByteArray& xmlData);
    static QString normalizeHex(const QString& str);
};

} // namespace driverpack
