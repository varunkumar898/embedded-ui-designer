/**
 * Phase B acceptance suite — PeripheralPlan, sourced capability data and the
 * "Pins" validator.
 *
 * Covers:
 *  - Loading src-independent capability data (STM32F030R8) through the same
 *    search paths the app uses.
 *  - A representative slice of transcribed values checked against their cited
 *    documents (DS9773 Rev 5, RM0360 Rev 5, CMSIS stm32f030x8.h).
 *  - Schema rules: every fact entry carries sources + license + verified.
 *  - PeripheralPlan resolution from a HardwareConfig (probe A shape).
 *  - PinsValidator: valid configs stay quiet; pins outside the verified data
 *    are Info (never guessed, never a crash); undocumented AF numbers are
 *    Info; misplaced signals are Warnings; AFR-less ports are Info; conflicts
 *    are Errors; EXTI lines resolve; probe B (I2C1 on PB10/PB11) is flagged.
 *  - DiagnosticsService integration via the registered "Pins" source.
 */
#include <QTest>
#include <QObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <utility>

#include "hardware/HardwareModel.h"
#include "hardware/PinCapabilities.h"
#include "hardware/PeripheralPlan.h"
#include "hardware/PinsValidator.h"
#include "hardware/DeviceDatabase.h"
#include "diagnostics/DiagnosticsService.h"
#include "diagnostics/DiagnosticsSources.h"
#include "project/Project.h"
#include "canvas/CanvasScene.h"

using namespace Hardware;

namespace {

QString capabilityFile()
{
    for (const QString& root : PinCapabilityDatabase::candidateRoots()) {
        const QString path = QDir(root).filePath("stmicroelectronics/stm32f030r8_pin_data.json");
        if (QFile::exists(path)) return path;
    }
    return QString();
}

bool hasSeverity(const QVector<PinIssue>& issues, PinIssue::Severity severity)
{
    for (const PinIssue& issue : issues) {
        if (issue.severity == severity) return true;
    }
    return false;
}

QStringList messages(const QVector<PinIssue>& issues)
{
    QStringList out;
    for (const PinIssue& issue : issues) out << issue.message;
    return out;
}

// recursive schema walk: any object that holds a fact value must carry
// sources/license/verified
void collectFactProblems(const QJsonValue& node, QString* pathOut, QStringList* problems)
{
    if (node.isArray()) {
        const QJsonArray array = node.toArray();
        for (int i = 0; i < array.size(); ++i) {
            collectFactProblems(array.at(i), pathOut, problems);
        }
        return;
    }
    if (!node.isObject()) return;

    const QJsonObject obj = node.toObject();
    const bool isFact = obj.contains("value") || obj.contains("af")
        || obj.contains("number") || obj.contains("bit");
    if (isFact) {
        const QStringList sourcesKeys = { "sources" };
        bool hasSources = false;
        for (const QJsonValue& src : obj.value("sources").toArray()) {
            if (src.isString() && !src.toString().trimmed().isEmpty()) {
                hasSources = true;
                break;
            }
        }
        if (!hasSources) problems->append(*pathOut + ": fact without sources");
        if (obj.value("license").toString().trimmed().isEmpty()) {
            problems->append(*pathOut + ": fact without license");
        }
        if (!obj.value("verified").toBool(false)) {
            problems->append(*pathOut + ": fact not marked verified");
        }
    }

    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const QString saved = *pathOut;
        *pathOut += QLatin1Char('/') + it.key();
        collectFactProblems(it.value(), pathOut, problems);
        *pathOut = saved;
    }
}

HardwareConfig probeConfig()
{
    HardwareConfig cfg;
    cfg.deviceId = "STM32F030R8";
    cfg.family = "STM32";

    PinConfiguration led;
    led.pin = "PA5";
    led.gpio = "PA5";
    led.mode = "GPIO_Output";
    led.initialOutput = "High";
    cfg.pins.insert("PA5", led);

    PinConfiguration tx;
    tx.pin = "PA9";
    tx.gpio = "PA9";
    tx.mode = "AlternateFunction";
    tx.alternateFunction = "USART1_TX";
    tx.pull = "PullUp";
    tx.speed = "VeryHigh";
    cfg.pins.insert("PA9", tx);

    PinConfiguration rx;
    rx.pin = "PA10";
    rx.gpio = "PA10";
    rx.mode = "AlternateFunction";
    rx.alternateFunction = "USART1_RX";
    rx.pull = "PullUp";
    rx.speed = "VeryHigh";
    cfg.pins.insert("PA10", rx);

    PeripheralConfiguration usart;
    usart.name = "USART1";
    usart.type = "USART";
    usart.enabled = true;
    usart.assignedPins.insert("TX", "PA9");
    usart.assignedPins.insert("RX", "PA10");
    usart.parameters.insert("baudRate", 230400);
    cfg.peripherals.insert("USART1", usart);

    return cfg;
}

} // namespace

