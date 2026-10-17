/**
 * Task 4 — MCU Capability Database acceptance tests.
 *
 * Covers:
 *  - Every shipped resources/devices/*.json validates against _schema.json
 *    (mini draft-07-subset validator, no external dependency).
 *  - CapabilityDatabase loads the JSON overlay: >= 5 families / >= 8 devices,
 *    lookup by name, unknown-device handling, framework and family queries.
 *  - Provenance rules: every shipped device declares "verified" + non-empty
 *    "sources"; no device has verified=true with an empty sources list.
 *  - The vendor-verified corrections (H745/H743 RAM 512 -> 1056 KB, H743
 *    400 -> 480 MHz, H742 added, external-flash parts flagged as external).
 *  - Analyzer rule: external-flash parts must not treat a typical module size
 *    as a hard limit (Info instead of Critical); a board-provided flash size is
 *    enforced with the normal 80% / 95% thresholds.
 */
#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QStringList>
#include <cstdint>

#include "analyzer/CapabilityDatabase.h"
#include "analyzer/ResourceEstimator.h"
#include "analyzer/ResourceAnalysis.h"
#include "hardware/BoardRegistry.h"
#include "project/Project.h"
#include "models/Screen.h"
#include "models/ButtonComponent.h"
#include "models/ImageComponent.h"

class TestMcuCapabilities : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void testShippedFilesValidateAgainstSchema();
    void testDatabaseLoadsFamiliesAndDevices();
    void testLookupAndUnknownDeviceHandling();
    void testFrameworkAndFamilyQueries();
    void testProvenanceRules();
    void testVerifiedCorrectionsAndPinnedValues();
    void testExternalFlashFlags();
    void testExternalFlashAnalyzerRule();
    void testBoardProvidedFlashIsEnforced();
    void testInternalFlashThresholdsUnchanged();

private:
    // Minimal JSON-Schema (draft-07 subset) validator: type (string or array),
    // properties, required, items, additionalProperties=false, minimum.
    // Returns an empty string when the document is valid.
    static QString validate(const QJsonValue& schema, const QJsonValue& doc, const QString& path);

    QJsonObject m_schema;
    QStringList m_deviceFiles;
};

QString TestMcuCapabilities::validate(const QJsonValue& schema, const QJsonValue& doc, const QString& path)
{
    if (!schema.isObject()) return QString();
    const QJsonObject s = schema.toObject();

    // ── type ──
    if (s.contains("type")) {
        QStringList types;
        if (s.value("type").isArray()) {
            for (const QJsonValue& t : s.value("type").toArray()) types << t.toString();
        } else if (s.value("type").isString()) {
            types << s.value("type").toString();
        }
        if (!types.isEmpty()) {
            bool ok = false;
            for (const QString& t : types) {
                if (t == "object" && doc.isObject()) ok = true;
                else if (t == "array" && doc.isArray()) ok = true;
                else if (t == "string" && doc.isString()) ok = true;
                else if (t == "boolean" && doc.isBool()) ok = true;
                else if (t == "null" && doc.isNull()) ok = true;
                else if (t == "integer" && doc.isDouble()) {
                    const double d = doc.toDouble();
                    if (d == static_cast<double>(static_cast<qint64>(d))) ok = true;
                } else if (t == "number" && doc.isDouble()) ok = true;
            }
            if (!ok) {
                return QString("%1: type mismatch (expected %2)").arg(path, types.join("/"));
            }
        }
    }

    // ── minimum ──
    if (s.contains("minimum") && doc.isDouble()) {
        if (doc.toDouble() < s.value("minimum").toDouble()) {
            return QString("%1: %2 < minimum %3")
                .arg(path)
                .arg(doc.toDouble())
                .arg(s.value("minimum").toDouble());
        }
    }

    // ── object constraints ──
    if (doc.isObject()) {
        const QJsonObject obj = doc.toObject();
        const QJsonObject props = s.value("properties").toObject();

        for (const QJsonValue& r : s.value("required").toArray()) {
            if (!obj.contains(r.toString())) {
                return QString("%1: missing required property '%2'").arg(path, r.toString());
            }
        }

        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            if (props.contains(it.key())) {
                const QString err = validate(props.value(it.key()), it.value(), path + "." + it.key());
                if (!err.isEmpty()) return err;
            } else if (s.contains("additionalProperties")
                       && s.value("additionalProperties").isBool()
                       && !s.value("additionalProperties").toBool()) {
                return QString("%1: unexpected property '%2'").arg(path, it.key());
            }
        }
    }

    // ── array items ──
    if (doc.isArray() && s.contains("items")) {
        const QJsonArray arr = doc.toArray();
        for (int i = 0; i < arr.size(); ++i) {
            const QString err = validate(s.value("items"), arr.at(i), QString("%1[%2]").arg(path).arg(i));
            if (!err.isEmpty()) return err;
        }
    }

    return QString();
}

