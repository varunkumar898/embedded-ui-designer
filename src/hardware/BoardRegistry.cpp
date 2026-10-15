#include "BoardRegistry.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <algorithm>

QJsonObject BoardDef::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["displayName"] = displayName;
    obj["vendor"] = vendor;
    obj["family"] = family;
    obj["display"] = display.toJson();
    
    QJsonObject pinMapObj;
    for (auto it = pinMap.begin(); it != pinMap.end(); ++it) {
        pinMapObj[it.key()] = it.value().toJson();
    }
    obj["pinMap"] = pinMapObj;

    QJsonObject protoObj;
    for (auto it = protocols.begin(); it != protocols.end(); ++it) {
        QJsonArray arr;
        for (const QString& p : it.value()) arr.append(p);
        protoObj[it.key()] = arr;
    }
    obj["protocols"] = protoObj;

    obj["openocdConfig"] = openocdConfig;
    obj["flashCommand"] = flashCommand;
    obj["flashBytes"] = flashBytes;
    obj["ramBytes"] = ramBytes;
    obj["clockMhz"] = clockMhz;
    obj["architecture"] = architecture;
    obj["mcu"] = mcu;
    return obj;
}

BoardDef BoardDef::fromJson(const QJsonObject& obj) {
    BoardDef b;
    b.id = obj.value("id").toString();
    b.displayName = obj.value("displayName").toString();
    if (b.displayName.isEmpty()) {
        b.displayName = obj.value("name").toString();
    }
    b.vendor = obj.value("vendor").toString();
    if (b.vendor.isEmpty()) {
        b.vendor = obj.value("manufacturer").toString();
    }
    b.family = obj.value("family").toString();
    if (b.family.isEmpty()) {
        b.family = "Custom";
    }

    if (obj.contains("display") && obj.value("display").isObject()) {
        b.display = DisplayDef::fromJson(obj.value("display").toObject());
    }

    if (obj.contains("pinMap") && obj.value("pinMap").isObject()) {
        QJsonObject pinObj = obj.value("pinMap").toObject();
        for (auto it = pinObj.begin(); it != pinObj.end(); ++it) {
            b.pinMap[it.key()] = PinDef::fromJson(it.value().toObject());
        }
    }

    if (obj.contains("protocols") && obj.value("protocols").isObject()) {
        QJsonObject protoObj = obj.value("protocols").toObject();
        for (auto it = protoObj.begin(); it != protoObj.end(); ++it) {
            QStringList list;
            for (const auto& v : it.value().toArray()) {
                list.append(v.toString());
            }
            b.protocols[it.key()] = list;
        }
    }

    b.openocdConfig = obj.value("openocdConfig").toString();
    if (b.openocdConfig.isEmpty()) {
        b.openocdConfig = obj.value("openOcdConfig").toString();
    }
    if (b.openocdConfig.isEmpty() && obj.contains("openocd") && obj.value("openocd").isObject()) {
        QJsonObject o = obj.value("openocd").toObject();
        b.openocdConfig = o.value("config_file").toString();
        if (b.openocdConfig.isEmpty()) b.openocdConfig = o.value("target").toString();
    }

    b.flashCommand = obj.value("flashCommand").toString();
    if (b.flashCommand.isEmpty()) {
        b.flashCommand = obj.value("flash_command").toString();
    }

    if (obj.contains("flash_kb")) {
        b.flashBytes = static_cast<qint64>(obj.value("flash_kb").toInt()) * 1024;
    } else if (obj.contains("flashMb")) {
        b.flashBytes = static_cast<qint64>(obj.value("flashMb").toDouble() * 1024.0 * 1024.0);
    }

    if (obj.contains("ram_kb")) {
        b.ramBytes = static_cast<qint64>(obj.value("ram_kb").toInt()) * 1024;
    } else if (obj.contains("ramKb")) {
        b.ramBytes = static_cast<qint64>(obj.value("ramKb").toInt()) * 1024;
    }

    if (obj.contains("frequency_mhz")) {
        b.clockMhz = obj.value("frequency_mhz").toInt();
    }

    b.architecture = obj.value("architecture").toString();
    b.mcu = obj.value("mcu").toString();
    if (b.mcu.isEmpty()) {
        b.mcu = obj.value("mcuModel").toString();
    }

    return b;
}