class TestPinsValidator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void testDataFileLoads();
    void testSchemaEveryFactIsSourced();
    void testAlternateFunctionFactsMatchCitedTables();
    void testPeripheralFactsMatchCitedDocuments();
    void testPortAndSystemFactsMatchCitedDocuments();

    void testPlanResolvesProbeConfig();
    void testPlanKeepsUnknownsUnknown();

    void testValidatorValidConfigStaysQuiet();
    void testValidatorEmptyConfigStaysQuiet();
    void testValidatorUnknownPinIsInfo();
    void testValidatorPinOutsideDataIsInfo();
    void testValidatorProbeBMisplacedI2cSignals();
    void testValidatorUndocumentedAfIsInfo();
    void testValidatorSignalOnWrongPinIsWarning();
    void testValidatorAfrLessPortIsInfo();
    void testValidatorConflictIsError();
    void testValidatorInterruptResolvesExtiLine();
    void testValidatorWithoutDataIsInfo();
    void testDeviceDatabaseUnaffectedByPinDataFile();

    void testDiagnosticsPinsSourceRegistered();

private:
    const PinCapabilities* m_caps = nullptr;
};

void TestPinsValidator::initTestCase()
{
    Hardware::DeviceDatabase::instance().initialize();
    m_caps = PinCapabilityDatabase::instance().forDevice("STM32F030R8");
    QVERIFY2(m_caps != nullptr, qPrintable(QStringLiteral("pin capability data not found; data file: %1").arg(capabilityFile())));
}

void TestPinsValidator::testDataFileLoads()
{
    QCOMPARE(m_caps->device(), QString("STM32F030R8"));
    QCOMPARE(m_caps->family(), QString("STM32F0"));
    QCOMPARE(m_caps->gpioPins().size(), 35);
    QCOMPARE(m_caps->peripheralNames().size(), 14);
    QVERIFY(m_caps->provenanceVerified());
    QVERIFY(!m_caps->provenanceLicense().isEmpty());
    QVERIFY(m_caps->provenanceSources().size() >= 3);
}

void TestPinsValidator::testSchemaEveryFactIsSourced()
{
    const QString path = capabilityFile();
    QVERIFY2(!path.isEmpty(), "capability data file not found");

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);
    QVERIFY(doc.isObject());

    const QJsonObject root = doc.object();
    QCOMPARE(root.value("kind").toString(), QString("pinCapabilities"));

    QStringList problems;
    QString pathPrefix;
    collectFactProblems(root, &pathPrefix, &problems);
    QVERIFY2(problems.isEmpty(), qPrintable(problems.join(QStringLiteral("; "))));
}

