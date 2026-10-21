#include "BoardDatabase.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDebug>

BoardDatabase& BoardDatabase::instance() {
    static BoardDatabase s_instance;
    return s_instance;
}

BoardDatabase::BoardDatabase(QObject* parent)
    : QObject(parent)
{
    loadDatabase();
}

bool BoardDatabase::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[BoardDatabase] Failed to open board database:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    return parseJson(data);
}

bool BoardDatabase::loadFromAPI(const QString& /*apiUrl*/) {
    return loadDatabase();
}

bool BoardDatabase::loadDatabase(const QString& jsonPath) {
    QString path = jsonPath;
    if (path.isEmpty()) {
        QString appDir = QCoreApplication::applicationDirPath();
        QStringList candidates = {
            "config/boards/boards.json",
            appDir + "/config/boards/boards.json",
            appDir + "/../config/boards/boards.json"
        };
        for (const QString& candidate : candidates) {
            if (QFile::exists(candidate)) {
                path = candidate;
                break;
            }
        }
    }

    if (!path.isEmpty() && loadFromFile(path)) {
        qDebug() << "[BoardDatabase] Loaded" << m_boards.size() << "boards from" << path;
        return true;
    }

    populateDefaults();
    return !m_boards.empty();
}

bool BoardDatabase::parseJson(const QByteArray& jsonData) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[BoardDatabase] JSON parse error:" << err.errorString();
        return false;
    }

    QJsonArray boardsArray;
    if (doc.isArray()) {
        boardsArray = doc.array();
    } else if (doc.isObject()) {
        QJsonObject root = doc.object();
        if (root.contains("boards") && root["boards"].isArray()) {
            boardsArray = root["boards"].toArray();
        } else {
            return false;
        }
    } else {
        return false;
    }

    if (boardsArray.isEmpty()) {
        return false;
    }

    m_boards.clear();
    m_boardsById.clear();
    m_boardsByUsbKey.clear();

    for (const QJsonValue& val : boardsArray) {
        if (!val.isObject()) continue;
        BoardSpec spec = parseBoard(val.toObject());
        m_boards.push_back(spec);
        if (!spec.id.isEmpty()) {
            m_boardsById[spec.id] = spec;
        }
        if (!spec.name.isEmpty()) {
            m_boardsById[spec.name] = spec;
        }
        if (!spec.vid.isEmpty() && !spec.pid.isEmpty()) {
            m_boardsByUsbKey[QString("%1:%2").arg(spec.vid, spec.pid)] = spec;
        }
    }

    emit databaseUpdated();
    return !m_boards.empty();
}

BoardSpec BoardDatabase::parseBoard(const QJsonObject& b) {
    BoardSpec spec;
    spec.id = b["id"].toString();
    spec.name = b["name"].toString();
    spec.manufacturer = b["manufacturer"].toString();
    spec.family = b["family"].toString();

    if (b.contains("mcuModel")) {
        spec.mcuModel = b["mcuModel"].toString();
        spec.mcu = spec.mcuModel;
    } else {
        spec.mcu = b["mcu"].toString();
        spec.mcuModel = spec.mcu;
    }

    spec.architecture = b["architecture"].toString();
    spec.frequencyMhz = b["frequency_mhz"].toInt();

    if (b.contains("flash_kb")) {
        spec.flashKb = b["flash_kb"].toInt();
        spec.flashMb = b.contains("flashMb") ? b["flashMb"].toDouble() : (spec.flashKb / 1024.0);
    } else if (b.contains("flashMb")) {
        spec.flashMb = b["flashMb"].toDouble();
        spec.flashKb = static_cast<int>(spec.flashMb * 1024.0);
    }

    spec.ramKb = b.contains("ram_kb") ? b["ram_kb"].toInt() : b["ramKb"].toInt();

    if (b.contains("gpio_count")) {
        spec.gpioCount = b["gpio_count"].toInt();
        spec.gpioPins = b.contains("gpioPins") ? b["gpioPins"].toInt() : spec.gpioCount;
    } else if (b.contains("gpioPins")) {
        spec.gpioPins = b["gpioPins"].toInt();
        spec.gpioCount = spec.gpioPins;
    }

    QJsonArray aPins = b["analog_pins"].toArray();
    for (const auto& p : aPins) spec.analogPins.append(p.toString());

    QJsonArray pPins = b["pwm_pins"].toArray();
    for (const auto& p : pPins) spec.pwmPins.append(p.toString());

    QJsonArray dPins = b["digital_pins"].toArray();
    for (const auto& p : dPins) spec.digitalPins.append(p.toString());

    if (b["usb_adapter"].isObject()) {
        QJsonObject usb = b["usb_adapter"].toObject();
        spec.usbAdapterType = usb["type"].toString();
        spec.usbAdapter = spec.usbAdapterType;
        spec.vid = usb["vid"].toString().toLower();
        spec.pid = usb["pid"].toString().toLower();
        spec.driver = usb["driver"].toString();
    } else if (b.contains("usbAdapter")) {
        spec.usbAdapter = b["usbAdapter"].toString();
        spec.usbAdapterType = spec.usbAdapter;
    }

    if (b["openocd"].isObject()) {
        QJsonObject ocd = b["openocd"].toObject();
        spec.openocdInterface = ocd["interface"].toString();
        spec.openocdTarget = ocd["target"].toString();
        spec.openocdConfigFile = ocd["config_file"].toString();
        spec.openOcdConfig = spec.openocdConfigFile.isEmpty() ? ocd["target"].toString() : spec.openocdConfigFile;
        spec.openocdTransport = ocd["transport"].toString();
    } else if (b.contains("openOcdConfig")) {
        spec.openOcdConfig = b["openOcdConfig"].toString();
        spec.openocdConfigFile = spec.openOcdConfig;
    }

    if (b["toolchain"].isObject()) {
        QJsonObject tc = b["toolchain"].toObject();
        spec.compiler = tc["compiler"].toString();
        spec.toolchain = spec.compiler;
        spec.halHeader = tc["hal_header"].toString();
        spec.flags = tc["flags"].toString();
    } else if (b.contains("toolchain")) {
        spec.toolchain = b["toolchain"].toString();
        spec.compiler = spec.toolchain;
    }

    spec.datasheetUrl = b.contains("datasheetUrl") ? b["datasheetUrl"].toString() : b["datasheet_url"].toString();
    spec.productUrl = b.contains("productUrl") ? b["productUrl"].toString() : b["product_url"].toString();

    QJsonArray feats = b["features"].toArray();
    for (const auto& f : feats) {
        spec.features.append(f.toString());
    }

    if (spec.id.isEmpty()) {
        spec.id = spec.name.toLower().replace(' ', '_').replace('-', '_');
    }

    return spec;
}

