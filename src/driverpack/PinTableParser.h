#pragma once

#include "CustomPackModel.h"
#include <QString>
#include <QStringList>
#include <QVector>
#include <QSet>

namespace driverpack {

struct PinTableParseResult {
    bool success = false;
    QVector<CustomPinDef> pins;
    QStringList errors;
    QStringList warnings;
};

class PinTableParser {
public:
    static PinTableParseResult parseCsvString(const QString& csvContent, const QSet<QString>& allowedPeripherals = QSet<QString>());
    static PinTableParseResult parseCsvFile(const QString& filePath, const QSet<QString>& allowedPeripherals = QSet<QString>());
    static QString exportToCsv(const QVector<CustomPinDef>& pins);
};

} // namespace driverpack