void TestPinsValidator::testAlternateFunctionFactsMatchCitedTables()
{
    struct ExpectedAf { const char* pin; const char* signal; int af; const char* table; };
    const ExpectedAf expected[] = {
        // DS9773 Rev 5, Table 12 (GPIOA_AFR)
        { "PA9",  "USART1_TX",  1, "Table 12" },
        { "PA10", "USART1_RX",  1, "Table 12" },
        { "PA2",  "USART2_TX",  1, "Table 12" },
        { "PA3",  "USART2_RX",  1, "Table 12" },
        { "PA5",  "SPI1_SCK",   0, "Table 12" },
        { "PA6",  "SPI1_MISO",  0, "Table 12" },
        { "PA7",  "SPI1_MOSI",  0, "Table 12" },
        { "PA4",  "SPI1_NSS",   0, "Table 12" },
        { "PA15", "SPI1_NSS",   0, "Table 12" },
        // DS9773 Rev 5, Table 13 (GPIOB_AFR)
        { "PB6",  "USART1_TX",  0, "Table 13" },
        { "PB7",  "USART1_RX",  0, "Table 13" },
        { "PB3",  "SPI1_SCK",   0, "Table 13" },
        { "PB4",  "SPI1_MISO",  0, "Table 13" },
        { "PB5",  "SPI1_MOSI",  0, "Table 13" },
        { "PB12", "SPI2_NSS",   0, "Table 13" },
        { "PB13", "SPI2_SCK",   0, "Table 13" },
        { "PB14", "SPI2_MISO",  0, "Table 13" },
        { "PB15", "SPI2_MOSI",  0, "Table 13" },
        { "PB6",  "I2C1_SCL",   1, "Table 13" },
        { "PB7",  "I2C1_SDA",   1, "Table 13" },
        { "PB8",  "I2C1_SCL",   1, "Table 13" },
        { "PB9",  "I2C1_SDA",   1, "Table 13" },
        { "PB10", "I2C2_SCL",   1, "Table 13" },
        { "PB11", "I2C2_SDA",   1, "Table 13" },
    };

    for (const ExpectedAf& exp : expected) {
        const AfFact* fact = m_caps->afFact(exp.pin, exp.signal);
        QVERIFY2(fact != nullptr, qPrintable(QStringLiteral("missing AF fact for %1/%2").arg(exp.pin, exp.signal)));
        QCOMPARE(fact->af, exp.af);
        QVERIFY(fact->verified);
        QVERIFY(!fact->license.isEmpty());
        bool citesDs9773 = false;
        for (const QString& source : fact->sources) {
            if (source.contains("DS9773 Rev 5") && source.contains(exp.table)) citesDs9773 = true;
        }
        QVERIFY2(citesDs9773, qPrintable(QStringLiteral("%1/%2 does not cite DS9773 Rev 5 %3").arg(exp.pin, exp.signal, exp.table)));
    }

    // x4/x6-only or xC-only entries must NOT be present for STM32F030R8.
    QVERIFY(m_caps->afFact("PA14", "USART1_TX") == nullptr);   // x4/x6 only (Table 12)
    QVERIFY(m_caps->afFact("PA9", "I2C1_SCL") == nullptr);     // xC + x4/x6 only (Table 12)
    QVERIFY(m_caps->afFact("PB13", "USART3_CTS") == nullptr);  // xC only (Table 13)
}