QJsonObject BoardPreset::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["boardId"] = boardId;
    obj["isCustom"] = isCustom;

    QJsonObject pinObj;
    for (auto it = pinOverrides.begin(); it != pinOverrides.end(); ++it) {
        pinObj[it.key()] = it.value().toJson();
    }
    obj["pinOverrides"] = pinObj;

    QJsonObject protoObj;
    for (auto it = protocolOverrides.begin(); it != protocolOverrides.end(); ++it) {
        QJsonArray arr;
        for (const QString& p : it.value()) arr.append(p);
        protoObj[it.key()] = arr;
    }
    obj["protocolOverrides"] = protoObj;

    return obj;
}

BoardPreset BoardPreset::fromJson(const QJsonObject& obj) {
    BoardPreset p;
    p.name = obj.value("name").toString();
    p.boardId = obj.value("boardId").toString();
    p.isCustom = obj.value("isCustom").toBool(false);

    if (obj.contains("pinOverrides") && obj.value("pinOverrides").isObject()) {
        QJsonObject pinObj = obj.value("pinOverrides").toObject();
        for (auto it = pinObj.begin(); it != pinObj.end(); ++it) {
            p.pinOverrides[it.key()] = PinDef::fromJson(it.value().toObject());
        }
    }

    if (obj.contains("protocolOverrides") && obj.value("protocolOverrides").isObject()) {
        QJsonObject protoObj = obj.value("protocolOverrides").toObject();
        for (auto it = protoObj.begin(); it != protoObj.end(); ++it) {
            QStringList list;
            for (const auto& v : it.value().toArray()) {
                list.append(v.toString());
            }
            p.protocolOverrides[it.key()] = list;
        }
    }

    return p;
}

BoardRegistry& BoardRegistry::instance() {
    static BoardRegistry reg;
    return reg;
}

BoardRegistry::BoardRegistry() {
    loadFromBoardsJson();
    loadPresets();
}

QString BoardRegistry::presetsFilePath() const {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    return path + "/presets.json";
}

bool BoardRegistry::loadFromBoardsJson(const QString& customPath) {
    if (m_loaded && customPath.isEmpty()) return true;

    QString chosenPath;
    if (!customPath.isEmpty() && QFile::exists(customPath)) {
        chosenPath = customPath;
    } else {
        const QString appDir = QCoreApplication::applicationDirPath();
        const QStringList candidates = {
            ":/config/boards/boards.json",
            "config/boards/boards.json",
            appDir + "/config/boards/boards.json",
            appDir + "/../config/boards/boards.json",
            QDir::current().filePath("config/boards/boards.json"),
            "/home/cherry/embedded-ui-designer/config/boards/boards.json"
        };
        for (const QString& c : candidates) {
            if (QFile::exists(c)) {
                chosenPath = c;
                break;
            }
        }
    }

    if (chosenPath.isEmpty()) {
        qWarning() << "[BoardRegistry] Could not find config/boards/boards.json";
        return false;
    }

    QFile file(chosenPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[BoardRegistry] Failed to open:" << chosenPath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[BoardRegistry] JSON parse error:" << err.errorString();
        return false;
    }

    m_boards.clear();
    m_boardMap.clear();
    QSet<QString> famSet;

    QJsonArray boardsArr = doc.object().value("boards").toArray();
    for (const auto& val : boardsArr) {
        if (!val.isObject()) continue;
        BoardDef b = BoardDef::fromJson(val.toObject());
        if (b.id.isEmpty()) continue;

        ensureFamilyPinDefaults(b);

        m_boards.append(b);
        m_boardMap[b.id] = b;
        if (!b.family.isEmpty()) famSet.insert(b.family);
    }

    m_families = famSet.values();
    m_families.sort();
    m_loaded = true;

    if (!m_boards.isEmpty() && m_activeBoard.id.isEmpty()) {
        m_activeBoard = m_boards.first();
    }

    return true;
}

