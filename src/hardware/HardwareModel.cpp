#include "HardwareModel.h"

namespace Hardware {

// ── PinDefinition ──

QJsonObject PinDefinition::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["physicalPin"] = physicalPin;
    if (!description.isEmpty()) obj["description"] = description;
    QJsonArray afArray;
    for (const QString& af : alternateFunctions) afArray.append(af);
    obj["alternateFunctions"] = afArray;
    if (!defaultFunction.isEmpty()) obj["defaultFunction"] = defaultFunction;
    if (isPower) obj["isPower"] = true;
    if (isGround) obj["isGround"] = true;
    if (isReset) obj["isReset"] = true;
    if (isReserved) obj["isReserved"] = true;
    if (isBoot) obj["isBoot"] = true;
    return obj;
}

PinDefinition PinDefinition::fromJson(const QJsonObject& obj) {
    PinDefinition pin;
    pin.name = obj.value("name").toString();
    pin.physicalPin = obj.value("physicalPin").toInt();
    pin.description = obj.value("description").toString();
    QJsonArray afArray = obj.value("alternateFunctions").toArray();
    for (const QJsonValue& v : afArray) pin.alternateFunctions.append(v.toString());
    pin.defaultFunction = obj.value("defaultFunction").toString();
    pin.isPower = obj.value("isPower").toBool(false);
    pin.isGround = obj.value("isGround").toBool(false);
    pin.isReset = obj.value("isReset").toBool(false);
    pin.isReserved = obj.value("isReserved").toBool(false);
    pin.isBoot = obj.value("isBoot").toBool(false);
    return pin;
}

// ── PeripheralDescriptor ──

QJsonObject PeripheralDescriptor::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["type"] = type;
    QJsonObject sigObj;
    for (auto it = signalOptions.begin(); it != signalOptions.end(); ++it) {
        QJsonArray arr;
        for (const QString& p : it.value()) arr.append(p);
        sigObj[it.key()] = arr;
    }
    obj["signals"] = sigObj;
    obj["defaults"] = defaults;
    return obj;
}

PeripheralDescriptor PeripheralDescriptor::fromJson(const QJsonObject& obj) {
    PeripheralDescriptor desc;
    desc.name = obj.value("name").toString();
    desc.type = obj.value("type").toString();
    QJsonObject sigObj = obj.value("signals").toObject();
    for (auto it = sigObj.begin(); it != sigObj.end(); ++it) {
        QStringList pinList;
        for (const QJsonValue& v : it.value().toArray()) pinList.append(v.toString());
        desc.signalOptions[it.key()] = pinList;
    }
    desc.defaults = obj.value("defaults").toObject();
    return desc;
}

// ── DeviceDefinition ──

const PinDefinition* DeviceDefinition::findPin(const QString& pinName) const {
    for (const auto& pin : pins) {
        if (pin.name.compare(pinName, Qt::CaseInsensitive) == 0) return &pin;
    }
    return nullptr;
}

const PeripheralDescriptor* DeviceDefinition::findPeripheral(const QString& periName) const {
    for (const auto& desc : peripheralDescriptors) {
        if (desc.name.compare(periName, Qt::CaseInsensitive) == 0) return &desc;
    }
    return nullptr;
}

QStringList DeviceDefinition::availablePinsForSignal(const QString& periName, const QString& signalName) const {
    const PeripheralDescriptor* desc = findPeripheral(periName);
    if (!desc) return QStringList();
    return desc->signalOptions.value(signalName);
}

QJsonObject DeviceDefinition::toJson() const {
    QJsonObject obj;
    obj["schemaVersion"] = "1.0";

    QJsonObject provObj;
    provObj["source"] = sourceProvenance;
    provObj["vendor"] = vendor;
    provObj["partNumber"] = partNumber;
    provObj["version"] = version;
    obj["provenance"] = provObj;

    QJsonObject ident;
    ident["vendor"] = vendor;
    ident["family"] = family;
    ident["series"] = series;
    ident["partNumber"] = partNumber;
    ident["architecture"] = architecture;
    ident["core"] = core;
    ident["package"] = package;
    ident["flash"] = static_cast<qint64>(flashBytes);
    ident["ram"] = static_cast<qint64>(ramBytes);
    ident["eeprom"] = static_cast<qint64>(eepromBytes);
    ident["minVoltage"] = minVoltage;
    ident["maxVoltage"] = maxVoltage;
    ident["maxClockMhz"] = maxClockMhz;
    ident["gpioCount"] = gpioCount;
    obj["identity"] = ident;

    QJsonObject perCounts;
    for (auto it = peripheralsCount.begin(); it != peripheralsCount.end(); ++it) {
        perCounts[it.key()] = it.value();
    }
    obj["peripherals"] = perCounts;

    QJsonArray descArr;
    for (const auto& d : peripheralDescriptors) descArr.append(d.toJson());
    obj["peripheralDescriptors"] = descArr;

    QJsonArray pinArr;
    for (const auto& p : pins) pinArr.append(p.toJson());
    obj["pins"] = pinArr;

    obj["clockTree"] = clockTree;
    return obj;
}