void TestPinsValidator::testPeripheralFactsMatchCitedDocuments()
{
    struct ExpectedPeripheral {
        const char* name;
        const char* base;
        const char* bus;
        const char* clockRegister;
        int clockBit;
        int irq;
    };
    const ExpectedPeripheral expected[] = {
        { "USART1", "0x40013800", "APB2", "RCC_APB2ENR", 14, 27 },
        { "USART2", "0x40004400", "APB1", "RCC_APB1ENR", 17, 28 },
        { "SPI1",   "0x40013000", "APB2", "RCC_APB2ENR", 12, 25 },
        { "SPI2",   "0x40003800", "APB1", "RCC_APB1ENR", 14, 26 },
        { "I2C1",   "0x40005400", "APB1", "RCC_APB1ENR", 21, 23 },
        { "I2C2",   "0x40005800", "APB1", "RCC_APB1ENR", 22, 24 },
        { "ADC1",   "0x40012400", "APB2", "RCC_APB2ENR",  9, 12 },
        { "TIM1",   "0x40012C00", "APB2", "RCC_APB2ENR", 11, 13 },
        { "TIM3",   "0x40000400", "APB1", "RCC_APB1ENR",  1, 16 },
        { "TIM6",   "0x40001000", "APB1", "RCC_APB1ENR",  4, 17 },
        { "TIM14",  "0x40002000", "APB1", "RCC_APB1ENR",  8, 19 },
        { "TIM15",  "0x40014000", "APB2", "RCC_APB2ENR", 16, 20 },
        { "TIM16",  "0x40014400", "APB2", "RCC_APB2ENR", 17, 21 },
        { "TIM17",  "0x40014800", "APB2", "RCC_APB2ENR", 18, 22 },
    };

    for (const ExpectedPeripheral& exp : expected) {
        const PeripheralFacts* facts = m_caps->peripheral(exp.name);
        QVERIFY2(facts != nullptr, qPrintable(QStringLiteral("missing peripheral %1").arg(exp.name)));
        QVERIFY(facts->available);
        QVERIFY(facts->availableVerified);
        QCOMPARE(facts->registerBase.asString(), QString(exp.base));
        QCOMPARE(facts->clockBus, QString(exp.bus));
        QCOMPARE(facts->clock.registerName, QString(exp.clockRegister));
        QCOMPARE(facts->clock.bit, exp.clockBit);
        QVERIFY(facts->clock.verified);
        QVERIFY(facts->registerBase.verified);
        QVERIFY2(facts->registerBase.sources.join(";").contains("stm32f030x8.h"),
                 qPrintable(QStringLiteral("%1 base does not cite the CMSIS header").arg(exp.name)));
        QVERIFY(facts->primaryIrq() != nullptr);
        QCOMPARE(facts->primaryIrq()->number, exp.irq);
        QVERIFY(facts->primaryIrq()->verified);
        // CMSIS vector names are not always "<PERIPH>_IRQHandler": TIM1's
        // break/update/trigger/com trigger vector is TIM1_BRK_UP_TRG_COM
        // (RM0360 Rev 5 Table 31, position 13; stm32f030x8.h IRQn_Type).
        const QString expectedHandler =
            QString::compare(exp.name, QStringLiteral("TIM1")) == 0
                ? QStringLiteral("TIM1_BRK_UP_TRG_COM_IRQHandler")
                : QString(exp.name) + QStringLiteral("_IRQHandler");
        QCOMPARE(facts->primaryIrq()->handler, expectedHandler);
    }

    // TIM1 carries both of its vector-table interrupts.
    const PeripheralFacts* tim1 = m_caps->peripheral("TIM1");
    QCOMPARE(tim1->irqs.size(), 2);
    QCOMPARE(tim1->irqs.at(1).name, QString("TIM1_CC"));
    QCOMPARE(tim1->irqs.at(1).number, 14);
}

void TestPinsValidator::testPortAndSystemFactsMatchCitedDocuments()
{
    const PortFacts* gpioA = m_caps->port("GPIOA");
    QVERIFY(gpioA != nullptr);
    QCOMPARE(gpioA->registerBase.asString(), QString("0x48000000"));
    QCOMPARE(gpioA->clock.registerName, QString("RCC_AHBENR"));
    QCOMPARE(gpioA->clock.bit, 17);
    QVERIFY(gpioA->afrAvailable);

    const PortFacts* gpioB = m_caps->port("GPIOB");
    QVERIFY(gpioB != nullptr);
    QCOMPARE(gpioB->registerBase.asString(), QString("0x48000400"));
    QCOMPARE(gpioB->clock.bit, 18);
    QVERIFY(gpioB->afrAvailable);

    const PortFacts* gpioC = m_caps->port("GPIOC");
    QVERIFY(gpioC != nullptr);
    QCOMPARE(gpioC->registerBase.asString(), QString("0x48000800"));
    QCOMPARE(gpioC->clock.bit, 19);
    QVERIFY(!gpioC->afrAvailable);  // DS9773 Rev 5 Table 14, note 2

    const SystemFacts& system = m_caps->system();
    QVERIFY(system.nvicAvailable);
    QVERIFY(!system.nvicSources.isEmpty());

    QVERIFY(system.syscfgAvailable);
    QCOMPARE(system.syscfgBase.asString(), QString("0x40010000"));
    QCOMPARE(system.syscfgExtiRegisters,
             QStringList({ "SYSCFG_EXTICR1", "SYSCFG_EXTICR2", "SYSCFG_EXTICR3", "SYSCFG_EXTICR4" }));
    QCOMPARE(system.syscfgClock.registerName, QString("RCC_APB2ENR"));
    QCOMPARE(system.syscfgClock.bit, 0);

    QVERIFY(system.extiAvailable);
    QCOMPARE(system.extiBase.asString(), QString("0x40010400"));
    QCOMPARE(system.extiIrqs.size(), 3);
    QCOMPARE(system.extiIrqs.at(0).number, 5);
    QCOMPARE(system.extiIrqs.at(1).number, 6);
    QCOMPARE(system.extiIrqs.at(2).number, 7);
    QVERIFY(system.extiIrqForLine(0) != nullptr);
    QVERIFY(system.extiIrqForLine(9) != nullptr);
    QCOMPARE(system.extiIrqForLine(9)->number, 7);
    QVERIFY(system.extiIrqForLine(16) == nullptr);

    QVERIFY(system.dmaAvailable);
    QCOMPARE(system.dmaController, QString("DMA1"));
    QCOMPARE(system.dmaBase.asString(), QString("0x40020000"));
    QCOMPARE(system.dmaChannels, 5);
    QCOMPARE(system.dmaClock.registerName, QString("RCC_AHBENR"));
    QCOMPARE(system.dmaClock.bit, 0);
    QCOMPARE(system.dmaIrqs.size(), 3);
    QCOMPARE(system.dmaIrqs.at(0).number, 9);
    QCOMPARE(system.dmaIrqs.at(1).number, 10);
    QCOMPARE(system.dmaIrqs.at(2).number, 11);
}

