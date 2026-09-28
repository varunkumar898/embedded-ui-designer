#include "WidgetModel.h"

WidgetModel::WidgetModel(QObject* parent)
    : QObject(parent)
{
}

WidgetModel::WidgetModel(const QString& id, const QString& type, qreal x, qreal y, qreal width, qreal height, const QColor& color, QObject* parent)
    : QObject(parent)
    , m_id(id)
    , m_type(type)
    , m_x(x)
    , m_y(y)
    , m_width(width)
    , m_height(height)
    , m_color(color)
{
}

void WidgetModel::setId(const QString& id) {
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
    }
}

void WidgetModel::setType(const QString& type) {
    if (m_type != type) {
        m_type = type;
        emit typeChanged(m_type);
    }
}

void WidgetModel::setX(qreal x) {
    if (m_x != x) {
        m_x = x;
        emit xChanged(m_x);
        emit geometryChanged();
    }
}

void WidgetModel::setY(qreal y) {
    if (m_y != y) {
        m_y = y;
        emit yChanged(m_y);
        emit geometryChanged();
    }
}

void WidgetModel::setWidth(qreal width) {
    if (m_width != width) {
        m_width = width;
        emit widthChanged(m_width);
        emit geometryChanged();
    }
}

void WidgetModel::setHeight(qreal height) {
    if (m_height != height) {
        m_height = height;
        emit heightChanged(m_height);
        emit geometryChanged();
    }
}

void WidgetModel::setColor(const QColor& color) {
    if (m_color != color) {
        m_color = color;
        emit colorChanged(m_color);
    }
}

void WidgetModel::setText(const QString& text) {
    if (m_text != text) {
        m_text = text;
        emit textChanged(m_text);
    }
}

void WidgetModel::setTargetScreenId(const QString& targetScreenId) {
    if (m_targetScreenId != targetScreenId) {
        m_targetScreenId = targetScreenId;
        emit targetScreenIdChanged(m_targetScreenId);
    }
}

void WidgetModel::setImagePath(const QString& path) {
    if (m_imagePath != path) {
        m_imagePath = path;
        emit imagePathChanged(m_imagePath);
    }
}

void WidgetModel::setCustomProperties(const QVariantMap& props) {
    m_customProperties = props;
    emit customPropertiesChanged();
}

void WidgetModel::setGeometry(qreal x, qreal y, qreal width, qreal height) {
    bool moved = (m_x != x || m_y != y);
    bool resized = (m_width != width || m_height != height);

    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;

    if (moved) {
        emit xChanged(m_x);
        emit yChanged(m_y);
    }
    if (resized) {
        emit widthChanged(m_width);
        emit heightChanged(m_height);
    }
    if (moved || resized) {
        emit geometryChanged();
    }
}

void WidgetModel::setCustomProperty(const QString& key, const QVariant& value) {
    if (m_customProperties.value(key) != value) {
        m_customProperties.insert(key, value);
        emit customPropertiesChanged();
    }
}

QVariant WidgetModel::getCustomProperty(const QString& key, const QVariant& defaultValue) const {
    return m_customProperties.value(key, defaultValue);
}

QJsonObject WidgetModel::toJson() const {
    QJsonObject obj;
    obj["id"] = m_id;
    obj["type"] = m_type;
    obj["x"] = m_x;
    obj["y"] = m_y;
    obj["width"] = m_width;
    obj["height"] = m_height;
    obj["color"] = m_color.name();
    obj["text"] = m_text;

    if (!m_targetScreenId.isEmpty()) {
        obj["targetScreenId"] = m_targetScreenId;
    }
    if (!m_imagePath.isEmpty()) {
        obj["imagePath"] = m_imagePath;
    }

    if (!m_customProperties.isEmpty()) {
        obj["custom"] = QJsonObject::fromVariantMap(m_customProperties);
    }

    return obj;
}

void WidgetModel::fromJson(const QJsonObject& json) {
    setId(json.value("id").toString(m_id));
    setType(json.value("type").toString(m_type));
    setX(json.value("x").toDouble(m_x));
    setY(json.value("y").toDouble(m_y));
    setWidth(json.value("width").toDouble(m_width));
    setHeight(json.value("height").toDouble(m_height));
    if (json.contains("color")) {
        setColor(QColor(json.value("color").toString()));
    }
    setText(json.value("text").toString(m_text));
    setTargetScreenId(json.value("targetScreenId").toString(m_targetScreenId));
    setImagePath(json.value("imagePath").toString(m_imagePath));

    if (json.contains("custom") && json.value("custom").isObject()) {
        setCustomProperties(json.value("custom").toObject().toVariantMap());
    }
}

WidgetModel* WidgetModel::clone(QObject* parent) const {
    auto w = new WidgetModel(m_id + "_copy", m_type, m_x, m_y, m_width, m_height, m_color, parent);
    w->setText(m_text);
    w->setTargetScreenId(m_targetScreenId);
    w->setImagePath(m_imagePath);
    w->setCustomProperties(m_customProperties);
    return w;
}