void TestMcuCapabilities::initTestCase()
{
    m_deviceFiles = QStringList{
        "stm32h7_series.json",
        "stm32f0_series.json",
        "esp32_series.json",
        "nxp_imxrt_series.json",
        "rpi_pico.json"
    };

    QFile sf(":/devices/_schema.json");
    QVERIFY2(sf.open(QIODevice::ReadOnly), ":/devices/_schema.json missing (resources/app.qrc)");
    const QJsonDocument sd = QJsonDocument::fromJson(sf.readAll());
    QVERIFY(sd.isObject());
    m_schema = sd.object();
}

void TestMcuCapabilities::testShippedFilesValidateAgainstSchema()
{
    // All device files must be present in the Qt resource system.
    const QStringList inResource = QDir(QStringLiteral(":/devices")).entryList(QStringList{"*.json"}, QDir::Files);
    QVERIFY2(inResource.contains(QStringLiteral("_schema.json")), qPrintable(inResource.join(",")));
    for (const QString& f : m_deviceFiles) {
        QVERIFY2(inResource.contains(f), qPrintable("qrc missing: " + f));

        QFile df(":/devices/" + f);
        QVERIFY2(df.open(QIODevice::ReadOnly), qPrintable(f));
        QJsonParseError perr;
        const QJsonDocument dd = QJsonDocument::fromJson(df.readAll(), &perr);
        QVERIFY2(perr.error == QJsonParseError::NoError, qPrintable(f + ": " + perr.errorString()));
        QVERIFY2(dd.isObject(), qPrintable(f + ": root must be an object"));

        const QString err = validate(QJsonValue(m_schema), QJsonValue(dd.object()), f);
        QVERIFY2(err.isEmpty(), qPrintable(err));
    }
}

void TestMcuCapabilities::testDatabaseLoadsFamiliesAndDevices()
{
    CapabilityDatabase db;
    QVERIFY(db.loadBuiltinDatabase());

    // Compiled-in profiles remain available (fallback and untouched devices).
    QVERIFY(db.deviceExists("STM32F746"));
    QVERIFY(db.deviceExists("STM32F429"));

    // At least 5 families and 8 devices total from the shipped database.
    QVERIFY2(db.listFamilies().size() >= 5, qPrintable(db.listFamilies().join(",")));
    QVERIFY(db.listAllDevices().size() >= 8);

    const QStringList required = {
        "STM32H745", "STM32H743", "STM32H742", "STM32F030R8",
        "ESP32", "ESP32-S3", "ESP32-C3", "ESP32-C6",
        "NXP i.MX RT1060", "Raspberry Pi Pico", "Raspberry Pi Pico 2"
    };
    for (const QString& name : required) {
        QVERIFY2(db.deviceExists(name), qPrintable(name));
    }

    const QStringList families = db.listFamilies();
    for (const QString& f : QStringList{"STM32H7", "STM32F0", "ESP32", "i.MX RT", "RP2040", "RP2350"}) {
        QVERIFY2(families.contains(f), qPrintable(families.join(",")));
    }
}