void TestPinsValidator::testPlanResolvesProbeConfig()
{
    const PeripheralPlan plan = PeripheralPlan::fromConfig(probeConfig(), m_caps);
    QVERIFY(!plan.isEmpty());
    QCOMPARE(plan.deviceName(), QString("STM32F030R8"));
    QCOMPARE(plan.pins.size(), 3);
    QCOMPARE(plan.peripherals.size(), 1);

    const PlannedPeripheral& usart = plan.peripherals.first();
    QCOMPARE(usart.name, QString("USART1"));
    QVERIFY(usart.factsKnown);
    QVERIFY(usart.available);
    QCOMPARE(usart.registerBase, QString("0x40013800"));
    QCOMPARE(usart.clockBus, QString("APB2"));
    QCOMPARE(usart.clockRegister, QString("RCC_APB2ENR"));
    QCOMPARE(usart.clockBit, 14);
    QCOMPARE(usart.irqNumber, 27);
    QCOMPARE(usart.irqName, QString("USART1"));
    QCOMPARE(usart.parameters.value("baudRate").toInt(), 230400);

    QCOMPARE(usart.signalUses.size(), 2);
    for (const PlannedSignalUse& use : usart.signalUses) {
        QCOMPARE(use.pinKnown, true);
        QCOMPARE(use.afKnown, true);
        QCOMPARE(use.afNumber, 1);
        QVERIFY(!use.afSources.isEmpty());
        if (use.signal == "TX") QCOMPARE(use.fullSignal, QString("USART1_TX"));
        if (use.signal == "RX") QCOMPARE(use.fullSignal, QString("USART1_RX"));
    }

    bool foundLed = false;
    for (const PlannedPin& pin : plan.pins) {
        if (pin.pin == "PA5") {
            foundLed = true;
            QCOMPARE(pin.mode, QString("GPIO_Output"));
            QVERIFY(pin.pinKnown);
        }
    }
    QVERIFY(foundLed);
}

void TestPinsValidator::testPlanKeepsUnknownsUnknown()
{
    HardwareConfig cfg;
    cfg.deviceId = "STM32F030R8";

    PinConfiguration pin;
    pin.pin = "PA0";
    pin.mode = "AlternateFunction";
    pin.alternateFunction = "TIM2_CH1";   // named in the device pack, not in DS9773 for F030x8
    cfg.pins.insert("PA0", pin);

    PeripheralConfiguration usart3;
    usart3.name = "USART3";
    usart3.type = "USART";
    usart3.enabled = true;
    usart3.assignedPins.insert("TX", "PC4");
    cfg.peripherals.insert("USART3", usart3);

    const PeripheralPlan plan = PeripheralPlan::fromConfig(cfg, m_caps);
    QCOMPARE(plan.pins.size(), 1);
    QCOMPARE(plan.pins.first().afKnown, false);
    QCOMPARE(plan.pins.first().afNumber, -1);

    QCOMPARE(plan.peripherals.size(), 1);
    QVERIFY(!plan.peripherals.first().factsKnown);
    QCOMPARE(plan.peripherals.first().available, false);
    QCOMPARE(plan.peripherals.first().registerBase, QString());
    QCOMPARE(plan.peripherals.first().irqNumber, -1);
}

