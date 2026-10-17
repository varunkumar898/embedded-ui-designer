#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>

struct PinDef {
    QString function = "GPIO"; // "GPIO" | "PWM" | "ADC" | "SPI" | "I2C" | "UART" | "POWER" | "GND"
    QStringList alternates;
    bool reserved = false;
    bool strapping = false;
    QString note;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["function"] = function;
        QJsonArray altArr;
        for (const QString& a : alternates) altArr.append(a);
        obj["alternates"] = altArr;
        obj["reserved"] = reserved;
        obj["strapping"] = strapping;
        obj["note"] = note;
        return obj;
    }

    static PinDef fromJson(const QJsonObject& obj) {
        PinDef p;
        p.function = obj.value("function").toString("GPIO");
        const QJsonArray altArr = obj.value("alternates").toArray();
        for (const auto& v : altArr) p.alternates.append(v.toString());
        p.reserved = obj.value("reserved").toBool(false);
        p.strapping = obj.value("strapping").toBool(false);
        p.note = obj.value("note").toString();
        return p;
    }
};

struct DisplayDef {
    int width = 320;
    int height = 240;
    int colorDepth = 16;
    QString interface = "SPI";

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["width"] = width;
        obj["height"] = height;
        obj["colorDepth"] = colorDepth;
        obj["interface"] = interface;
        return obj;
    }

    static DisplayDef fromJson(const QJsonObject& obj) {
        DisplayDef d;
        d.width = obj.value("width").toInt(320);
        d.height = obj.value("height").toInt(240);
        d.colorDepth = obj.value("colorDepth").toInt(16);
        d.interface = obj.value("interface").toString("SPI");
        return d;
    }
};

struct BoardDef {
    QString id;
    QString displayName;
    QString vendor;
    QString family; // "STM32", "ESP32", "RP2040", "NXP", "RaspberryPi", "Custom"
    DisplayDef display;
    QMap<QString, PinDef> pinMap;
    QMap<QString, QStringList> protocols;
    QString openocdConfig;
    QString flashCommand;
    qint64 flashBytes = 0;
    qint64 ramBytes = 0;
    int clockMhz = 0;
    QString architecture;
    QString mcu;

    bool isValid() const { return !id.isEmpty(); }
    bool operator==(const BoardDef& other) const { return id == other.id; }

    QStringList pinNames() const {
        return pinMap.keys();
    }

    QJsonObject toJson() const;
    static BoardDef fromJson(const QJsonObject& obj);
};

struct BoardPreset {
    QString name;
    QString boardId;
    bool isCustom = false;
    QMap<QString, PinDef> pinOverrides;
    QMap<QString, QStringList> protocolOverrides;

    QJsonObject toJson() const;
    static BoardPreset fromJson(const QJsonObject& obj);
};

class BoardRegistry : public QObject {
    Q_OBJECT
public:
    static BoardRegistry& instance();

    bool loadFromBoardsJson(const QString& path = QString());
    
    QList<BoardDef> boards() const;
    BoardDef board(const QString& id) const;
    QStringList families() const;
    QList<BoardDef> boardsForFamily(const QString& family) const;

    BoardDef activeBoard() const;
    void setActiveBoard(const QString& id);
    void setActiveBoard(const BoardDef& b);

    // Presets
    bool hasPresets() const;
    QList<BoardPreset> presets() const;
    BoardPreset activePreset() const;
    bool savePreset(const QString& name, const QString& boardId,
                    const QMap<QString, PinDef>& pinOverrides = {},
                    const QMap<QString, QStringList>& protocolOverrides = {});
    bool deletePreset(const QString& name);
    bool selectPreset(const QString& name);

    // Custom boards
    bool addCustomBoard(const BoardDef& customBoard);
    bool deleteCustomBoard(const QString& id);

    void loadPresets();
    void savePresets();

    bool isOnboardingCompleted() const;
    void setOnboardingCompleted(bool completed);
    void ensureFamilyPinDefaults(BoardDef& b);

signals:
    void activeBoardChanged(const BoardDef& board);
    void presetsChanged();

private:
    BoardRegistry();
    ~BoardRegistry() override = default;
    BoardRegistry(const BoardRegistry&) = delete;
    BoardRegistry& operator=(const BoardRegistry&) = delete;

    QString presetsFilePath() const;

    QList<BoardDef> m_boards;
    QMap<QString, BoardDef> m_boardMap;
    QStringList m_families;
    BoardDef m_activeBoard;
    BoardPreset m_activePreset;
    QList<BoardPreset> m_presets;
    QList<BoardDef> m_customBoards;
    bool m_onboardingCompleted = false;
    bool m_loaded = false;
};
