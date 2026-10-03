#pragma once

#include <QString>
#include <QColor>
#include <QJsonObject>

/// Named Color Style
///
/// A project-scoped, named color that components can reference by name
/// (see UIComponent::setColorStyleRef). When the style's color changes,
/// Project::updateColorStyle immediately re-applies it to every component
/// that references it — no reload required.
struct ColorStyle {
    QString name;
    QColor  color;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["hex"]  = color.name(QColor::HexRgb);
        return obj;
    }

    static ColorStyle fromJson(const QJsonObject& obj) {
        ColorStyle cs;
        cs.name  = obj.value("name").toString();
        cs.color = QColor(obj.value("hex").toString("#000000"));
        return cs;
    }
};
