#include "PinCapabilities.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace Hardware {

QStringList PeripheralFacts::allSources() const {
    QStringList all = availableSources;
    all << registerBase.sources << clock.sources;
    for (const IrqFact& irq : irqs) all << irq.sources;
    return all;
}

const IrqFact* PeripheralFacts::primaryIrq() const {
    return irqs.isEmpty() ? nullptr : &irqs.first();
}

const IrqFact* SystemFacts::extiIrqForLine(int line) const {
    if (line < 0 || line > 15) return nullptr;
    const int index = line <= 1 ? 0 : (line <= 3 ? 1 : 2);
    return index < extiIrqs.size() ? &extiIrqs.at(index) : nullptr;
}

const AfFact* PinCapabilities::afFact(const QString& pin, const QString& signal) const {
    const auto it = m_alternateFunctions.constFind(pin);
    if (it == m_alternateFunctions.constEnd()) return nullptr;
    for (const AfFact& fact : it.value()) {
        if (fact.signal.compare(signal, Qt::CaseInsensitive) == 0) return &fact;
    }
    return nullptr;
}

const PortFacts* PinCapabilities::port(const QString& portName) const {
    const auto it = m_ports.constFind(portName);
    return it == m_ports.constEnd() ? nullptr : &it.value();
}

const PortFacts* PinCapabilities::portForPin(const QString& pin) const {
    return port(portNameForPin(pin));
}

const PeripheralFacts* PinCapabilities::peripheral(const QString& name) const {
    const auto it = m_peripherals.constFind(name);
    return it == m_peripherals.constEnd() ? nullptr : &it.value();
}

QString PinCapabilities::portNameForPin(const QString& pin) {
    const QString trimmed = pin.trimmed().toUpper();
    if (trimmed.size() < 2 || trimmed.at(0) != QLatin1Char('P')) return QString();
    const QChar portLetter = trimmed.at(1);
    if (portLetter < QLatin1Char('A') || portLetter > QLatin1Char('I')) return QString();
    if (pinIndex(trimmed) < 0) return QString();
    return QStringLiteral("GPIO") + portLetter;
}

int PinCapabilities::pinIndex(const QString& pin) {
    const QString trimmed = pin.trimmed().toUpper();
    if (trimmed.size() < 3 || trimmed.at(0) != QLatin1Char('P')) return -1;
    bool ok = false;
    const int index = trimmed.mid(2).toInt(&ok);
    if (!ok || index < 0 || index > 15) return -1;
    return index;
}

namespace {

SourcedValue parseSourcedValue(const QJsonObject& obj) {
    SourcedValue value;
    if (obj.contains(QStringLiteral("value"))) {
        value.value = obj.value(QStringLiteral("value")).toVariant();
        value.known = true;
    }
    for (const QJsonValue& src : obj.value(QStringLiteral("sources")).toArray()) {
        if (src.isString()) value.sources << src.toString();
    }
    value.license = obj.value(QStringLiteral("license")).toString();
    value.verified = obj.value(QStringLiteral("verified")).toBool(false);
    return value;
}

ClockEnableFact parseClock(const QJsonObject& obj) {
    ClockEnableFact clock;
    clock.registerName = obj.value(QStringLiteral("register")).toString();
    if (obj.contains(QStringLiteral("bit"))) clock.bit = obj.value(QStringLiteral("bit")).toInt(-1);
    for (const QJsonValue& src : obj.value(QStringLiteral("sources")).toArray()) {
        if (src.isString()) clock.sources << src.toString();
    }
    clock.license = obj.value(QStringLiteral("license")).toString();
    clock.verified = obj.value(QStringLiteral("verified")).toBool(false);
    return clock;
}

IrqFact parseIrq(const QJsonObject& obj) {
    IrqFact irq;
    irq.name = obj.value(QStringLiteral("name")).toString();
    irq.number = obj.value(QStringLiteral("number")).toInt(-1);
    irq.handler = obj.value(QStringLiteral("handler")).toString();
    for (const QJsonValue& src : obj.value(QStringLiteral("sources")).toArray()) {
        if (src.isString()) irq.sources << src.toString();
    }
    irq.license = obj.value(QStringLiteral("license")).toString();
    irq.verified = obj.value(QStringLiteral("verified")).toBool(false);
    return irq;
}

} // namespace