DeviceDefinition DeviceDefinition::fromJson(const QJsonObject& obj) {
    DeviceDefinition dev;
    QJsonObject provObj = obj.value("provenance").toObject();
    dev.sourceProvenance = provObj.value("source").toString();
    dev.version = provObj.value("version").toString("1.0.0");

    QJsonObject ident = obj.value("identity").toObject();
    dev.vendor = ident.value("vendor").toString();
    dev.family = ident.value("family").toString();
    dev.series = ident.value("series").toString();
    dev.partNumber = ident.value("partNumber").toString();
    dev.architecture = ident.value("architecture").toString();
    dev.core = ident.value("core").toString();
    dev.package = ident.value("package").toString();
    dev.flashBytes = static_cast<quint64>(ident.value("flash").toVariant().toLongLong());
    dev.ramBytes = static_cast<quint64>(ident.value("ram").toVariant().toLongLong());
    dev.eepromBytes = static_cast<quint64>(ident.value("eeprom").toVariant().toLongLong());
    dev.minVoltage = ident.value("minVoltage").toDouble();
    dev.maxVoltage = ident.value("maxVoltage").toDouble();
    dev.maxClockMhz = ident.value("maxClockMhz").toInt();
    dev.gpioCount = ident.value("gpioCount").toInt();

    QJsonObject perCounts = obj.value("peripherals").toObject();
    for (auto it = perCounts.begin(); it != perCounts.end(); ++it) {
        dev.peripheralsCount[it.key()] = it.value().toInt();
    }

    QJsonArray descArr = obj.value("peripheralDescriptors").toArray();
    for (const QJsonValue& v : descArr) {
        dev.peripheralDescriptors.append(PeripheralDescriptor::fromJson(v.toObject()));
    }

    QJsonArray pinArr = obj.value("pins").toArray();
    for (const QJsonValue& v : pinArr) {
        dev.pins.append(PinDefinition::fromJson(v.toObject()));
    }

    dev.clockTree = obj.value("clockTree").toObject();
    return dev;
}

// ── BoardConnector ──

QJsonObject BoardConnector::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["pinCount"] = pinCount;
    QJsonObject mapObj;
    for (auto it = pinMapping.begin(); it != pinMapping.end(); ++it) {
        mapObj[QString::number(it.key())] = it.value();
    }
    obj["pinMapping"] = mapObj;
    return obj;
}

BoardConnector BoardConnector::fromJson(const QJsonObject& obj) {
    BoardConnector conn;
    conn.name = obj.value("name").toString();
    conn.pinCount = obj.value("pinCount").toInt();
    QJsonObject mapObj = obj.value("pinMapping").toObject();
    for (auto it = mapObj.begin(); it != mapObj.end(); ++it) {
        conn.pinMapping[it.key().toInt()] = it.value().toString();
    }
    return conn;
}

// ── BoardDefinition ──

QJsonObject BoardDefinition::toJson() const {
    QJsonObject obj;
    obj["schemaVersion"] = "1.0";

    QJsonObject prov;
    prov["source"] = sourceProvenance;
    prov["vendor"] = manufacturer;
    prov["boardId"] = id;
    prov["version"] = version;
    obj["provenance"] = prov;

    QJsonObject meta;
    meta["id"] = id;
    meta["name"] = name;
    meta["manufacturer"] = manufacturer;
    meta["boardFamily"] = boardFamily;
    meta["mcuPartNumber"] = mcuPartNumber;
    meta["architecture"] = architecture;
    meta["debugInterface"] = debugInterface;
    meta["supplyVoltage"] = supplyVoltage;
    obj["metadata"] = meta;

    QJsonObject pinLabels;
    for (auto it = physicalPinLabels.begin(); it != physicalPinLabels.end(); ++it) {
        pinLabels[it.key()] = it.value();
    }
    obj["physicalPinLabels"] = pinLabels;

    QJsonArray connArr;
    for (const auto& c : connectors) connArr.append(c.toJson());
    obj["connectors"] = connArr;

    obj["onboardPeripherals"] = onboardPeripherals;
    return obj;
}

