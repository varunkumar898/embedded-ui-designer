#pragma once

#include "IRTypes.h"
#include <QString>
#include <QJsonObject>
#include <QColor>

namespace IR {

class IRSerialization {
public:
    // Project serialization
    static QString toJson(const IRProject& project);
    static QJsonObject toJsonObject(const IRProject& project);
    static IRProject fromJson(const QString& json, QString* outError = nullptr);
    static IRProject fromJsonObject(const QJsonObject& json, QString* outError = nullptr);

    // Screen serialization
    static QJsonObject screenToJsonObject(const IRScreen& screen);
    static IRScreen screenFromJsonObject(const QJsonObject& json, QString* outError = nullptr);

    // Widget / Component serialization
    static QJsonObject widgetToJsonObject(const IRWidget& widget);
    static IRWidget widgetFromJsonObject(const QJsonObject& json, QString* outError = nullptr);

    // Data Source serialization
    static QJsonObject dataSourceToJsonObject(const IRDataSource& ds);
    static IRDataSource dataSourceFromJsonObject(const QJsonObject& json, QString* outError = nullptr);

    // Data Binding serialization
    static QJsonObject bindingToJsonObject(const IRBinding& binding);
    static IRBinding bindingFromJsonObject(const QJsonObject& json, QString* outError = nullptr);

    // Animation serialization
    static QJsonObject animationToJsonObject(const IRAnimation& anim);
    static IRAnimation animationFromJsonObject(const QJsonObject& json, QString* outError = nullptr);

    // Color conversion helpers
    static QString colorToHex(const QColor& color);
    static QColor colorFromHex(const QString& hex);

    // Enum string conversions (lowercase)
    static QString layoutTypeToString(IRLayoutType type);
    static IRLayoutType stringToLayoutType(const QString& str);

    static QString flexDirectionToString(IRFlexDirection dir);
    static IRFlexDirection stringToFlexDirection(const QString& str);

    static QString justifyContentToString(IRJustifyContent jc);
    static IRJustifyContent stringToJustifyContent(const QString& str);

    static QString alignItemsToString(IRAlignItems ai);
    static IRAlignItems stringToAlignItems(const QString& str);

    static QString easingTypeToString(IREasingType type);
    static IREasingType stringToEasingType(const QString& str);

    static QString bindingDirectionToString(IRBindingDirection dir);
    static IRBindingDirection stringToBindingDirection(const QString& str);

    static QString dataSourceTypeToString(IRDataSourceType type);
    static IRDataSourceType stringToDataSourceType(const QString& str);
};

} // namespace IR
