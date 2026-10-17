#include "CustomPackModel.h"
#include <QRegularExpression>
#include <QSet>

namespace driverpack {

bool CustomPackValidator::isValidHexAddress(const QString& hex) {
    static const QRegularExpression hexRegex("^0x[0-9a-fA-F]+$");
    return hexRegex.match(hex.trimmed()).hasMatch();
}

bool CustomPackValidator::validateJson(const QJsonObject& json, QStringList& errors) {
    if (!json.contains("device_identity") || !json["device_identity"].isObject()) {
        errors.append("Missing required object 'device_identity'");
    } else {
        QJsonObject id = json["device_identity"].toObject();
        if (id.value("name").toString().trimmed().isEmpty()) errors.append("device_identity.name cannot be empty");
        if (id.value("vendor").toString().trimmed().isEmpty()) errors.append("device_identity.vendor cannot be empty");
        if (id.value("part_number").toString().trimmed().isEmpty()) errors.append("device_identity.part_number cannot be empty");
    }

    if (json.value("family").toString().trimmed().isEmpty()) {
        errors.append("Missing or empty required field 'family'");
    }
    if (json.value("core").toString().trimmed().isEmpty()) {
        errors.append("Missing or empty required field 'core'");
    }

    if (!json.contains("memory_regions") || !json["memory_regions"].isArray()) {
        errors.append("Missing required array 'memory_regions'");
    } else {
        QJsonArray memArr = json["memory_regions"].toArray();
        if (memArr.isEmpty()) {
            errors.append("memory_regions array cannot be empty");
        }
        for (int i = 0; i < memArr.size(); ++i) {
            QJsonObject mem = memArr[i].toObject();
            if (mem.value("name").toString().trimmed().isEmpty()) {
                errors.append(QString("memory_regions[%1].name cannot be empty").arg(i));
            }
            if (mem.value("size_kb").toInt(0) <= 0) {
                errors.append(QString("memory_regions[%1].size_kb must be greater than 0").arg(i));
            }
            const QString addr = mem.value("start_address").toString();
            if (!isValidHexAddress(addr)) {
                errors.append(QString("memory_regions[%1].start_address '%2' is not a valid hex address").arg(i).arg(addr));
            }
        }
    }

    QSet<QString> periphNames;
    if (json.contains("peripherals") && json["peripherals"].isArray()) {
        QJsonArray periphArr = json["peripherals"].toArray();
        for (int i = 0; i < periphArr.size(); ++i) {
            QJsonObject p = periphArr[i].toObject();
            const QString pName = p.value("name").toString().trimmed();
            if (pName.isEmpty()) {
                errors.append(QString("peripherals[%1].name cannot be empty").arg(i));
            } else {
                periphNames.insert(pName);
            }
            const QString bAddr = p.value("base_address").toString();
            if (!isValidHexAddress(bAddr)) {
                errors.append(QString("peripherals[%1].base_address '%2' is not a valid hex address").arg(i).arg(bAddr));
            }
        }
    }

    if (json.contains("pin_table") && json["pin_table"].isArray()) {
        QJsonArray pinArr = json["pin_table"].toArray();
        for (int i = 0; i < pinArr.size(); ++i) {
            QJsonObject pin = pinArr[i].toObject();
            if (pin.value("pin").toString().trimmed().isEmpty()) {
                errors.append(QString("pin_table[%1].pin cannot be empty").arg(i));
            }
            if (pin.value("function").toString().trimmed().isEmpty()) {
                errors.append(QString("pin_table[%1].function cannot be empty").arg(i));
            }
            const QString pRef = pin.value("peripheral").toString().trimmed();
            if (pRef.isEmpty()) {
                errors.append(QString("pin_table[%1].peripheral cannot be empty").arg(i));
            } else if (!periphNames.isEmpty() && !periphNames.contains(pRef)) {
                errors.append(QString("pin_table[%1] references unknown peripheral '%2'").arg(i).arg(pRef));
            }
        }
    }

    if (!json.contains("provenance") || !json["provenance"].isObject()) {
        errors.append("Missing required object 'provenance'");
    } else {
        QJsonObject prov = json["provenance"].toObject();
        if (prov.value("importer_version").toString().trimmed().isEmpty()) {
            errors.append("provenance.importer_version cannot be empty");
        }
        const QString genBy = prov.value("generated_by").toString().trimmed();
        if (genBy != "user" && genBy != "importer" && genBy != "ai") {
            errors.append(QString("provenance.generated_by must be 'user', 'importer', or 'ai' (found '%1')").arg(genBy));
        }
    }

    return errors.isEmpty();
}

bool CustomPackValidator::validatePack(const CustomPackDef& pack, QStringList& errors) {
    return validateJson(pack.toJson(), errors);
}

const CustomPeripheralDef* CustomPackDef::findPeripheral(const QString& periphName) const {
    for (const auto& p : peripherals) {
        if (p.name.compare(periphName, Qt::CaseInsensitive) == 0) {
            return &p;
        }
    }
    return nullptr;
}

bool CustomPackDef::hasPeripheral(const QString& periphName) const {
    return findPeripheral(periphName) != nullptr;
}

uint32_t CustomPackDef::totalRamKb() const {
    uint32_t total = 0;
    for (const auto& m : memoryRegions) {
        if (m.type == "sram" || m.type == "psram" || m.type == "tcm") {
            total += m.sizeKb;
        }
    }
    return total;
}

uint32_t CustomPackDef::totalFlashKb() const {
    uint32_t total = 0;
    for (const auto& m : memoryRegions) {
        if (m.type == "flash" || m.type == "rom") {
            total += m.sizeKb;
        }
    }
    return total;
}

bool CustomPackDef::isValid(QStringList* errors) const {
    QStringList localErrors;
    bool ok = CustomPackValidator::validatePack(*this, localErrors);
    if (errors) *errors = localErrors;
    return ok;
}

QJsonObject CustomPackDef::toJson() const {
    QJsonObject root;

    // device_identity
    QJsonObject id;
    id["name"] = name;
    id["vendor"] = vendor;
    id["part_number"] = partNumber;
    root["device_identity"] = id;

    root["family"] = family;
    root["core"] = core;
    root["clock_speed_mhz"] = static_cast<int>(clockSpeedMhz);

    // memory_regions
    QJsonArray memArr;
    for (const auto& m : memoryRegions) {
        QJsonObject mo;
        mo["name"] = m.name;
        mo["type"] = m.type;
        mo["start_address"] = m.startAddress;
        mo["size_kb"] = static_cast<int>(m.sizeKb);
        if (m.usableForFramebuffer >= 0) {
            mo["usable_for_framebuffer"] = (m.usableForFramebuffer == 1);
        } else {
            mo["usable_for_framebuffer"] = QJsonValue::Null;
        }
        memArr.append(mo);
    }
    root["memory_regions"] = memArr;

    // peripherals
    QJsonArray periphArr;
    for (const auto& p : peripherals) {
        QJsonObject po;
        po["name"] = p.name;
        po["base_address"] = p.baseAddress;
        po["type"] = p.type;
        po["description"] = p.description;

        QJsonArray regArr;
        for (const auto& r : p.registers) {
            QJsonObject ro;
            ro["name"] = r.name;
            ro["offset"] = r.offset;
            ro["description"] = r.description;
            if (r.resetValue != 0) {
                ro["reset_value"] = QString("0x%1").arg(r.resetValue, 8, 16, QChar('0'));
            }
            regArr.append(ro);
        }
        po["registers"] = regArr;
        periphArr.append(po);
    }
    root["peripherals"] = periphArr;

    // pin_table
    QJsonArray pinArr;
    for (const auto& pt : pinTable) {
        QJsonObject pto;
        pto["pin"] = pt.pin;
        pto["function"] = pt.function;
        pto["peripheral"] = pt.peripheral;
        pinArr.append(pto);
    }
    root["pin_table"] = pinArr;

    // board_wiring
    QJsonObject bw;
    bw["display_interface"] = boardWiring.displayInterface;
    bw["display_pins"] = boardWiring.displayPins;
    bw["touch_pins"] = boardWiring.touchPins;
    QJsonArray railsArr;
    for (const auto& r : boardWiring.powerRails) railsArr.append(r);
    bw["power_rails"] = railsArr;
    root["board_wiring"] = bw;

    // sdk_reference
    QJsonObject sdk;
    sdk["path"] = sdkReference.path;
    QJsonArray reqFilesArr;
    for (const auto& f : sdkReference.requiredFiles) reqFilesArr.append(f);
    sdk["required_files"] = reqFilesArr;
    sdk["detected"] = sdkReference.detected;
    root["sdk_reference"] = sdk;

    root["toolchain"] = toolchain;

    // templates_used
    QJsonArray templArr;
    for (const auto& t : templatesUsed) templArr.append(t);
    root["templates_used"] = templArr;

    // provenance
    QJsonObject prov;
    QJsonArray srcArr;
    for (const auto& s : provenance.sourceFiles) srcArr.append(s);
    prov["source_files"] = srcArr;
    prov["importer_version"] = provenance.importerVersion;
    prov["generated_by"] = provenance.generatedBy;
    prov["verified"] = provenance.verified; // false by default
    root["provenance"] = prov;

    return root;
}

CustomPackDef CustomPackDef::fromJson(const QJsonObject& obj, QStringList* errors) {
    CustomPackDef pack;

    if (errors) {
        CustomPackValidator::validateJson(obj, *errors);
    }

    if (obj.contains("device_identity")) {
        QJsonObject id = obj["device_identity"].toObject();
        pack.name = id.value("name").toString();
        pack.vendor = id.value("vendor").toString();
        pack.partNumber = id.value("part_number").toString();
    }

    pack.family = obj.value("family").toString();
    pack.core = obj.value("core").toString();
    pack.clockSpeedMhz = static_cast<uint32_t>(obj.value("clock_speed_mhz").toInt(64));

    if (obj.contains("memory_regions")) {
        for (const auto& v : obj["memory_regions"].toArray()) {
            QJsonObject mo = v.toObject();
            CustomMemoryRegion mr;
            mr.name = mo.value("name").toString();
            mr.type = mo.value("type").toString();
            mr.startAddress = mo.value("start_address").toString();
            mr.sizeKb = static_cast<uint32_t>(mo.value("size_kb").toInt(0));
            if (mo.value("usable_for_framebuffer").isNull()) {
                mr.usableForFramebuffer = -1;
            } else if (mo.value("usable_for_framebuffer").isBool()) {
                mr.usableForFramebuffer = mo.value("usable_for_framebuffer").toBool() ? 1 : 0;
            } else {
                mr.usableForFramebuffer = mo.value("usable_for_framebuffer").toInt(-1);
            }
            pack.memoryRegions.append(mr);
        }
    }

    if (obj.contains("peripherals")) {
        for (const auto& v : obj["peripherals"].toArray()) {
            QJsonObject po = v.toObject();
            CustomPeripheralDef pd;
            pd.name = po.value("name").toString();
            pd.baseAddress = po.value("base_address").toString();
            pd.type = po.value("type").toString();
            pd.description = po.value("description").toString();

            if (po.contains("registers")) {
                for (const auto& rv : po["registers"].toArray()) {
                    QJsonObject ro = rv.toObject();
                    CustomRegisterDef rd;
                    rd.name = ro.value("name").toString();
                    rd.offset = ro.value("offset").toString();
                    rd.description = ro.value("description").toString();
                    pd.registers.append(rd);
                }
            }
            pack.peripherals.append(pd);
        }
    }

    if (obj.contains("pin_table")) {
        for (const auto& v : obj["pin_table"].toArray()) {
            QJsonObject pto = v.toObject();
            CustomPinDef pin;
            pin.pin = pto.value("pin").toString();
            pin.function = pto.value("function").toString();
            pin.peripheral = pto.value("peripheral").toString();
            pack.pinTable.append(pin);
        }
    }

    if (obj.contains("board_wiring")) {
        QJsonObject bw = obj["board_wiring"].toObject();
        pack.boardWiring.displayInterface = bw.value("display_interface").toString();
        pack.boardWiring.displayPins = bw.value("display_pins").toObject();
        pack.boardWiring.touchPins = bw.value("touch_pins").toObject();
        for (const auto& r : bw.value("power_rails").toArray()) {
            pack.boardWiring.powerRails.append(r.toString());
        }
    }

    if (obj.contains("sdk_reference")) {
        QJsonObject sdk = obj["sdk_reference"].toObject();
        pack.sdkReference.path = sdk.value("path").toString();
        for (const auto& f : sdk.value("required_files").toArray()) {
            pack.sdkReference.requiredFiles.append(f.toString());
        }
        pack.sdkReference.detected = sdk.value("detected").toBool(false);
    }

    pack.toolchain = obj.value("toolchain").toString("arm-none-eabi-gcc");

    if (obj.contains("templates_used")) {
        for (const auto& t : obj["templates_used"].toArray()) {
            pack.templatesUsed.append(t.toString());
        }
    }

    if (obj.contains("provenance")) {
        QJsonObject prov = obj["provenance"].toObject();
        for (const auto& s : prov.value("source_files").toArray()) {
            pack.provenance.sourceFiles.append(s.toString());
        }
        pack.provenance.importerVersion = prov.value("importer_version").toString("1.0.0");
        pack.provenance.generatedBy = prov.value("generated_by").toString("importer");
        pack.provenance.verified = prov.value("verified").toBool(false);
    }

    return pack;
}

} // namespace driverpack