void TestMcuCapabilities::testLookupAndUnknownDeviceHandling()
{
    CapabilityDatabase db;
    QVERIFY(db.loadBuiltinDatabase());

    // Lookup by exact and case-insensitive name.
    const DeviceProfile h742 = db.getDeviceProfile("stm32h742");
    QCOMPARE(h742.name, QString("STM32H742"));
    QCOMPARE(h742.ram_kb, 688u);
    QVERIFY(h742.verified);
    QVERIFY(!h742.sources.isEmpty());
    QVERIFY(!h742.ram_regions.isEmpty());

    // Unknown device: deviceExists() is false and the documented Generic
    // fallback profile is returned (existing contract, unchanged).
    QVERIFY(!db.deviceExists("TotallyUnknownMCU123"));
    const DeviceProfile unknown = db.getDeviceProfile("TotallyUnknownMCU123");
    QCOMPARE(unknown.family, QString("Generic"));
    QCOMPARE(unknown.ram_kb, 256u);
    QCOMPARE(unknown.flash_kb, 1024u);
}

void TestMcuCapabilities::testFrameworkAndFamilyQueries()
{
    CapabilityDatabase db;
    QVERIFY(db.loadBuiltinDatabase());

    QVERIFY(db.supportsFramework("STM32H745", "lvgl"));
    QVERIFY(db.supportsFramework("STM32H742", "embedweb"));
    QVERIFY(!db.supportsFramework("STM32F030R8", "qt-for-mcus"));
    QVERIFY(db.supportsFramework("STM32F030R8", "ugfx"));

    const QStringList h7 = db.getDevicesByFamily("STM32H7");
    QVERIFY(h7.contains("STM32H742"));
    QVERIFY(h7.contains("STM32H743"));
    QVERIFY(h7.contains("STM32H745"));

    // Case-insensitive family query.
    const QStringList esp = db.getDevicesByFamily("esp32");
    QCOMPARE(esp.size(), 4);
}

void TestMcuCapabilities::testProvenanceRules()
{
    // Every shipped device must declare verification + non-empty sources,
    // checked directly on the JSON (schema already enforces presence).
    for (const QString& f : m_deviceFiles) {
        QFile df(":/devices/" + f);
        QVERIFY2(df.open(QIODevice::ReadOnly), qPrintable(f));
        const QJsonObject root = QJsonDocument::fromJson(df.readAll()).object();
        for (const QJsonValue& v : root.value("devices").toArray()) {
            const QJsonObject d = v.toObject();
            const QString name = d.value("name").toString();
            QVERIFY2(d.value("verified").isBool(), qPrintable(name + ": verified must be a bool"));
            const QJsonArray sources = d.value("sources").toArray();
            QVERIFY2(sources.size() > 0, qPrintable(name + ": empty sources"));
            for (const QJsonValue& s : sources) {
                QVERIFY2(!s.toString().trimmed().isEmpty(), qPrintable(name + ": blank source entry"));
            }
        }
    }

    // Database-level rule: verified=true implies non-empty sources.
    CapabilityDatabase db;
    QVERIFY(db.loadBuiltinDatabase());
    int verifiedCount = 0;
    for (const QString& name : db.listAllDevices()) {
        const DeviceProfile p = db.getDeviceProfile(name);
        if (p.verified) {
            verifiedCount++;
            QVERIFY2(!p.sources.isEmpty(), qPrintable(name + " is verified but has no sources"));
        }
    }
    QVERIFY2(verifiedCount >= 11, qPrintable(QString("verified devices: %1").arg(verifiedCount)));
}