void BoardRegistry::ensureFamilyPinDefaults(BoardDef& b) {
    if (b.pinMap.isEmpty()) {
        if (b.family == "STM32") {
            const QStringList ports = {"PA", "PB", "PC", "PD", "PE", "PF", "PG", "PH"};
            for (const QString& port : ports) {
                for (int i = 0; i <= 15; ++i) {
                    QString pinName = QString("%1%2").arg(port).arg(i);
                    PinDef p;
                    p.function = "GPIO";
                    if (pinName == "PA13") {
                        p.reserved = true;
                        p.note = "SWDIO (Debug)";
                    } else if (pinName == "PA14") {
                        p.reserved = true;
                        p.note = "SWCLK (Debug)";
                    }
                    b.pinMap[pinName] = p;
                }
            }
        } else if (b.family == "ESP32") {
            for (int i = 0; i <= 48; ++i) {
                QString pinName = QString("GPIO%1").arg(i);
                PinDef p;
                p.function = "GPIO";
                if (i == 0 || i == 2 || i == 12 || i == 15) {
                    p.strapping = true;
                    p.note = "Strapping Pin";
                } else if (i >= 6 && i <= 11) {
                    p.reserved = true;
                    p.note = "Integrated Flash";
                }
                b.pinMap[pinName] = p;
            }
        } else if (b.family == "RP2040") {
            for (int i = 0; i <= 29; ++i) {
                QString pinName = QString("GP%1").arg(i);
                PinDef p;
                p.function = "GPIO";
                b.pinMap[pinName] = p;
            }
        } else if (b.family == "RaspberryPi") {
            static const QMap<int, int> bcmToPhys = {
                {2, 3}, {3, 5}, {4, 7}, {14, 8}, {15, 10}, {17, 11}, {18, 12}, {27, 13},
                {22, 15}, {23, 16}, {24, 18}, {10, 19}, {9, 21}, {25, 22}, {11, 23}, {8, 24},
                {7, 26}, {0, 27}, {1, 28}, {5, 29}, {6, 31}, {12, 32}, {13, 33}, {19, 35},
                {16, 36}, {26, 37}, {20, 38}, {21, 40}
            };
            for (int i = 2; i <= 27; ++i) {
                QString pinName = QString("GPIO%1").arg(i);
                PinDef p;
                p.function = "GPIO";
                int phys = bcmToPhys.value(i, 0);
                if (phys > 0) {
                    p.note = QString("BCM %1 (Physical Pin %2)").arg(i).arg(phys);
                } else {
                    p.note = QString("BCM %1").arg(i);
                }
                b.pinMap[pinName] = p;
            }
        } else if (b.family == "NXP") {
            for (int i = 0; i <= 15; ++i) {
                QString pinName = QString("GPIO_AD_B0_%1").arg(i, 2, 10, QChar('0'));
                PinDef p;
                p.function = "GPIO";
                b.pinMap[pinName] = p;
            }
        }
    } else {
        // Ensure strapping and reserved flags are correctly stamped if missing
        if (b.family == "ESP32") {
            for (auto it = b.pinMap.begin(); it != b.pinMap.end(); ++it) {
                QString key = it.key();
                if (key == "GPIO0" || key == "GPIO2" || key == "GPIO12" || key == "GPIO15") {
                    it.value().strapping = true;
                    if (it.value().note.isEmpty()) it.value().note = "Strapping Pin";
                } else if (key == "GPIO6" || key == "GPIO7" || key == "GPIO8" || key == "GPIO9" || key == "GPIO10" || key == "GPIO11") {
                    it.value().reserved = true;
                    if (it.value().note.isEmpty()) it.value().note = "Integrated SPI Flash";
                }
            }
        } else if (b.family == "STM32") {
            if (b.pinMap.contains("PA13")) {
                b.pinMap["PA13"].reserved = true;
                if (b.pinMap["PA13"].note.isEmpty()) b.pinMap["PA13"].note = "SWDIO (Debug)";
            }
            if (b.pinMap.contains("PA14")) {
                b.pinMap["PA14"].reserved = true;
                if (b.pinMap["PA14"].note.isEmpty()) b.pinMap["PA14"].note = "SWCLK (Debug)";
            }
        }
    }
}