void TestPinsValidator::testValidatorValidConfigStaysQuiet()
{
    const QVector<PinIssue> issues = PinsValidator::validate(probeConfig(), m_caps);
    QVERIFY2(issues.isEmpty(), qPrintable(messages(issues).join(QStringLiteral(" | "))));
}

void TestPinsValidator::testValidatorEmptyConfigStaysQuiet()
{
    HardwareConfig cfg;
    cfg.deviceId = "STM32F030R8";
    QVERIFY(PinsValidator::validate(cfg, m_caps).isEmpty());
    QVERIFY(PinsValidator::validate(cfg, nullptr).isEmpty());
}

void TestPinsValidator::testValidatorUnknownPinIsInfo()
{
    HardwareConfig cfg = probeConfig();
    PinConfiguration ghost;
    ghost.pin = "PX1";
    ghost.mode = "GPIO_Output";
    cfg.pins.insert("PX1", ghost);

    const QVector<PinIssue> issues = PinsValidator::validate(cfg, m_caps);
    QVERIFY2(!hasSeverity(issues, PinIssue::Severity::Error),
             qPrintable(messages(issues).join(QStringLiteral(" | "))));
    QCOMPARE(issues.size(), 1);
    QCOMPARE(issues.first().severity, PinIssue::Severity::Info);
    QVERIFY(issues.first().message.contains("No verified pin data"));
    QVERIFY(issues.first().message.contains("PX1"));
}

void TestPinsValidator::testValidatorPinOutsideDataIsInfo()
{
    // PD2 is a real F0 pin but is not part of the verified data set. Assigning
    // it must produce an Info diagnostic - never a guess, never a crash.
    HardwareConfig cfg;
    cfg.deviceId = "STM32F030R8";

    PinConfiguration pin;
    pin.pin = "PD2";
    pin.mode = "GPIO_Output";
    cfg.pins.insert("PD2", pin);

    const QVector<PinIssue> issues = PinsValidator::validate(cfg, m_caps);
    QVERIFY2(!hasSeverity(issues, PinIssue::Severity::Error),
             qPrintable(messages(issues).join(QStringLiteral(" | "))));
    QCOMPARE(issues.size(), 1);
    QCOMPARE(issues.first().severity, PinIssue::Severity::Info);
    QVERIFY(issues.first().message.contains("No verified pin data"));
    QVERIFY(issues.first().message.contains("PD2"));

    const PeripheralPlan plan = PeripheralPlan::fromConfig(cfg, m_caps);
    QCOMPARE(plan.pins.size(), 1);
    QCOMPARE(plan.pins.first().pinKnown, false);
    QCOMPARE(plan.pins.first().afNumber, -1);
}

void TestPinsValidator::testValidatorProbeBMisplacedI2cSignals()
{
    // Probe B assigns I2C1 to PB10/PB11. On STM32F030R8 those pins carry
    // I2C2_SCL/I2C2_SDA (AF1); I2C1 is on PB6/PB7 and PB8/PB9 (DS9773 Rev 5
    // Table 13). The validator must flag the misplaced signals as Warnings -
    // exactly once per signal even though the config names them both as pins
    // and as peripheral pins.
    HardwareConfig cfg;
    cfg.deviceId = "STM32F030R8";

    const std::pair<const char*, const char*> afPins[] = {
        { "PB6",  "USART1_TX" },
        { "PB7",  "USART1_RX" },
        { "PB10", "I2C1_SCL"  },
        { "PB11", "I2C1_SDA"  },
    };
    for (const auto& entry : afPins) {
        PinConfiguration pin;
        pin.pin = entry.first;
        pin.mode = "AlternateFunction";
        pin.alternateFunction = entry.second;
        cfg.pins.insert(entry.first, pin);
    }

    PinConfiguration led;
    led.pin = "PC13";
    led.mode = "GPIO_Output";
    cfg.pins.insert("PC13", led);

    PeripheralConfiguration usart;
    usart.name = "USART1";
    usart.type = "USART";
    usart.enabled = true;
    usart.assignedPins.insert("TX", "PB6");
    usart.assignedPins.insert("RX", "PB7");
    cfg.peripherals.insert("USART1", usart);

    PeripheralConfiguration i2c;
    i2c.name = "I2C1";
    i2c.type = "I2C";
    i2c.enabled = true;
    i2c.assignedPins.insert("SCL", "PB10");
    i2c.assignedPins.insert("SDA", "PB11");
    cfg.peripherals.insert("I2C1", i2c);

    const QVector<PinIssue> issues = PinsValidator::validate(cfg, m_caps);
    QVERIFY2(!hasSeverity(issues, PinIssue::Severity::Error),
             qPrintable(messages(issues).join(QStringLiteral(" | "))));
    QCOMPARE(issues.size(), 2);
    for (const PinIssue& issue : issues) {
        QCOMPARE(issue.severity, PinIssue::Severity::Warning);
        QVERIFY(issue.message.contains("not documented"));
    }
    const QString joined = messages(issues).join(QStringLiteral(" | "));
    QVERIFY(joined.contains("PB10"));
    QVERIFY(joined.contains("I2C1_SCL"));
    QVERIFY(joined.contains("PB11"));
    QVERIFY(joined.contains("I2C1_SDA"));
}