bool PinCapabilities::parse(const QJsonObject& root, QString* error) {
    const QJsonObject identity = root.value(QStringLiteral("identity")).toObject();
    m_device = identity.value(QStringLiteral("device")).toString();
    m_family = identity.value(QStringLiteral("family")).toString();
    m_package = identity.value(QStringLiteral("package")).toString();
    if (m_device.isEmpty()) {
        if (error) *error = QStringLiteral("pin capabilities file has no identity.device");
        return false;
    }

    const QJsonObject provenance = root.value(QStringLiteral("provenance")).toObject();
    for (const QJsonValue& src : provenance.value(QStringLiteral("sources")).toArray()) {
        if (src.isString()) m_provenanceSources << src.toString();
    }
    m_provenanceLicense = provenance.value(QStringLiteral("license")).toString();
    m_provenanceVerified = provenance.value(QStringLiteral("verified")).toBool(false);

    for (const QJsonValue& pin : root.value(QStringLiteral("gpioPins")).toArray()) {
        if (pin.isString()) m_gpioPins << pin.toString();
    }

    const QJsonObject ports = root.value(QStringLiteral("ports")).toObject();
    for (auto it = ports.constBegin(); it != ports.constEnd(); ++it) {
        const QJsonObject portObj = it.value().toObject();
        PortFacts port;
        port.name = it.key();
        port.registerBase = parseSourcedValue(portObj.value(QStringLiteral("registerBase")).toObject());
        port.clock = parseClock(portObj.value(QStringLiteral("clockEnable")).toObject());
        const QJsonObject afr = portObj.value(QStringLiteral("afrAvailable")).toObject();
        port.afrAvailable = afr.value(QStringLiteral("value")).toBool(false);
        for (const QJsonValue& src : afr.value(QStringLiteral("sources")).toArray()) {
            if (src.isString()) port.afrSources << src.toString();
        }
        port.afrLicense = afr.value(QStringLiteral("license")).toString();
        port.afrVerified = afr.value(QStringLiteral("verified")).toBool(false);
        m_ports.insert(port.name, port);
    }

    const QJsonObject peripherals = root.value(QStringLiteral("peripherals")).toObject();
    for (auto it = peripherals.constBegin(); it != peripherals.constEnd(); ++it) {
        const QJsonObject pObj = it.value().toObject();
        PeripheralFacts facts;
        facts.name = it.key();
        const QJsonObject available = pObj.value(QStringLiteral("available")).toObject();
        facts.available = available.value(QStringLiteral("value")).toBool(false);
        for (const QJsonValue& src : available.value(QStringLiteral("sources")).toArray()) {
            if (src.isString()) facts.availableSources << src.toString();
        }
        facts.availableLicense = available.value(QStringLiteral("license")).toString();
        facts.availableVerified = available.value(QStringLiteral("verified")).toBool(false);

        facts.registerBase = parseSourcedValue(pObj.value(QStringLiteral("registerBase")).toObject());
        facts.clockBus = pObj.value(QStringLiteral("clockBus")).toObject().value(QStringLiteral("value")).toString();
        facts.clock = parseClock(pObj.value(QStringLiteral("clockEnable")).toObject());
        for (const QJsonValue& irq : pObj.value(QStringLiteral("irqs")).toArray()) {
            facts.irqs.append(parseIrq(irq.toObject()));
        }
        m_peripherals.insert(facts.name, facts);
    }

    const QJsonObject system = root.value(QStringLiteral("system")).toObject();

    const QJsonObject nvic = system.value(QStringLiteral("nvic")).toObject();
    const QJsonObject nvicAvailable = nvic.value(QStringLiteral("available")).toObject();
    m_system.nvicAvailable = nvicAvailable.value(QStringLiteral("value")).toBool(false);
    for (const QJsonValue& src : nvicAvailable.value(QStringLiteral("sources")).toArray()) {
        if (src.isString()) m_system.nvicSources << src.toString();
    }
    m_system.nvicLicense = nvicAvailable.value(QStringLiteral("license")).toString();
    m_system.nvicVerified = nvicAvailable.value(QStringLiteral("verified")).toBool(false);

    const QJsonObject syscfg = system.value(QStringLiteral("syscfg")).toObject();
    m_system.syscfgAvailable = syscfg.value(QStringLiteral("available")).toObject().value(QStringLiteral("value")).toBool(false);
    m_system.syscfgBase = parseSourcedValue(syscfg.value(QStringLiteral("registerBase")).toObject());
    m_system.syscfgClock = parseClock(syscfg.value(QStringLiteral("clockEnable")).toObject());
    for (const QJsonValue& reg : syscfg.value(QStringLiteral("extiConfigRegisters")).toObject().value(QStringLiteral("value")).toArray()) {
        if (reg.isString()) m_system.syscfgExtiRegisters << reg.toString();
    }
    m_system.syscfgSources << m_system.syscfgBase.sources;

    const QJsonObject exti = system.value(QStringLiteral("exti")).toObject();
    m_system.extiAvailable = exti.value(QStringLiteral("available")).toObject().value(QStringLiteral("value")).toBool(false);
    m_system.extiBase = parseSourcedValue(exti.value(QStringLiteral("registerBase")).toObject());
    for (const QJsonValue& irq : exti.value(QStringLiteral("irqs")).toArray()) {
        m_system.extiIrqs.append(parseIrq(irq.toObject()));
    }
    m_system.extiSources << m_system.extiBase.sources;

    const QJsonObject dma = system.value(QStringLiteral("dma")).toObject();
    m_system.dmaAvailable = dma.value(QStringLiteral("available")).toObject().value(QStringLiteral("value")).toBool(false);
    m_system.dmaController = dma.value(QStringLiteral("controller")).toObject().value(QStringLiteral("value")).toString();
    m_system.dmaBase = parseSourcedValue(dma.value(QStringLiteral("registerBase")).toObject());
    m_system.dmaChannels = dma.value(QStringLiteral("channels")).toObject().value(QStringLiteral("value")).toInt(0);
    m_system.dmaClock = parseClock(dma.value(QStringLiteral("clockEnable")).toObject());
    for (const QJsonValue& irq : dma.value(QStringLiteral("irqs")).toArray()) {
        m_system.dmaIrqs.append(parseIrq(irq.toObject()));
    }
    m_system.dmaSources << m_system.dmaBase.sources << m_system.dmaClock.sources;

    const QJsonObject afs = root.value(QStringLiteral("alternateFunctions")).toObject();
    for (auto it = afs.constBegin(); it != afs.constEnd(); ++it) {
        QVector<AfFact> facts;
        for (const QJsonValue& entry : it.value().toArray()) {
            const QJsonObject af = entry.toObject();
            AfFact fact;
            fact.signal = af.value(QStringLiteral("signal")).toString();
            fact.af = af.value(QStringLiteral("af")).toInt(-1);
            for (const QJsonValue& src : af.value(QStringLiteral("sources")).toArray()) {
                if (src.isString()) fact.sources << src.toString();
            }
            fact.license = af.value(QStringLiteral("license")).toString();
            fact.verified = af.value(QStringLiteral("verified")).toBool(false);
            if (!fact.signal.isEmpty()) facts.append(fact);
        }
        m_alternateFunctions.insert(it.key(), facts);
    }

    return true;
}

