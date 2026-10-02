#pragma once

#include <QString>
#include <QColor>
#include <QJsonObject>

struct ColorStyle {
    QString name;
    QColor color;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["hex"] = color.name(QColor::HexRgb);
        return obj;
    }

    static ColorStyle fromJson(const QJsonObject& obj) {
        ColorStyle cs;
        cs.name = obj.value("name").toString();
        cs.color = QColor(obj.value("hex").toString("#000000"));
        return cs;
    }
};