void TestPinsValidator::testValidatorUndocumentedAfIsInfo()
{
    HardwareConfig cfg = probeConfig();
    PinConfiguration pin;
    pin.pin = "PA0";
    pin.mode = "AlternateFunction";
    pin.alternateFunction = "TIM2_CH1";
    cfg.pins.insert("PA0", pin);

    const QVector<PinIssue> issues = PinsValidator::validate(cfg, m_caps);
    QVERIFY(!hasSeverity(issues, PinIssue::Severity::Error));
    bool infoAboutAf = false;
    for (const PinIssue& issue : issues) {
        if (issue.severity == PinIssue::Severity::Info
            && issue.message.contains("TIM2_CH1") && issue.message.contains("not documented")) {
            infoAboutAf = true;
        }
    }
    QVERIFY2(infoAboutAf, qPrintable(messages(issues).join(QStringLiteral(" | "))));
}

void TestPinsValidator::testValidatorSignalOnWrongPinIsWarning()
{
    HardwareConfig cfg = probeConfig();
    PinConfiguration pin;
    pin.pin = "PA9";
    pin.mode = "AlternateFunction";
    pin.alternateFunction = "I2C1_SCL";   // documented on PB6/PB8, xC-only on PA9
    cfg.pins.insert("PA9", pin);
    cfg.peripherals.clear();

    const QVector<PinIssue> issues = PinsValidator::validate(cfg, m_caps);
    bool warning = false;
    for (const PinIssue& issue : issues) {
        if (issue.severity == PinIssue::Severity::Warning
            && issue.message.contains("I2C1_SCL") && issue.message.contains("not documented")) {
            warning = true;
        }
    }
    QVERIFY2(warning, qPrintable(messages(issues).join(QStringLiteral(" | "))));
}

void TestPinsValidator::testValidatorAfrLessPortIsInfo()
{
    HardwareConfig cfg;
    cfg.deviceId = "STM32F030R8";
    PinConfiguration pin;
    pin.pin = "PC13";
    pin.mode = "AlternateFunction";
    pin.alternateFunction = "RTC_TAMP1";
    cfg.pins.insert("PC13", pin);

    const QVector<PinIssue> issues = PinsValidator::validate(cfg, m_caps);
    QVERIFY(!hasSeverity(issues, PinIssue::Severity::Error));
    bool info = false;
    for (const PinIssue& issue : issues) {
        if (issue.severity == PinIssue::Severity::Info && issue.message.contains("AFR")) info = true;
    }
    QVERIFY2(info, qPrintable(messages(issues).join(QStringLiteral(" | "))));
}

void TestPinsValidator::testValidatorConflictIsError()
{
    HardwareConfig cfg = probeConfig();
    PinConfiguration pin;
    pin.pin = "PA9";
    pin.mode = "GPIO_Output";   // PA9 also carries USART1_TX
    cfg.pins.insert("PA9", pin);

    const QVector<PinIssue> issues = PinsValidator::validate(cfg, m_caps);
    bool conflict = false;
    for (const PinIssue& issue : issues) {
        if (issue.severity == PinIssue::Severity::Error && issue.message.contains("multiple functions")
            && issue.message.contains("PA9")) {
            conflict = true;
        }
    }
    QVERIFY2(conflict, qPrintable(messages(issues).join(QStringLiteral(" | "))));
}