void TestMcuCapabilities::testVerifiedCorrectionsAndPinnedValues()
{
    CapabilityDatabase db;
    QVERIFY(db.loadBuiltinDatabase());

    // STM32H745: RAM total 512 -> 1056 KB (region sum), Stage 4.x pins preserved.
    const DeviceProfile h745 = db.getDeviceProfile("STM32H745");
    QCOMPARE(h745.ram_kb, 1056u);
    QCOMPARE(h745.flash_kb, 2048u);
    QCOMPARE(h745.cpu_speed_mhz, 480u);
    QVERIFY(!h745.flash_is_external);
    QCOMPARE(h745.internal_sram_kb, 1024u);   // kept: ST "1 Mbyte" headline
    QCOMPARE(h745.dma_sram_kb, 512u);
    QCOMPARE(h745.tcm_kb, 192u);
    QCOMPARE(h745.onchip_flash_kb, 2048u);
    QCOMPARE(h745.has_lcd_controller, 1);
    QCOMPARE(h745.cores.size(), 2);
    uint32_t regionSum = 0;
    for (const DeviceProfile::RamRegion& r : h745.ram_regions) regionSum += r.kb;
    QCOMPARE(regionSum, h745.ram_kb);

    // STM32H743: 400 -> 480 MHz per DS12110 Rev 10; same RAM correction.
    const DeviceProfile h743 = db.getDeviceProfile("STM32H743");
    QCOMPARE(h743.cpu_speed_mhz, 480u);
    QCOMPARE(h743.ram_kb, 1056u);
    QCOMPARE(h743.internal_sram_kb, 1024u);

    // STM32H742: newly added, 688 KB RAM, no LCD-TFT controller.
    const DeviceProfile h742 = db.getDeviceProfile("STM32H742");
    QCOMPARE(h742.family, QString("STM32H7"));
    QCOMPARE(h742.ram_kb, 688u);
    QCOMPARE(h742.flash_kb, 2048u);
    QCOMPARE(h742.tcm_kb, 192u);
    QCOMPARE(h742.has_lcd_controller, 0);
    regionSum = 0;
    for (const DeviceProfile::RamRegion& r : h742.ram_regions) regionSum += r.kb;
    QCOMPARE(regionSum, h742.ram_kb);

    // STM32F030R8: verified to match the compiled-in values, kept as-is.
    const DeviceProfile f030 = db.getDeviceProfile("STM32F030R8");
    QCOMPARE(f030.ram_kb, 8u);
    QCOMPARE(f030.flash_kb, 64u);
    QCOMPARE(f030.cpu_speed_mhz, 48u);
    QCOMPARE(f030.internal_sram_kb, 8u);
    QCOMPARE(f030.onchip_flash_kb, 64u);
    QCOMPARE(f030.safety_margin_percent, 10.0f);
}

void TestMcuCapabilities::testExternalFlashFlags()
{
    CapabilityDatabase db;
    QVERIFY(db.loadBuiltinDatabase());

    const DeviceProfile esp32 = db.getDeviceProfile("ESP32");
    QVERIFY(esp32.flash_is_external);
    QCOMPARE(esp32.onchip_flash_kb, 0u);
    QCOMPARE(esp32.external_flash_kb, 4096u);

    const DeviceProfile s3 = db.getDeviceProfile("ESP32-S3");
    QVERIFY(s3.flash_is_external);
    QCOMPARE(s3.external_flash_kb, 8192u);
    QCOMPARE(s3.psram_kb, 8192u);
    QCOMPARE(s3.dma_sram_kb, 384u);
    QCOMPARE(s3.has_lcd_controller, 1);

    const DeviceProfile c3 = db.getDeviceProfile("ESP32-C3");
    QVERIFY(c3.flash_is_external);
    QCOMPARE(c3.external_flash_kb, 4096u);

    const DeviceProfile c6 = db.getDeviceProfile("ESP32-C6");
    QVERIFY(c6.flash_is_external);
    QCOMPARE(c6.external_flash_kb, 8192u);   // corrected from 4096 (DevKitC-1 = N8)

    const DeviceProfile rt = db.getDeviceProfile("NXP i.MX RT1060");
    QVERIFY(rt.flash_is_external);
    QCOMPARE(rt.onchip_flash_kb, 0u);
    QCOMPARE(rt.external_flash_kb, 8192u);
    QCOMPARE(rt.dma_sram_kb, 512u);
    QCOMPARE(rt.tcm_kb, 512u);
    QCOMPARE(rt.has_lcd_controller, 1);

    const DeviceProfile pico = db.getDeviceProfile("Raspberry Pi Pico");
    QVERIFY(pico.flash_is_external);
    QCOMPARE(pico.ram_kb, 264u);
    QCOMPARE(pico.external_flash_kb, 2048u);
    QCOMPARE(pico.dma_sram_kb, 264u);

    const DeviceProfile pico2 = db.getDeviceProfile("Raspberry Pi Pico 2");
    QVERIFY(pico2.flash_is_external);
    QCOMPARE(pico2.ram_kb, 520u);
    QCOMPARE(pico2.external_flash_kb, 4096u);

    // Internal-flash parts stay internal.
    QVERIFY(!db.getDeviceProfile("STM32F030R8").flash_is_external);
    QVERIFY(!db.getDeviceProfile("STM32H742").flash_is_external);
}

