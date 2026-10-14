#pragma once

#include <QString>
#include <QJsonObject>

struct DisplayConfig {
    int width = 320;
    int height = 240;
    int colorDepth = 16;       // 1, 8, 16, 24-bit
    QString colorFormat = "RGB565";
    bool doubleBuffered = false;
    QString type = "LCD";      // LCD, OLED, E-Ink
    int dpi = 96;
    // Code generation for round displays is not solved here; this only changes the canvas boundary.
    bool round = false;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["width"] = width;
        obj["height"] = height;
        obj["colorDepth"] = colorDepth;
        obj["colorFormat"] = colorFormat;
        obj["doubleBuffered"] = doubleBuffered;
        obj["type"] = type;
        obj["dpi"] = dpi;
        obj["round"] = round;
        return obj;
    }

    static DisplayConfig fromJson(const QJsonObject& obj) {
        DisplayConfig config;
        config.width = obj.value("width").toInt(320);
        config.height = obj.value("height").toInt(240);
        config.colorDepth = obj.value("colorDepth").toInt(16);
        config.colorFormat = obj.value("colorFormat").toString("RGB565");
        config.doubleBuffered = obj.value("doubleBuffered").toBool(false);
        config.type = obj.value("type").toString("LCD");
        config.dpi = obj.value("dpi").toInt(96);
        config.round = obj.value("round").toBool(false);
        return config;
    }
};
