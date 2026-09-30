#pragma once

#include <QString>
#include <QColor>
#include <QRectF>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

struct PrimitiveShapeData {
    QString type; // "Rectangle", "Circle", "Text"
    QRectF relativeRect;
    QJsonObject properties;
    bool isThumb = false;
    bool isTrack = false;
    bool isFill = false;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["type"] = type;
        obj["x"] = relativeRect.x();
        obj["y"] = relativeRect.y();
        obj["width"] = relativeRect.width();
        obj["height"] = relativeRect.height();
        obj["properties"] = properties;
        obj["isThumb"] = isThumb;
        obj["isTrack"] = isTrack;
        obj["isFill"] = isFill;
        return obj;
    }

    static PrimitiveShapeData fromJson(const QJsonObject& obj) {
        PrimitiveShapeData data;
        data.type = obj.value("type").toString("Rectangle");
        data.relativeRect = QRectF(
            obj.value("x").toDouble(0.0),
            obj.value("y").toDouble(0.0),
            obj.value("width").toDouble(40.0),
            obj.value("height").toDouble(40.0)
        );
        data.properties = obj.value("properties").toObject();
        data.isThumb = obj.value("isThumb").toBool(false);
        data.isTrack = obj.value("isTrack").toBool(false);
        data.isFill = obj.value("isFill").toBool(false);
        return data;
    }
};

class CustomComponentDefinition {
public:
    QString id;
    QString name = "CustomComponent";
    QString behaviorRole = "None"; // "None", "Slider", "ProgressBar", "Button"
    qreal width = 160.0;
    qreal height = 40.0;
    double minValue = 0.0;
    double maxValue = 100.0;
    double defaultValue = 50.0;
    QList<PrimitiveShapeData> primitives;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["behaviorRole"] = behaviorRole;
        obj["width"] = width;
        obj["height"] = height;
        obj["minValue"] = minValue;
        obj["maxValue"] = maxValue;
        obj["defaultValue"] = defaultValue;

        QJsonArray primsArray;
        for (const auto& p : primitives) {
            primsArray.append(p.toJson());
        }
        obj["primitives"] = primsArray;
        return obj;
    }

    static CustomComponentDefinition fromJson(const QJsonObject& obj) {
        CustomComponentDefinition def;
        def.id = obj.value("id").toString();
        def.name = obj.value("name").toString("CustomComponent");
        def.behaviorRole = obj.value("behaviorRole").toString("None");
        def.width = obj.value("width").toDouble(160.0);
        def.height = obj.value("height").toDouble(40.0);
        def.minValue = obj.value("minValue").toDouble(0.0);
        def.maxValue = obj.value("maxValue").toDouble(100.0);
        def.defaultValue = obj.value("defaultValue").toDouble(50.0);

        if (obj.contains("primitives") && obj.value("primitives").isArray()) {
            QJsonArray arr = obj.value("primitives").toArray();
            for (const auto& item : arr) {
                def.primitives.append(PrimitiveShapeData::fromJson(item.toObject()));
            }
        }
        return def;
    }
};