BoardDefinition BoardDefinition::fromJson(const QJsonObject& obj) {
    BoardDefinition b;
    QJsonObject prov = obj.value("provenance").toObject();
    b.sourceProvenance = prov.value("source").toString();
    b.version = prov.value("version").toString("1.0.0");

    QJsonObject meta = obj.value("metadata").toObject();
    b.id = meta.value("id").toString();
    b.name = meta.value("name").toString();
    b.manufacturer = meta.value("manufacturer").toString();
    b.boardFamily = meta.value("boardFamily").toString();
    b.mcuPartNumber = meta.value("mcuPartNumber").toString();
    b.architecture = meta.value("architecture").toString();
    b.debugInterface = meta.value("debugInterface").toString();
    b.supplyVoltage = meta.value("supplyVoltage").toString();

    QJsonObject pinLabels = obj.value("physicalPinLabels").toObject();
    for (auto it = pinLabels.begin(); it != pinLabels.end(); ++it) {
        b.physicalPinLabels[it.key()] = it.value().toString();
    }

    QJsonArray connArr = obj.value("connectors").toArray();
    for (const QJsonValue& v : connArr) {
        b.connectors.append(BoardConnector::fromJson(v.toObject()));
    }

    b.onboardPeripherals = obj.value("onboardPeripherals").toObject();
    return b;
}

// ── PinConfiguration ──

QJsonObject PinConfiguration::toJson() const {
    QJsonObject obj;
    obj["pin"] = pin;
    if (!label.isEmpty()) obj["label"] = label;
    if (!gpio.isEmpty()) obj["gpio"] = gpio;
    obj["mode"] = mode;
    if (!alternateFunction.isEmpty()) obj["alternateFunction"] = alternateFunction;
    obj["pull"] = pull;
    obj["speed"] = speed;
    obj["outputType"] = outputType;
    obj["initialOutput"] = initialOutput;
    obj["interrupt"] = interrupt;
    if (locked) obj["locked"] = true;
    return obj;
}

PinConfiguration PinConfiguration::fromJson(const QJsonObject& obj) {
    PinConfiguration c;
    c.pin = obj.value("pin").toString();
    c.label = obj.value("label").toString();
    c.gpio = obj.value("gpio").toString();
    c.mode = obj.value("mode").toString("None");
    c.alternateFunction = obj.value("alternateFunction").toString();
    c.pull = obj.value("pull").toString("NoPull");
    c.speed = obj.value("speed").toString("Medium");
    c.outputType = obj.value("outputType").toString("PushPull");
    c.initialOutput = obj.value("initialOutput").toString("Low");
    c.interrupt = obj.value("interrupt").toString("None");
    c.locked = obj.value("locked").toBool(false);
    return c;
}

// ── PeripheralConfiguration ──

QJsonObject PeripheralConfiguration::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["type"] = type;
    obj["enabled"] = enabled;
    QJsonObject pinsObj;
    for (auto it = assignedPins.begin(); it != assignedPins.end(); ++it) {
        pinsObj[it.key()] = it.value();
    }
    obj["assignedPins"] = pinsObj;

    QJsonObject paramsObj;
    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        paramsObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    obj["parameters"] = paramsObj;
    return obj;
}

PeripheralConfiguration PeripheralConfiguration::fromJson(const QJsonObject& obj) {
    PeripheralConfiguration c;
    c.name = obj.value("name").toString();
    c.type = obj.value("type").toString();
    c.enabled = obj.value("enabled").toBool(false);
    QJsonObject pinsObj = obj.value("assignedPins").toObject();
    for (auto it = pinsObj.begin(); it != pinsObj.end(); ++it) {
        c.assignedPins[it.key()] = it.value().toString();
    }
    QJsonObject paramsObj = obj.value("parameters").toObject();
    for (auto it = paramsObj.begin(); it != paramsObj.end(); ++it) {
        c.parameters[it.key()] = it.value().toVariant();
    }
    return c;
}

// ── ClockConfiguration ──

QJsonObject ClockConfiguration::toJson() const {
    QJsonObject obj;
    obj["sysClockMhz"] = sysClockMhz;
    obj["hclkMhz"] = hclkMhz;
    obj["apb1Mhz"] = apb1Mhz;
    obj["apb2Mhz"] = apb2Mhz;
    obj["oscSource"] = oscSource;
    obj["extOscMhz"] = extOscMhz;
    return obj;
}

