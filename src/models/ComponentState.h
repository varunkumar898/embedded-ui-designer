#pragma once

#include <QString>
#include <QColor>
#include <QJsonObject>
#include <QMap>

enum class ComponentStateEnum {
    Normal,
    Warning,
    Error,
    Disabled,
    Active,
    Focused,
    Pressed,
    Selected,
    Loading,
    Custom
};

using ComponentState = ComponentStateEnum;

QString componentStateToString(ComponentStateEnum state);
ComponentStateEnum stringToComponentState(const QString& str);

struct ComponentStateStyle {
    bool hasBackgroundColor = false;
    QColor backgroundColor;

    bool hasTextColor = false;
    QColor textColor;

    bool hasBorderColor = false;
    QColor borderColor;

    bool hasBorderWidth = false;
    int borderWidth = 0;

    bool hasOpacity = false;
    qreal opacity = 1.0;

    bool hasVisible = false;
    bool visible = true;

    bool hasEnabled = false;
    bool enabled = true;

    QJsonObject customProperties;

    QJsonObject toJson() const;
    static ComponentStateStyle fromJson(const QJsonObject& json);
    bool isEmpty() const;
    bool operator==(const ComponentStateStyle& other) const;
};