QList<BoardDef> BoardRegistry::boards() const {
    return m_boards;
}

BoardDef BoardRegistry::board(const QString& id) const {
    return m_boardMap.value(id);
}

QStringList BoardRegistry::families() const {
    return m_families;
}

QList<BoardDef> BoardRegistry::boardsForFamily(const QString& family) const {
    QList<BoardDef> list;
    for (const auto& b : m_boards) {
        if (b.family.compare(family, Qt::CaseInsensitive) == 0) {
            list.append(b);
        }
    }
    return list;
}

BoardDef BoardRegistry::activeBoard() const {
    BoardDef b = m_activeBoard;
    // Apply preset overrides if available
    if (!m_activePreset.name.isEmpty()) {
        for (auto it = m_activePreset.pinOverrides.begin(); it != m_activePreset.pinOverrides.end(); ++it) {
            b.pinMap[it.key()] = it.value();
        }
        for (auto it = m_activePreset.protocolOverrides.begin(); it != m_activePreset.protocolOverrides.end(); ++it) {
            b.protocols[it.key()] = it.value();
        }
    }
    return b;
}

void BoardRegistry::setActiveBoard(const QString& id) {
    if (m_boardMap.contains(id)) {
        setActiveBoard(m_boardMap.value(id));
    }
}

void BoardRegistry::setActiveBoard(const BoardDef& b) {
    m_activeBoard = b;
    if (m_activePreset.boardId != b.id) {
        m_activePreset = BoardPreset();
    }
    savePresets();
    emit activeBoardChanged(activeBoard());
}

bool BoardRegistry::hasPresets() const {
    return QFile::exists(presetsFilePath()) && (!m_presets.isEmpty() || !m_activePreset.name.isEmpty());
}

QList<BoardPreset> BoardRegistry::presets() const {
    return m_presets;
}

BoardPreset BoardRegistry::activePreset() const {
    return m_activePreset;
}

bool BoardRegistry::savePreset(const QString& name, const QString& boardId,
                              const QMap<QString, PinDef>& pinOverrides,
                              const QMap<QString, QStringList>& protocolOverrides) {
    if (name.isEmpty() || boardId.isEmpty()) return false;

    BoardPreset p;
    p.name = name;
    p.boardId = boardId;
    p.pinOverrides = pinOverrides;
    p.protocolOverrides = protocolOverrides;
    p.isCustom = m_boardMap.value(boardId).family == "Custom";

    // Replace if exists, or append
    int idx = -1;
    for (int i = 0; i < m_presets.size(); ++i) {
        if (m_presets[i].name == name) {
            idx = i;
            break;
        }
    }

    if (idx >= 0) {
        m_presets[idx] = p;
    } else {
        m_presets.append(p);
    }

    m_activePreset = p;
    if (m_boardMap.contains(boardId)) {
        m_activeBoard = m_boardMap.value(boardId);
    }
    m_onboardingCompleted = true;

    savePresets();
    emit presetsChanged();
    emit activeBoardChanged(activeBoard());
    return true;
}

bool BoardRegistry::deletePreset(const QString& name) {
    int idx = -1;
    for (int i = 0; i < m_presets.size(); ++i) {
        if (m_presets[i].name == name) {
            idx = i;
            break;
        }
    }

    if (idx < 0) return false;

    m_presets.removeAt(idx);
    if (m_activePreset.name == name) {
        if (!m_presets.isEmpty()) {
            selectPreset(m_presets.first().name);
        } else {
            m_activePreset = BoardPreset();
        }
    }

    savePresets();
    emit presetsChanged();
    return true;
}

