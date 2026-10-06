#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QColor>

class UIComponent;

struct PropertyDefinition {
    QString name;
    QString type;           // "string", "int", "double", "bool", "color", "enum"
    QJsonValue defaultValue;
    bool editable = true;
    double minimum = 0.0;
    double maximum = 0.0;
    bool hasRange = false;
    QStringList enumValues;
    QString description;

    QJsonObject toJson() const;
    static PropertyDefinition fromJson(const QJsonObject& json);
};

struct ComponentTypeSchema {
    QString type;           // Canonical type id, e.g. "progress_bar"
    QString className;      // C++ class name, e.g. "ProgressBarComponent"
    QString displayName;    // Human-readable, e.g. "Progress Bar"
    QString category;       // e.g. "Controls", "Display", "Shapes", "Input", "Media"
    QList<PropertyDefinition> properties;
    QJsonObject defaultProperties;
    QStringList capabilities; // e.g. ["resize", "move", "colors", "styles", "interactive_radius"]

    QJsonObject toJson() const;
    const PropertyDefinition* findProperty(const QString& name) const;
};

class ComponentSchemaRegistry {
public:
    static ComponentSchemaRegistry& instance();

    const QList<ComponentTypeSchema>& allSchemas() const;
    const ComponentTypeSchema* findSchema(const QString& typeName) const;
    QString normalizeTypeName(const QString& typeName) const;
    QStringList availableTypes() const;

    QJsonObject fullSchemaJson() const;

    // Factory method to construct component instance by canonical or alias type
    UIComponent* createComponent(const QString& typeName, const QString& id = QString()) const;

    // Inspection: extracts component-specific properties as a clean QJsonObject
    QJsonObject getComponentProperties(const UIComponent* comp) const;

    // Mutation: sets a specific property dynamically with type and range checking
    bool setComponentProperty(UIComponent* comp, const QString& propName, const QJsonValue& val, QString* error = nullptr) const;

    // Batch mutation: sets multiple properties
    bool setComponentProperties(UIComponent* comp, const QJsonObject& props, QString* error = nullptr) const;

private:
    ComponentSchemaRegistry();
    void registerAllBuiltins();

    QList<ComponentTypeSchema> m_schemas;
    QMap<QString, int> m_typeMap;
};
