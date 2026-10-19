#pragma once

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

class QJsonObject;

namespace Hardware {

/**
 * Sourced hardware capability data (Phase B).
 *
 * Everything the generator needs to know about a device is loaded from a
 * `*_pin_data.json` file shipped next to the device packs. Every value carries
 * the documents it was transcribed from, its license basis and a `verified`
 * flag. A value that is not documented is simply absent (known() == false):
 * consumers must emit an Info diagnostic and omit the configuration instead of
 * guessing.
 */

struct SourcedValue {
    bool known = false;
    QVariant value;
    QStringList sources;
    QString license;
    bool verified = false;

    QString asString() const { return value.toString(); }
    int asInt() const { return value.toInt(); }
};

struct ClockEnableFact {
    QString registerName;   // e.g. "RCC_APB2ENR"
    int bit = -1;           // bit position inside the register
    QStringList sources;
    QString license;
    bool verified = false;

    bool known() const { return bit >= 0 && !registerName.isEmpty(); }
};

struct IrqFact {
    QString name;           // e.g. "USART1" (IRQn_Type prefix)
    int number = -1;
    QString handler;        // e.g. "USART1_IRQHandler"
    QStringList sources;
    QString license;
    bool verified = false;

    bool known() const { return number >= 0; }
};

struct AfFact {
    QString signal;         // e.g. "USART1_TX"
    int af = -1;            // 0..7 (AF0..AF7)
    QStringList sources;
    QString license;
    bool verified = false;

    bool known() const { return af >= 0; }
};

struct PortFacts {
    QString name;               // "GPIOA"
    SourcedValue registerBase;  // "0x48000000"
    ClockEnableFact clock;
    bool afrAvailable = false;  // some F0 ports have no AFR registers
    QStringList afrSources;
    QString afrLicense;
    bool afrVerified = false;
};

struct PeripheralFacts {
    QString name;               // "USART1"
    bool available = false;
    QStringList availableSources;
    QString availableLicense;
    bool availableVerified = false;

    SourcedValue registerBase;  // "0x40013800"
    QString clockBus;           // "APB2"
    ClockEnableFact clock;
    QVector<IrqFact> irqs;

    QStringList allSources() const;
    const IrqFact* primaryIrq() const;
};

struct SystemFacts {
    bool nvicAvailable = false;
    QStringList nvicSources;
    QString nvicLicense;
    bool nvicVerified = false;

    bool syscfgAvailable = false;
    SourcedValue syscfgBase;
    QStringList syscfgExtiRegisters;    // SYSCFG_EXTICR1..4
    ClockEnableFact syscfgClock;
    QStringList syscfgSources;

    bool extiAvailable = false;
    SourcedValue extiBase;
    QVector<IrqFact> extiIrqs;          // EXTI0_1, EXTI2_3, EXTI4_15
    QStringList extiSources;

    bool dmaAvailable = false;
    QString dmaController;              // "DMA1"
    SourcedValue dmaBase;
    int dmaChannels = 0;
    ClockEnableFact dmaClock;
    QVector<IrqFact> dmaIrqs;
    QStringList dmaSources;

    const IrqFact* extiIrqForLine(int line) const;
};

class PinCapabilities {
public:
    bool isValid() const { return !m_device.isEmpty(); }

    QString device() const { return m_device; }
    QString family() const { return m_family; }
    QString package() const { return m_package; }

    QStringList provenanceSources() const { return m_provenanceSources; }
    QString provenanceLicense() const { return m_provenanceLicense; }
    bool provenanceVerified() const { return m_provenanceVerified; }

    bool hasGpioPin(const QString& pin) const { return m_gpioPins.contains(pin, Qt::CaseInsensitive); }
    QStringList gpioPins() const { return m_gpioPins; }

    QVector<AfFact> afFacts(const QString& pin) const { return m_alternateFunctions.value(pin); }
    const AfFact* afFact(const QString& pin, const QString& signal) const;

    const PortFacts* port(const QString& portName) const;
    const PortFacts* portForPin(const QString& pin) const;

    const PeripheralFacts* peripheral(const QString& name) const;
    QStringList peripheralNames() const { return m_peripherals.keys(); }

    const SystemFacts& system() const { return m_system; }

    bool parse(const QJsonObject& root, QString* error = nullptr);

    static bool loadFile(const QString& path, PinCapabilities* out, QString* error = nullptr);

    // "PA9" -> "GPIOA"; empty when the name is not a GPIO pin.
    static QString portNameForPin(const QString& pin);
    // "PA9" -> 9; -1 when the name is not a GPIO pin.
    static int pinIndex(const QString& pin);

private:
    QString m_device;
    QString m_family;
    QString m_package;
    QStringList m_provenanceSources;
    QString m_provenanceLicense;
    bool m_provenanceVerified = false;

    QStringList m_gpioPins;
    QMap<QString, QVector<AfFact>> m_alternateFunctions;
    QMap<QString, PortFacts> m_ports;
    QMap<QString, PeripheralFacts> m_peripherals;
    SystemFacts m_system;
};

/**
 * Finds and caches `*_pin_data.json` capability files. Search paths mirror
 * DeviceDatabase so repository checkouts, build directories and installed
 * layouts all resolve the same data.
 */
class PinCapabilityDatabase {
public:
    static PinCapabilityDatabase& instance();

    // Returns nullptr when no capability file exists for the device. Callers
    // must treat this as "data unknown" and report an Info diagnostic.
    const PinCapabilities* forDevice(const QString& deviceId);

    void clearCache();

    // Candidate data roots (first hit wins), exposed for tests.
    static QStringList candidateRoots();

private:
    PinCapabilityDatabase() = default;

    const PinCapabilities* loadForDevice(const QString& deviceId);
    static QString findDataFile(const QString& deviceId);

    QMap<QString, PinCapabilities> m_cache;   // key: lowercased device id
};

} // namespace Hardware
