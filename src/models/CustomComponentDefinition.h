#pragma once

#include <QString>
#include <QColor>
#include <QRectF>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

struct PrimitiveShapeData {
    QString shapeType = "Rectangle"; // "Rectangle", "Circle", "Text"
    QString role = "content";        // "track", "thumb", "fill", "content"
    qreal relX = 0;
    qreal relY = 0;
    qreal relWidth = 40;
    qreal relHeight = 40;
    QColor fillColor = QColor("#2196F3");
    QColor strokeColor = QColor("#FFFFFF");
    int strokeWidth = 0;
    int cornerRadius = 0;

    // Compatibility fields
    QString type = "Rectangle";
    QRectF relativeRect;
    QJsonObject properties;
    bool isThumb = false;
    bool isTrack = false;
    bool isFill = false;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["shapeType"] = shapeType;
        obj["type"] = shapeType;
        obj["role"] = role;
        obj["relX"] = relX;
        obj["relY"] = relY;
        obj["relWidth"] = relWidth;
        obj["relHeight"] = relHeight;
        obj["x"] = relX;
        obj["y"] = relY;
        obj["width"] = relWidth;
        obj["height"] = relHeight;
        obj["fillColor"] = fillColor.name(QColor::HexArgb);
        obj["strokeColor"] = strokeColor.name(QColor::HexArgb);
        obj["strokeWidth"] = strokeWidth;
        obj["cornerRadius"] = cornerRadius;
        obj["isThumb"] = (role == "thumb");
        obj["isTrack"] = (role == "track");
        obj["isFill"] = (role == "fill");
        return obj;
    }

    static PrimitiveShapeData fromJson(const QJsonObject& obj) {
        PrimitiveShapeData data;
        data.shapeType = obj.contains("shapeType") ? obj.value("shapeType").toString() : obj.value("type").toString("Rectangle");
        data.type = data.shapeType;
        data.role = obj.contains("role") ? obj.value("role").toString() : (obj.value("isThumb").toBool() ? "thumb" : (obj.value("isTrack").toBool() ? "track" : "content"));
        data.relX = obj.contains("relX") ? obj.value("relX").toDouble(0.0) : obj.value("x").toDouble(0.0);
        data.relY = obj.contains("relY") ? obj.value("relY").toDouble(0.0) : obj.value("y").toDouble(0.0);
        data.relWidth = obj.contains("relWidth") ? obj.value("relWidth").toDouble(40.0) : obj.value("width").toDouble(40.0);
        data.relHeight = obj.contains("relHeight") ? obj.value("relHeight").toDouble(40.0) : obj.value("height").toDouble(40.0);
        data.relativeRect = QRectF(data.relX, data.relY, data.relWidth, data.relHeight);

        if (obj.contains("fillColor")) data.fillColor = QColor(obj.value("fillColor").toString());
        if (obj.contains("strokeColor")) data.strokeColor = QColor(obj.value("strokeColor").toString());
        data.strokeWidth = obj.value("strokeWidth").toInt(0);
        data.cornerRadius = obj.value("cornerRadius").toInt(0);
        data.isThumb = (data.role == "thumb");
        data.isTrack = (data.role == "track");
        data.isFill = (data.role == "fill");
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