bool PinCapabilities::loadFile(const QString& path, PinCapabilities* out, QString* error) {
    if (!out) return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("cannot open %1").arg(path);
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) *error = QStringLiteral("%1: %2").arg(path, parseError.errorString());
        return false;
    }
    return out->parse(doc.object(), error);
}

PinCapabilityDatabase& PinCapabilityDatabase::instance() {
    static PinCapabilityDatabase db;
    return db;
}

QStringList PinCapabilityDatabase::candidateRoots() {
    QStringList roots;
    roots << QDir::current().filePath(QStringLiteral("data/hardware_packs"));
    roots << QDir::current().filePath(QStringLiteral("../data/hardware_packs"));
    roots << QDir::current().filePath(QStringLiteral("../../data/hardware_packs"));
    roots << QCoreApplication::applicationDirPath() + QStringLiteral("/data/hardware_packs");
    roots << QCoreApplication::applicationDirPath() + QStringLiteral("/../data/hardware_packs");
    roots << QCoreApplication::applicationDirPath() + QStringLiteral("/../../data/hardware_packs");
    roots << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/hardware_packs");
    return roots;
}

QString PinCapabilityDatabase::findDataFile(const QString& deviceId) {
    const QString fileName = deviceId.trimmed().toLower() + QStringLiteral("_pin_data.json");
    for (const QString& root : candidateRoots()) {
        QDir rootDir(root);
        if (!rootDir.exists()) continue;
        const QString direct = rootDir.filePath(fileName);
        if (QFile::exists(direct)) return direct;
        const QStringList subdirs = rootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& sub : subdirs) {
            const QString candidate = rootDir.filePath(sub + QLatin1Char('/') + fileName);
            if (QFile::exists(candidate)) return candidate;
        }
    }
    return QString();
}

const PinCapabilities* PinCapabilityDatabase::loadForDevice(const QString& deviceId) {
    const QString key = deviceId.trimmed().toLower();
    if (key.isEmpty()) return nullptr;

    const QString path = findDataFile(deviceId);
    if (path.isEmpty()) return nullptr;

    PinCapabilities caps;
    QString error;
    if (!PinCapabilities::loadFile(path, &caps, &error)) return nullptr;

    m_cache.insert(key, caps);
    return &m_cache[key];
}

const PinCapabilities* PinCapabilityDatabase::forDevice(const QString& deviceId) {
    const QString key = deviceId.trimmed().toLower();
    if (key.isEmpty()) return nullptr;
    const auto it = m_cache.constFind(key);
    if (it != m_cache.constEnd()) return &it.value();
    return loadForDevice(deviceId);
}

void PinCapabilityDatabase::clearCache() {
    m_cache.clear();
}

} // namespace Hardware