bool BoardRegistry::selectPreset(const QString& name) {
    for (const auto& p : m_presets) {
        if (p.name == name) {
            m_activePreset = p;
            if (m_boardMap.contains(p.boardId)) {
                m_activeBoard = m_boardMap.value(p.boardId);
            }
            savePresets();
            emit activeBoardChanged(activeBoard());
            return true;
        }
    }
    return false;
}

bool BoardRegistry::addCustomBoard(const BoardDef& customBoard) {
    if (customBoard.id.isEmpty()) return false;

    BoardDef b = customBoard;
    b.family = "Custom";
    ensureFamilyPinDefaults(b);

    // Replace or add
    m_customBoards.removeAll(b);
    m_customBoards.append(b);
    m_boardMap[b.id] = b;

    int existingIdx = -1;
    for (int i = 0; i < m_boards.size(); ++i) {
        if (m_boards[i].id == b.id) {
            existingIdx = i;
            break;
        }
    }
    if (existingIdx >= 0) {
        m_boards[existingIdx] = b;
    } else {
        m_boards.append(b);
    }

    if (!m_families.contains("Custom")) {
        m_families.append("Custom");
        m_families.sort();
    }

    savePresets();
    return true;
}

bool BoardRegistry::deleteCustomBoard(const QString& id) {
    if (!m_boardMap.contains(id)) return false;

    for (int i = 0; i < m_customBoards.size(); ++i) {
        if (m_customBoards[i].id == id) {
            m_customBoards.removeAt(i);
            break;
        }
    }

    for (int i = 0; i < m_boards.size(); ++i) {
        if (m_boards[i].id == id) {
            m_boards.removeAt(i);
            break;
        }
    }
    m_boardMap.remove(id);

    savePresets();
    return true;
}

void BoardRegistry::loadPresets() {
    QString path = presetsFilePath();
    if (!QFile::exists(path)) {
        m_onboardingCompleted = false;
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;
    QJsonObject root = doc.object();

    m_onboardingCompleted = root.value("onboardingCompleted").toBool(true);

    // Load custom boards first
    if (root.contains("customBoards") && root.value("customBoards").isArray()) {
        for (const auto& v : root.value("customBoards").toArray()) {
            if (v.isObject()) {
                BoardDef cb = BoardDef::fromJson(v.toObject());
                addCustomBoard(cb);
            }
        }
    }

    // Load presets
    m_presets.clear();
    if (root.contains("presets") && root.value("presets").isArray()) {
        for (const auto& v : root.value("presets").toArray()) {
            if (v.isObject()) {
                m_presets.append(BoardPreset::fromJson(v.toObject()));
            }
        }
    }

    QString lastPresetName = root.value("lastUsedPreset").toString();
    QString activeBoardId = root.value("activeBoardId").toString();

    if (!lastPresetName.isEmpty()) {
        selectPreset(lastPresetName);
    } else if (!activeBoardId.isEmpty() && m_boardMap.contains(activeBoardId)) {
        m_activeBoard = m_boardMap.value(activeBoardId);
    } else if (!m_presets.isEmpty()) {
        selectPreset(m_presets.first().name);
    }
}

void BoardRegistry::savePresets() {
    QJsonObject root;
    root["onboardingCompleted"] = m_onboardingCompleted;
    root["lastUsedPreset"] = m_activePreset.name;
    root["activeBoardId"] = m_activeBoard.id;

    QJsonArray presetArr;
    for (const auto& p : m_presets) {
        presetArr.append(p.toJson());
    }
    root["presets"] = presetArr;

    QJsonArray customArr;
    for (const auto& cb : m_customBoards) {
        customArr.append(cb.toJson());
    }
    root["customBoards"] = customArr;

    QFile file(presetsFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

bool BoardRegistry::isOnboardingCompleted() const {
    return m_onboardingCompleted && (hasPresets() || !m_activeBoard.id.isEmpty());
}

void BoardRegistry::setOnboardingCompleted(bool completed) {
    m_onboardingCompleted = completed;
    savePresets();
}