void TestPinsValidator::testValidatorInterruptResolvesExtiLine()
{
    HardwareConfig cfg;
    cfg.deviceId = "STM32F030R8";
    PinConfiguration pin;
    pin.pin = "PA9";
    pin.mode = "GPIO_Input";
    pin.interrupt = "RisingEdge";
    cfg.pins.insert("PA9", pin);

    const QVector<PinIssue> issues = PinsValidator::validate(cfg, m_caps);
    QVERIFY2(issues.isEmpty(), qPrintable(messages(issues).join(QStringLiteral(" | "))));

    const PeripheralPlan plan = PeripheralPlan::fromConfig(cfg, m_caps);
    QCOMPARE(plan.pins.size(), 1);
    QCOMPARE(plan.pins.first().extiLine, 9);
}

void TestPinsValidator::testValidatorWithoutDataIsInfo()
{
    HardwareConfig cfg;
    cfg.deviceId = "STM32F401";   // unverified / no capability data yet
    PinConfiguration pin;
    pin.pin = "PA5";
    pin.mode = "GPIO_Output";
    cfg.pins.insert("PA5", pin);

    const QVector<PinIssue> issues = PinsValidator::validate(cfg, nullptr);
    QCOMPARE(issues.size(), 1);
    QCOMPARE(issues.first().severity, PinIssue::Severity::Info);
    QVERIFY(issues.first().message.contains("STM32F401"));
    QVERIFY(issues.first().message.contains("no verified pin capability data"));
}

void TestPinsValidator::testDeviceDatabaseUnaffectedByPinDataFile()
{
    // The *_pin_data.json file lives next to the device packs. The pack loader
    // keys on "identity" + a non-empty identity.partNumber, so the capability
    // file (identity block without partNumber) must not replace the real
    // STM32F030R8 definition - and must not appear as a bogus device.
    const DeviceDefinition dev = Hardware::DeviceDatabase::instance().findDevice("STM32F030R8");
    QCOMPARE(dev.partNumber, QString("STM32F030R8"));
    QCOMPARE(dev.gpioCount, 55);
    QCOMPARE(dev.pins.size(), 40);
    const DeviceDefinition bogus = Hardware::DeviceDatabase::instance().findDevice("stm32f030r8_pin_data");
    QVERIFY(bogus.partNumber.isEmpty());
}

void TestPinsValidator::testDiagnosticsPinsSourceRegistered()
{
    Diagnostics::DiagnosticsService service;
    Diagnostics::DiagnosticsSources::registerBuiltinSources(service);
    QVERIFY(service.sourceNames().contains("Pins"));

    CanvasScene scene;
    Project project(&scene);

    // Probe B configuration: I2C1 assigned to PB10/PB11, which is the I2C2
    // pin pair on STM32F030R8. The "Pins" source must surface a Warning.
    HardwareConfig cfg;
    cfg.deviceId = "STM32F030R8";

    PinConfiguration scl;
    scl.pin = "PB10";
    scl.mode = "AlternateFunction";
    scl.alternateFunction = "I2C1_SCL";
    cfg.pins.insert("PB10", scl);

    PinConfiguration sda;
    sda.pin = "PB11";
    sda.mode = "AlternateFunction";
    sda.alternateFunction = "I2C1_SDA";
    cfg.pins.insert("PB11", sda);

    PeripheralConfiguration i2c;
    i2c.name = "I2C1";
    i2c.type = "I2C";
    i2c.enabled = true;
    i2c.assignedPins.insert("SCL", "PB10");
    i2c.assignedPins.insert("SDA", "PB11");
    cfg.peripherals.insert("I2C1", i2c);

    project.setHardwareConfig(cfg);

    service.setProject(&project);
    const QVector<Diagnostics::Diagnostic> diagnostics = service.runNow();

    int pinsWarnings = 0;
    for (const Diagnostics::Diagnostic& d : diagnostics) {
        if (d.source == "Pins"
            && d.severity == Diagnostics::Diagnostic::Severity::Warning
            && d.message.contains("PB10")
            && d.message.contains("not documented")) {
            pinsWarnings++;
        }
    }
    QVERIFY2(pinsWarnings >= 1, "expected a Pins Warning for I2C1_SCL on PB10");
}

QTEST_MAIN(TestPinsValidator)
#include "test_pins_validator.moc"