void TestMcuCapabilities::testExternalFlashAnalyzerRule()
{
    // Build a design whose 3 MB asset far exceeds the RP2040's typical 2 MB
    // module flash. Without the external-flash rule this would be CRITICAL.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const int assetBytes = 3 * 1024 * 1024;
    const QString assetPath = tmp.filePath("big_asset.bin");
    {
        QFile f(assetPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QCOMPARE(f.write(QByteArray(assetBytes, 'A')), static_cast<qint64>(assetBytes));
    }

    Project proj(nullptr);
    proj.newProject("ExtFlashNoBoard", 320, 240);
    proj.setTargetFramework("lvgl");
    Screen* scr = proj.activeScreen();
    QVERIFY(scr != nullptr);
    auto* img = new ImageComponent("img_ext_big");
    img->setImagePath(assetPath);
    img->setCompSize(8, 8);   // negligible decoded RAM
    scr->addComponent(img);

    ResourceEstimator estimator;
    ResourceAnalysis a = estimator.analyze(&proj, "Raspberry Pi Pico");

    QVERIFY(a.has_device);
    QCOMPARE(a.device_name, QString("Raspberry Pi Pico"));
    QVERIFY(a.flash_external_unbounded);
    QVERIFY2(a.flash_percent > 95.0f, qPrintable(QString("flash_percent=%1").arg(a.flash_percent)));

    // Must NOT be flagged Critical: the module size is not a hard limit.
    QCOMPARE(a.status, ResourceAnalysis::OK);

    bool infoFound = false;
    bool criticalFlash = false;
    for (const ResourceAnalysis::ResourceWarning& w : a.structuredWarnings) {
        if (w.message.contains("Flash size depends on your board")) {
            if (w.severity == ResourceAnalysis::WarningSeverity::Info) infoFound = true;
            else criticalFlash = true;
        }
        if (w.severity == ResourceAnalysis::WarningSeverity::Critical
            && w.message.contains("Flash")) {
            criticalFlash = true;
        }
    }
    QVERIFY2(infoFound, "expected Info 'Flash size depends on your board'");
    QVERIFY(!criticalFlash);

    // The Info notice is also visible to the plain warnings list (panels).
    bool inPlainWarnings = false;
    for (const QString& w : a.warnings) {
        if (w.contains("Flash size depends on your board")) inPlainWarnings = true;
    }
    QVERIFY(inPlainWarnings);
}

void TestMcuCapabilities::testBoardProvidedFlashIsEnforced()
{
    BoardRegistry& reg = BoardRegistry::instance();
    const BoardDef espBoard = reg.board("esp32_devkitv1");
    QVERIFY2(espBoard.flashBytes > 0, "boards.json not available to BoardRegistry");

    Project proj(nullptr);
    proj.newProject("ExtFlashBoard", 320, 240);
    proj.setTargetFramework("lvgl");
    Hardware::HardwareConfig hw;
    hw.targetType = "board";
    hw.deviceId = "ESP32";
    hw.boardId = espBoard.id;
    proj.setHardwareConfig(hw);
    Screen* scr = proj.activeScreen();
    QVERIFY(scr != nullptr);
    scr->addComponent(new ButtonComponent("btn_board"));

    // 1) Known board size: the board profile flash budget is used as the total
    //    and the flash is no longer "unbounded".
    ResourceEstimator estimatorBoard;
    ResourceAnalysis ba = estimatorBoard.analyze(&proj, "ESP32");
    QVERIFY(ba.has_device);
    QVERIFY(!ba.flash_external_unbounded);
    QCOMPARE(ba.flash_total, static_cast<uint32_t>(espBoard.flashBytes));

    // 2) A board whose flash budget is 1 MB plus a 3 MB asset => Critical.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString assetPath = tmp.filePath("big_asset.bin");
    {
        QFile f(assetPath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        QVERIFY(f.write(QByteArray(3 * 1024 * 1024, 'A')) > 0);
    }
    auto* img = new ImageComponent("img_board_big");
    img->setImagePath(assetPath);
    img->setCompSize(8, 8);
    scr->addComponent(img);

    Hardware::HardwareConfig small = hw;
    small.flashBytes = 1024 * 1024;   // 1 MB firmware budget
    proj.setHardwareConfig(small);

    ResourceEstimator estimatorSmall;
    ResourceAnalysis sa = estimatorSmall.analyze(&proj, "ESP32");
    QVERIFY(!sa.flash_external_unbounded);
    QCOMPARE(sa.flash_total, 1024u * 1024u);
    QCOMPARE(sa.status, ResourceAnalysis::CRITICAL);
    bool criticalFlash = false;
    for (const ResourceAnalysis::ResourceWarning& w : sa.structuredWarnings) {
        if (w.severity == ResourceAnalysis::WarningSeverity::Critical && w.message.contains("Flash")) {
            criticalFlash = true;
        }
    }
    QVERIFY(criticalFlash);
}

void TestMcuCapabilities::testInternalFlashThresholdsUnchanged()
{
    ResourceEstimator estimator;
    const DeviceProfile f030 = estimator.capabilityDatabase().getDeviceProfile("STM32F030R8");

    auto makeAnalysis = [](const DeviceProfile& dev, float flashPct) {
        ResourceAnalysis a;
        a.has_device = true;
        a.device_name = dev.name;
        a.device = dev;
        a.flash_percent = flashPct;
        a.ram_percent = 10.0f;
        a.flash_total = dev.flash_kb * 1024;
        a.ram_total = dev.ram_kb * 1024;
        a.flash_used = static_cast<uint32_t>(static_cast<float>(a.flash_total) * flashPct / 100.0f);
        a.ram_used = static_cast<uint32_t>(a.ram_total / 10);
        return a;
    };

    // 85% => Warning (unchanged 80% threshold for internal flash).
    {
        ResourceAnalysis a = makeAnalysis(f030, 85.0f);
        ResourceAnalysis::Status st = ResourceAnalysis::OK;
        QVector<QString> w;
        estimator.validateFitsOnDevice(a, st, w);
        QCOMPARE(st, ResourceAnalysis::WARNING);
    }

    // 97% => Critical (unchanged 95% threshold).
    {
        ResourceAnalysis a = makeAnalysis(f030, 97.0f);
        ResourceAnalysis::Status st = ResourceAnalysis::OK;
        QVector<QString> w;
        estimator.validateFitsOnDevice(a, st, w);
        QCOMPARE(st, ResourceAnalysis::CRITICAL);
    }

    // External part flagged unbounded at 150% => still OK + Info notice only.
    {
        const DeviceProfile pico = estimator.capabilityDatabase().getDeviceProfile("Raspberry Pi Pico");
        ResourceAnalysis a = makeAnalysis(pico, 150.0f);
        a.flash_external_unbounded = true;
        ResourceAnalysis::Status st = ResourceAnalysis::OK;
        QVector<QString> w;
        estimator.validateFitsOnDevice(a, st, w);
        QCOMPARE(st, ResourceAnalysis::OK);
        bool info = false;
        for (const ResourceAnalysis::ResourceWarning& x : a.structuredWarnings) {
            if (x.severity == ResourceAnalysis::WarningSeverity::Info
                && x.message.contains("Flash size depends on your board")) {
                info = true;
            }
        }
        QVERIFY(info);
    }
}

QTEST_MAIN(TestMcuCapabilities)
#include "test_mcu_capabilities.moc"