ClockConfiguration ClockConfiguration::fromJson(const QJsonObject& obj) {
    ClockConfiguration c;
    c.sysClockMhz = obj.value("sysClockMhz").toInt();
    c.hclkMhz = obj.value("hclkMhz").toInt();
    c.apb1Mhz = obj.value("apb1Mhz").toInt();
    c.apb2Mhz = obj.value("apb2Mhz").toInt();
    c.oscSource = obj.value("oscSource").toString("Internal");
    c.extOscMhz = obj.value("extOscMhz").toInt();
    return c;
}

// ── ToolchainConfiguration ──

QJsonObject ToolchainConfiguration::toJson() const {
    QJsonObject obj;
    obj["framework"] = framework;
    obj["toolchain"] = toolchain;
    obj["ide"] = ide;
    obj["library"] = library;
    return obj;
}

ToolchainConfiguration ToolchainConfiguration::fromJson(const QJsonObject& obj) {
    ToolchainConfiguration c;
    c.framework = obj.value("framework").toString("stm32cube");
    c.toolchain = obj.value("toolchain").toString("arm-none-eabi-gcc");
    c.ide = obj.value("ide").toString("STM32CubeIDE");
    c.library = obj.value("library").toString("HAL");
    return c;
}

// ── HardwareConfig ──

void HardwareConfig::clear() {
    targetType = "device";
    vendor.clear();
    family.clear();
    series.clear();
    deviceId.clear();
    boardId.clear();
    architecture.clear();
    core.clear();
    package.clear();
    flashBytes = 0;
    ramBytes = 0;
    pins.clear();
    peripherals.clear();
    clock = ClockConfiguration();
    toolchain = ToolchainConfiguration();
}

QJsonObject HardwareConfig::toJson() const {
    QJsonObject obj;
    QJsonObject target;
    target["type"] = targetType;
    target["vendor"] = vendor;
    target["family"] = family;
    target["series"] = series;
    target["deviceId"] = deviceId;
    if (!boardId.isEmpty()) target["boardId"] = boardId;
    target["architecture"] = architecture;
    target["core"] = core;
    target["package"] = package;
    target["flashBytes"] = static_cast<qint64>(flashBytes);
    target["ramBytes"] = static_cast<qint64>(ramBytes);
    obj["target"] = target;

    QJsonArray pinArr;
    for (const auto& p : pins) pinArr.append(p.toJson());
    obj["pins"] = pinArr;

    QJsonObject perObj;
    for (auto it = peripherals.begin(); it != peripherals.end(); ++it) {
        perObj[it.key()] = it.value().toJson();
    }
    obj["peripherals"] = perObj;

    obj["clock"] = clock.toJson();
    obj["toolchain"] = toolchain.toJson();
    return obj;
}

HardwareConfig HardwareConfig::fromJson(const QJsonObject& obj) {
    HardwareConfig cfg;
    QJsonObject target = obj.value("target").toObject();
    cfg.targetType = target.value("type").toString("device");
    cfg.vendor = target.value("vendor").toString();
    cfg.family = target.value("family").toString();
    cfg.series = target.value("series").toString();
    cfg.deviceId = target.value("deviceId").toString();
    cfg.boardId = target.value("boardId").toString();
    cfg.architecture = target.value("architecture").toString();
    cfg.core = target.value("core").toString();
    cfg.package = target.value("package").toString();
    cfg.flashBytes = static_cast<quint64>(target.value("flashBytes").toVariant().toLongLong());
    cfg.ramBytes = static_cast<quint64>(target.value("ramBytes").toVariant().toLongLong());

    QJsonArray pinArr = obj.value("pins").toArray();
    for (const QJsonValue& v : pinArr) {
        PinConfiguration p = PinConfiguration::fromJson(v.toObject());
        cfg.pins[p.pin] = p;
    }

    QJsonObject perObj = obj.value("peripherals").toObject();
    for (auto it = perObj.begin(); it != perObj.end(); ++it) {
        cfg.peripherals[it.key()] = PeripheralConfiguration::fromJson(it.value().toObject());
    }

    cfg.clock = ClockConfiguration::fromJson(obj.value("clock").toObject());
    cfg.toolchain = ToolchainConfiguration::fromJson(obj.value("toolchain").toObject());
    return cfg;
}

} // namespace Hardware
