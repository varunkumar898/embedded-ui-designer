#include "DataBinding.h"

QString bindingDirectionToString(BindingDirection dir) {
    switch (dir) {
        case BindingDirection::Read:      return "read";
        case BindingDirection::Write:     return "write";
        case BindingDirection::ReadWrite: return "read_write";
        default:                          return "read";
    }
}

BindingDirection stringToBindingDirection(const QString& str) {
    QString lower = str.trimmed().toLower();
    if (lower == "write" || lower == "out" || lower == "output") return BindingDirection::Write;
    if (lower == "read_write" || lower == "readwrite" || lower == "bidirectional" || lower == "both") return BindingDirection::ReadWrite;
    return BindingDirection::Read;
}

DataBinding::DataBinding(const QString& componentId, const QString& propertyName,
                         const QString& sourceId, BindingDirection direction)
    : m_componentId(componentId)
    , m_propertyName(propertyName)
    , m_sourceId(sourceId)
    , m_direction(direction)
{
}

QJsonObject DataBinding::toJson() const {
    QJsonObject obj;
    obj["componentId"] = m_componentId;
    obj["property"] = m_propertyName;
    obj["source"] = m_sourceId;
    obj["direction"] = bindingDirectionToString(m_direction);
    if (!m_transform.isEmpty()) {
        obj["transform"] = m_transform;
    }
    return obj;
}

DataBinding DataBinding::fromJson(const QJsonObject& json) {
    DataBinding b;
    b.m_componentId = json.value("componentId").toString();
    b.m_propertyName = json.value("property").toString(json.value("propertyName").toString());
    b.m_sourceId = json.value("source").toString(json.value("sourceId").toString());
    b.m_direction = stringToBindingDirection(json.value("direction").toString("read"));
    b.m_transform = json.value("transform").toString();
    return b;
}

bool DataBinding::operator==(const DataBinding& other) const {
    return m_componentId == other.m_componentId &&
           m_propertyName == other.m_propertyName &&
           m_sourceId == other.m_sourceId &&
           m_direction == other.m_direction &&
           m_transform == other.m_transform;
}