void BoardDatabase::populateDefaults() {
    if (!m_boards.empty()) return;

    BoardSpec stm32;
    stm32.id = "nucleo_f030r8";
    stm32.name = "STM32 NUCLEO-F030R8";
    stm32.mcu = "STM32F030R8";
    stm32.mcuModel = "STM32F030R8";
    stm32.flashMb = 0.064;
    stm32.flashKb = 64;
    stm32.ramKb = 8;
    stm32.gpioPins = 37;
    stm32.gpioCount = 37;
    stm32.usbAdapter = "ST-Link";
    stm32.usbAdapterType = "ST-Link";
    stm32.vid = "0483";
    stm32.pid = "374b";
    stm32.openOcdConfig = "target/stm32f0x.cfg";
    stm32.openocdConfigFile = "config/openocd/stm32f030r8.cfg";
    stm32.datasheetUrl = "https://www.st.com/resource/en/datasheet/stm32f030r8.pdf";
    stm32.toolchain = "arm-none-eabi-gcc";
    m_boards.push_back(stm32);
    m_boardsById[stm32.id] = stm32;
    m_boardsById[stm32.name] = stm32;
    m_boardsByUsbKey["0483:374b"] = stm32;

    BoardSpec esp32;
    esp32.id = "esp32_devkitv1_cp2104";
    esp32.name = "ESP32 DEVKITV1";
    esp32.mcu = "ESP32";
    esp32.mcuModel = "ESP32";
    esp32.flashMb = 4.0;
    esp32.flashKb = 4096;
    esp32.ramKb = 520;
    esp32.gpioPins = 34;
    esp32.gpioCount = 34;
    esp32.usbAdapter = "CP2104";
    esp32.usbAdapterType = "CP2104";
    esp32.vid = "10c4";
    esp32.pid = "ea60";
    esp32.openOcdConfig = "target/esp32.cfg";
    esp32.openocdConfigFile = "config/openocd/esp32-devkitv1-cp210x.cfg";
    esp32.datasheetUrl = "https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf";
    esp32.toolchain = "esp-idf";
    m_boards.push_back(esp32);
    m_boardsById[esp32.id] = esp32;
    m_boardsById[esp32.name] = esp32;
    m_boardsByUsbKey["10c4:ea60"] = esp32;

    BoardSpec pico;
    pico.id = "rpi_pico_rp2040";
    pico.name = "Raspberry Pi Pico";
    pico.mcu = "RP2040";
    pico.mcuModel = "RP2040";
    pico.flashMb = 2.0;
    pico.flashKb = 2048;
    pico.ramKb = 264;
    pico.gpioPins = 26;
    pico.gpioCount = 26;
    pico.usbAdapter = "CMSIS-DAP";
    pico.usbAdapterType = "CMSIS-DAP";
    pico.vid = "2e8a";
    pico.pid = "0003";
    pico.openOcdConfig = "target/rp2040.cfg";
    pico.openocdConfigFile = "target/rp2040.cfg";
    pico.datasheetUrl = "https://datasheets.raspberrypi.org/rp2040/rp2040-datasheet.pdf";
    pico.toolchain = "arm-none-eabi-gcc";
    m_boards.push_back(pico);
    m_boardsById[pico.id] = pico;
    m_boardsById[pico.name] = pico;
    m_boardsByUsbKey["2e8a:0003"] = pico;
}

