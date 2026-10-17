#include "PinTableParser.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace driverpack {

PinTableParseResult PinTableParser::parseCsvFile(const QString& filePath, const QSet<QString>& allowedPeripherals) {
    PinTableParseResult res;
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) {
        res.errors.append(QString("File not found: %1").arg(filePath));
        return res;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        res.errors.append(QString("Cannot open file for reading: %1").arg(filePath));
        return res;
    }

    return parseCsvString(QString::fromUtf8(file.readAll()), allowedPeripherals);
}

PinTableParseResult PinTableParser::parseCsvString(const QString& csvContent, const QSet<QString>& allowedPeripherals) {
    PinTableParseResult res;
    const QStringList lines = csvContent.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

    if (lines.isEmpty()) {
        res.errors.append("CSV content is empty");
        return res;
    }

    int pinCol = -1;
    int fnCol = -1;
    int periphCol = -1;
    bool hasHeader = false;

    // Detect header row
    const QString firstLine = lines.first();
    const QString delimiter = firstLine.contains(';') ? ";" : ",";
    const QStringList firstCols = firstLine.split(delimiter);

    for (int i = 0; i < firstCols.size(); ++i) {
        const QString col = firstCols[i].trimmed().toLower();
        if (col == "pin" || col == "pin_name" || col == "name") {
            pinCol = i;
            hasHeader = true;
        } else if (col == "function" || col == "func" || col == "signal" || col == "mode") {
            fnCol = i;
            hasHeader = true;
        } else if (col == "peripheral" || col == "periph" || col == "module") {
            periphCol = i;
            hasHeader = true;
        }
    }

    int startLine = 0;
    if (hasHeader) {
        startLine = 1;
        if (pinCol < 0) pinCol = 0;
        if (fnCol < 0) fnCol = 1;
        if (periphCol < 0) periphCol = 2;
    } else {
        // Default column order: Pin, Function, Peripheral
        pinCol = 0;
        fnCol = 1;
        periphCol = 2;
    }

    for (int lineIdx = startLine; lineIdx < lines.size(); ++lineIdx) {
        const QString line = lines[lineIdx].trimmed();
        if (line.isEmpty() || line.startsWith("#") || line.startsWith("//")) continue;

        const QStringList cols = line.split(delimiter);
        if (cols.size() <= qMax(pinCol, qMax(fnCol, periphCol))) {
            res.warnings.append(QString("Line %1: Insufficient columns (found %2)").arg(lineIdx + 1).arg(cols.size()));
            continue;
        }

        CustomPinDef p;
        p.pin = cols[pinCol].trimmed();
        p.function = cols[fnCol].trimmed();
        p.peripheral = cols[periphCol].trimmed();

        if (p.pin.isEmpty()) {
            res.warnings.append(QString("Line %1: Empty pin name ignored").arg(lineIdx + 1));
            continue;
        }

        // Validation against SVD peripheral list if provided
        if (!allowedPeripherals.isEmpty()) {
            bool matched = false;
            for (const QString& ap : allowedPeripherals) {
                if (ap.compare(p.peripheral, Qt::CaseInsensitive) == 0) {
                    matched = true;
                    p.peripheral = ap; // canonicalize
                    break;
                }
            }
            if (!matched) {
                res.errors.append(QString("Line %1: Pin '%2' references unknown peripheral '%3'")
                                      .arg(lineIdx + 1)
                                      .arg(p.pin)
                                      .arg(p.peripheral));
            }
        }

        res.pins.append(p);
    }

    res.success = res.errors.isEmpty() && !res.pins.isEmpty();
    if (res.pins.isEmpty() && res.errors.isEmpty()) {
        res.errors.append("No valid pins parsed from CSV");
    }

    return res;
}

QString PinTableParser::exportToCsv(const QVector<CustomPinDef>& pins) {
    QString out = "Pin,Function,Peripheral\n";
    for (const auto& p : pins) {
        out += QString("%1,%2,%3\n").arg(p.pin, p.function, p.peripheral);
    }
    return out;
}

} // namespace driverpack
