#include "CmsisSvdImporter.h"
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QDebug>

namespace driverpack {

QString CmsisSvdImporter::normalizeHex(const QString& str) {
    const QString trimmed = str.trimmed();
    if (trimmed.startsWith("0x", Qt::CaseInsensitive)) {
        bool ok = false;
        quint64 val = trimmed.toULongLong(&ok, 16);
        if (ok) {
            return QString("0x%1").arg(val, 0, 16).toLower();
        }
        return trimmed.toLower();
    }
    // If it's a decimal number
    bool ok = false;
    quint64 val = trimmed.toULongLong(&ok, 10);
    if (ok) {
        return QString("0x%1").arg(val, 0, 16).toLower();
    }
    return trimmed;
}

SvdImportResult CmsisSvdImporter::importFromFile(const QString& filePath) {
    SvdImportResult res;
    QFileInfo fi(filePath);
    if (!fi.exists() || !fi.isFile()) {
        res.errorMessage = QString("File not found: %1").arg(filePath);
        return res;
    }

    if (fi.size() > MAX_SVD_FILE_SIZE) {
        res.errorMessage = QString("File size (%1 bytes) exceeds maximum SVD limit of %2 bytes (10 MB)")
                               .arg(fi.size())
                               .arg(MAX_SVD_FILE_SIZE);
        return res;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        res.errorMessage = QString("Cannot open file for reading: %1").arg(filePath);
        return res;
    }

    return parseXml(file.readAll());
}

SvdImportResult CmsisSvdImporter::importFromString(const QString& xmlContent) {
    const QByteArray data = xmlContent.toUtf8();
    if (data.size() > MAX_SVD_FILE_SIZE) {
        SvdImportResult res;
        res.errorMessage = QString("Content size (%1 bytes) exceeds maximum limit of %2 bytes")
                               .arg(data.size())
                               .arg(MAX_SVD_FILE_SIZE);
        return res;
    }
    return parseXml(data);
}

SvdImportResult CmsisSvdImporter::parseXml(const QByteArray& xmlData) {
    SvdImportResult result;
    QXmlStreamReader reader(xmlData);

    int currentDepth = 0;
    CustomPeripheralDef currentPeriph;
    CustomRegisterDef currentReg;
    bool inDevice = false;
    bool inCpu = false;
    bool inPeripherals = false;
    bool inPeripheral = false;
    bool inRegisters = false;
    bool inRegister = false;

    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();

        if (token == QXmlStreamReader::DTD) {
            // Disallow or ignore external entity definitions
            const QString dtd = reader.text().toString();
            if (dtd.contains("SYSTEM", Qt::CaseInsensitive) && (dtd.contains("file:", Qt::CaseInsensitive) || dtd.contains("http", Qt::CaseInsensitive))) {
                result.errorMessage = "External entities are disallowed in SVD files (XXE prevention)";
                result.success = false;
                return result;
            }
        } else if (token == QXmlStreamReader::StartElement) {
            currentDepth++;
            if (currentDepth > MAX_XML_DEPTH) {
                result.errorMessage = QString("XML depth limit (%1) exceeded (malicious nesting detected)").arg(MAX_XML_DEPTH);
                result.success = false;
                return result;
            }

            const QString tag = reader.name().toString();

            if (tag == "device") {
                inDevice = true;
            } else if (tag == "cpu") {
                inCpu = true;
            } else if (tag == "peripherals") {
                inPeripherals = true;
            } else if (tag == "peripheral" && inPeripherals) {
                inPeripheral = true;
                currentPeriph = CustomPeripheralDef();
            } else if (tag == "registers" && inPeripheral) {
                inRegisters = true;
            } else if (tag == "register" && inRegisters) {
                inRegister = true;
                currentReg = CustomRegisterDef();
            } else if (inRegister) {
                if (tag == "name") {
                    currentReg.name = reader.readElementText().trimmed();
                    currentDepth--;
                } else if (tag == "description") {
                    currentReg.description = reader.readElementText().trimmed();
                    currentDepth--;
                } else if (tag == "addressOffset") {
                    currentReg.offset = normalizeHex(reader.readElementText().trimmed());
                    currentDepth--;
                } else if (tag == "resetValue") {
                    const QString rv = reader.readElementText().trimmed();
                    currentDepth--;
                    bool ok = false;
                    currentReg.resetValue = static_cast<uint32_t>(rv.toULongLong(&ok, 0));
                }
            } else if (inPeripheral && !inRegisters) {
                if (tag == "name") {
                    currentPeriph.name = reader.readElementText().trimmed();
                    currentDepth--;
                } else if (tag == "description") {
                    currentPeriph.description = reader.readElementText().trimmed();
                    currentDepth--;
                } else if (tag == "baseAddress") {
                    currentPeriph.baseAddress = normalizeHex(reader.readElementText().trimmed());
                    currentDepth--;
                } else if (tag == "groupName") {
                    currentPeriph.type = reader.readElementText().trimmed();
                    currentDepth--;
                }
            } else if (inCpu) {
                if (tag == "name") {
                    result.cpuName = reader.readElementText().trimmed();
                    currentDepth--;
                } else if (tag == "clockFrequency") {
                    result.clockFrequencyHz = reader.readElementText().trimmed().toUInt();
                    currentDepth--;
                }
            } else if (inDevice && !inCpu && !inPeripherals) {
                if (tag == "name") {
                    result.deviceName = reader.readElementText().trimmed();
                    currentDepth--;
                } else if (tag == "vendor") {
                    result.vendor = reader.readElementText().trimmed();
                    currentDepth--;
                }
            }
        } else if (token == QXmlStreamReader::EndElement) {
            currentDepth--;
            const QString tag = reader.name().toString();

            if (tag == "register" && inRegister) {
                if (!currentReg.name.isEmpty()) {
                    currentPeriph.registers.append(currentReg);
                }
                inRegister = false;
            } else if (tag == "registers") {
                inRegisters = false;
            } else if (tag == "peripheral" && inPeripheral) {
                if (!currentPeriph.name.isEmpty()) {
                    if (currentPeriph.type.isEmpty()) {
                        // Infer from name if groupName was missing (e.g. GPIOA -> GPIO)
                        if (currentPeriph.name.startsWith("GPIO", Qt::CaseInsensitive)) currentPeriph.type = "GPIO";
                        else if (currentPeriph.name.startsWith("USART", Qt::CaseInsensitive) || currentPeriph.name.startsWith("UART", Qt::CaseInsensitive)) currentPeriph.type = "USART";
                        else if (currentPeriph.name.startsWith("SPI", Qt::CaseInsensitive)) currentPeriph.type = "SPI";
                        else if (currentPeriph.name.startsWith("I2C", Qt::CaseInsensitive)) currentPeriph.type = "I2C";
                        else currentPeriph.type = "PERIPHERAL";
                    }
                    result.peripherals.append(currentPeriph);
                }
                inPeripheral = false;
            } else if (tag == "peripherals") {
                inPeripherals = false;
            } else if (tag == "cpu") {
                inCpu = false;
            } else if (tag == "device") {
                inDevice = false;
            }
        }
    }

    if (reader.hasError()) {
        result.errorMessage = QString("XML parse error at line %1, col %2: %3")
                                   .arg(reader.lineNumber())
                                   .arg(reader.columnNumber())
                                   .arg(reader.errorString());
        result.success = false;
        return result;
    }

    result.success = !result.peripherals.isEmpty() || !result.deviceName.isEmpty();
    if (!result.success && result.errorMessage.isEmpty()) {
        result.errorMessage = "No valid device or peripheral elements found in SVD";
    }

    return result;
}

} // namespace driverpack