std::vector<BoardSpec> BoardDatabase::getAllBoards() const {
    return m_boards;
}

QList<BoardSpec> BoardDatabase::allBoards() const {
    QList<BoardSpec> list;
    list.reserve(static_cast<qsizetype>(m_boards.size()));
    for (const auto& b : m_boards) {
        list.append(b);
    }
    return list;
}

BoardSpec BoardDatabase::getBoardByName(const QString& name) const {
    return findBoardByName(name);
}

std::vector<BoardSpec> BoardDatabase::getBoardsByMCU(const QString& mcu) const {
    std::vector<BoardSpec> results;
    for (const auto& board : m_boards) {
        if (board.mcuModel.compare(mcu, Qt::CaseInsensitive) == 0 ||
            board.mcu.compare(mcu, Qt::CaseInsensitive) == 0 ||
            board.mcu.startsWith(mcu, Qt::CaseInsensitive) ||
            board.mcuModel.startsWith(mcu, Qt::CaseInsensitive)) {
            results.push_back(board);
        }
    }
    return results;
}

QStringList BoardDatabase::getBoardNames() const {
    QStringList names;
    for (const auto& board : m_boards) {
        names.append(board.name);
    }
    return names;
}

BoardSpec BoardDatabase::findBoardById(const QString& id) const {
    return m_boardsById.value(id);
}

BoardSpec BoardDatabase::findBoardByUsb(const QString& vid, const QString& pid) const {
    QString key = QString("%1:%2").arg(vid.toLower(), pid.toLower());
    if (m_boardsByUsbKey.contains(key)) {
        return m_boardsByUsbKey.value(key);
    }
    for (const auto& b : m_boards) {
        if (b.vid.compare(vid, Qt::CaseInsensitive) == 0) {
            return b;
        }
    }
    return BoardSpec();
}

BoardSpec BoardDatabase::findBoardByName(const QString& name) const {
    for (const auto& b : m_boards) {
        if (b.name.compare(name, Qt::CaseInsensitive) == 0 ||
            b.id.compare(name, Qt::CaseInsensitive) == 0) {
            return b;
        }
    }

    // Normalized match (ignore whitespace and special chars)
    QString cleanSearch = name;
    cleanSearch.remove(' ').remove('-').remove('_');
    for (const auto& b : m_boards) {
        QString cleanName = b.name;
        cleanName.remove(' ').remove('-').remove('_');
        if (cleanName.compare(cleanSearch, Qt::CaseInsensitive) == 0 ||
            cleanName.startsWith(cleanSearch, Qt::CaseInsensitive)) {
            return b;
        }
    }

    for (const auto& b : m_boards) {
        if (b.name.contains(name, Qt::CaseInsensitive) ||
            name.contains(b.name, Qt::CaseInsensitive)) {
            return b;
        }
    }

    return BoardSpec();
}

BoardSpec BoardDatabase::findBoardByProbeName(const QString& probeName) const {
    if (probeName.contains("ST-Link", Qt::CaseInsensitive) || probeName.contains("0483")) {
        return findBoardById("nucleo_f030r8");
    }
    if (probeName.contains("CP210", Qt::CaseInsensitive) || probeName.contains("10c4")) {
        return findBoardById("esp32_devkitv1_cp2104");
    }
    if (probeName.contains("CH340", Qt::CaseInsensitive) || probeName.contains("1a86")) {
        return findBoardById("esp32_devkitv1_ch340");
    }
    if (probeName.contains("ESP32-S3", Qt::CaseInsensitive) || (probeName.contains("303a") && probeName.contains("s3", Qt::CaseInsensitive))) {
        return findBoardById("esp32_s3_devkitc");
    }
    if (probeName.contains("ESP32", Qt::CaseInsensitive)) {
        return findBoardById("esp32_devkitv1_cp2104");
    }
    if (probeName.contains("Pico", Qt::CaseInsensitive) || probeName.contains("RP2040", Qt::CaseInsensitive) || probeName.contains("2e8a")) {
        return findBoardById("rpi_pico_rp2040");
    }
    if (probeName.contains("i.MX", Qt::CaseInsensitive) || probeName.contains("RT1060", Qt::CaseInsensitive) || probeName.contains("0d28")) {
        return findBoardById("nxp_imxrt1060_evk");
    }
    return BoardSpec();
}

bool BoardDatabase::syncFromEspboardsDev(const QString& /*apiUrl*/) {
    return loadDatabase();
}
