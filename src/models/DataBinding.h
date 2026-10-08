#pragma once

#include <QString>
#include <QJsonObject>

enum class BindingDirection {
    Read,        // Data Source -> UI Component
    Write,       // UI Component -> Data Source
    ReadWrite    // Bi-directional
};

QString bindingDirectionToString(BindingDirection dir);
BindingDirection stringToBindingDirection(const QString& str);

class DataBinding {
public:
    DataBinding() = default;
    DataBinding(const QString& componentId, const QString& propertyName,
                const QString& sourceId, BindingDirection direction = BindingDirection::Read);

    QString componentId() const { return m_componentId; }
    void setComponentId(const QString& id) { m_componentId = id; }

    QString propertyName() const { return m_propertyName; }
    void setPropertyName(const QString& prop) { m_propertyName = prop; }

    QString sourceId() const { return m_sourceId; }
    void setSourceId(const QString& src) { m_sourceId = src; }

    BindingDirection direction() const { return m_direction; }
    void setDirection(BindingDirection dir) { m_direction = dir; }

    QString transformExpression() const { return m_transform; }
    void setTransformExpression(const QString& expr) { m_transform = expr; }

    QJsonObject toJson() const;
    static DataBinding fromJson(const QJsonObject& json);

    bool operator==(const DataBinding& other) const;

private:
    QString m_componentId;
    QString m_propertyName;
    QString m_sourceId;
    BindingDirection m_direction = BindingDirection::Read;
    QString m_transform;
};
