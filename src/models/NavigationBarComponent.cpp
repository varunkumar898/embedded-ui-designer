#include "NavigationBarComponent.h"
#include <QPen>
#include <QBrush>
#include <algorithm>

NavigationBarComponent::NavigationBarComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "NavigationBar", parent)
{
    m_width = 300;
    m_height = 44;

    m_items = {
        {"Home", "home", "screen_main", true},
        {"Diagnostics", "gauge", "screen_diag", true},
        {"Settings", "gear", "screen_settings", true}
    };
}

void NavigationBarComponent::setItems(const QList<NavItem>& items) {
    if (items.isEmpty()) return;
    m_items = items;
    if (m_selectedIndex >= m_items.size()) {
        m_selectedIndex = m_items.size() - 1;
    }
    update();
    emit propertyChanged(this);
}

void NavigationBarComponent::addItem(const QString& label, const QString& targetScreenId, const QString& icon) {
    NavItem it;
    it.label = label.trimmed().isEmpty() ? QString("Nav %1").arg(m_items.size() + 1) : label;
    it.targetScreenId = targetScreenId;
    it.icon = icon;
    it.enabled = true;
    m_items.append(it);
    update();
    emit propertyChanged(this);
}

void NavigationBarComponent::removeItem(int index) {
    if (index >= 0 && index < m_items.size() && m_items.size() > 1) {
        m_items.removeAt(index);
        if (m_selectedIndex >= m_items.size()) {
            m_selectedIndex = m_items.size() - 1;
        }
        update();
        emit propertyChanged(this);
    }
}

void NavigationBarComponent::setItemLabel(int index, const QString& label) {
    if (index >= 0 && index < m_items.size() && !label.isEmpty()) {
        m_items[index].label = label;
        update();
        emit propertyChanged(this);
    }
}

void NavigationBarComponent::setItemTargetScreen(int index, const QString& screenId) {
    if (index >= 0 && index < m_items.size()) {
        m_items[index].targetScreenId = screenId;
        update();
        emit propertyChanged(this);
    }
}

void NavigationBarComponent::setSelectedIndex(int idx) {
    if (idx >= 0 && idx < m_items.size() && m_selectedIndex != idx) {
        m_selectedIndex = idx;
        update();
        emit propertyChanged(this);
        emit interactionTriggered("On Nav Selected");
        if (!m_items.at(idx).targetScreenId.isEmpty()) {
            emit navigateToScreenRequested(m_items.at(idx).targetScreenId);
        }
    }
}

void NavigationBarComponent::setOrientation(const QString& orient) {
    Qt::Orientation o = (orient.compare("Vertical", Qt::CaseInsensitive) == 0) ? Qt::Vertical : Qt::Horizontal;
    if (m_orientation != o) {
        m_orientation = o;
        update();
        emit propertyChanged(this);
    }
}

void NavigationBarComponent::setItemSpacing(int sp) {
    sp = std::clamp(sp, 0, 20);
    if (m_itemSpacing != sp) {
        m_itemSpacing = sp;
        update();
        emit propertyChanged(this);
    }
}

void NavigationBarComponent::setBarColor(const QColor& color) {
    if (m_barColor != color) {
        m_barColor = color;
        update();
        emit propertyChanged(this);
    }
}

void NavigationBarComponent::setActiveItemColor(const QColor& color) {
    if (m_activeItemColor != color) {
        m_activeItemColor = color;
        update();
        emit propertyChanged(this);
    }
}

void NavigationBarComponent::setActiveTextColor(const QColor& color) {
    if (m_activeTextColor != color) {
        m_activeTextColor = color;
        update();
        emit propertyChanged(this);
    }
}

void NavigationBarComponent::setInactiveTextColor(const QColor& color) {
    if (m_inactiveTextColor != color) {
        m_inactiveTextColor = color;
        update();
        emit propertyChanged(this);
    }
}

void NavigationBarComponent::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void NavigationBarComponent::setBorderWidth(int width) {
    width = std::max(0, width);
    if (m_borderWidth != width) {
        m_borderWidth = width;
        update();
        emit propertyChanged(this);
    }
}

void NavigationBarComponent::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !m_items.isEmpty()) {
        QPointF p = event->pos();
        bool isH = (m_orientation == Qt::Horizontal);
        int count = m_items.size();

        if (isH) {
            qreal itemW = m_width / count;
            int idx = static_cast<int>(p.x() / itemW);
            if (idx >= 0 && idx < count) {
                setSelectedIndex(idx);
                event->accept();
                return;
            }
        } else {
            qreal itemH = m_height / count;
            int idx = static_cast<int>(p.y() / itemH);
            if (idx >= 0 && idx < count) {
                setSelectedIndex(idx);
                event->accept();
                return;
            }
        }
    }
    UIComponent::mousePressEvent(event);
}

void NavigationBarComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF barRect(0, 0, m_width, m_height);

    // 1. Bar Background & Border
    painter->setPen(QPen(m_borderColor, m_borderWidth));
    painter->setBrush(QBrush(m_barColor));
    painter->drawRoundedRect(barRect, 8.0, 8.0);

    // 2. Navigation Items
    int count = m_items.size();
    if (count == 0) return;

    bool isH = (m_orientation == Qt::Horizontal);
    qreal itemW = isH ? (m_width - (count + 1) * m_itemSpacing) / count : (m_width - m_itemSpacing * 2);
    qreal itemH = isH ? (m_height - m_itemSpacing * 2) : (m_height - (count + 1) * m_itemSpacing) / count;

    for (int i = 0; i < count; ++i) {
        bool isActive = (i == m_selectedIndex);
        const NavItem& item = m_items.at(i);

        QRectF itemRect;
        if (isH) {
            itemRect = QRectF(m_itemSpacing + i * (itemW + m_itemSpacing), m_itemSpacing, itemW, itemH);
        } else {
            itemRect = QRectF(m_itemSpacing, m_itemSpacing + i * (itemH + m_itemSpacing), itemW, itemH);
        }

        // Active item background pill
        if (isActive) {
            painter->setPen(QPen(QColor(56, 189, 248, 80), 1.0));
            painter->setBrush(QBrush(m_activeItemColor));
            painter->drawRoundedRect(itemRect, 6.0, 6.0);

            // Active underline indicator
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(56, 189, 248));
            if (isH) {
                painter->drawRoundedRect(QRectF(itemRect.left() + itemW * 0.25, itemRect.bottom() - 2.5, itemW * 0.5, 2.0), 1.0, 1.0);
            } else {
                painter->drawRoundedRect(QRectF(itemRect.left() + 1.0, itemRect.top() + itemH * 0.2, 2.5, itemH * 0.6), 1.0, 1.0);
            }
        }

        // Item Label Text
        painter->setPen(isActive ? m_activeTextColor : m_inactiveTextColor);
        QFont f = painter->font();
        f.setPixelSize(std::clamp(static_cast<int>(itemH * 0.38), 9, 13));
        f.setBold(isActive);
        painter->setFont(f);
        painter->drawText(itemRect, Qt::AlignCenter, item.label);
    }
}

QJsonObject NavigationBarComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    QJsonArray itemsArr;
    for (const NavItem& it : m_items) {
        itemsArr.append(it.toJson());
    }
    obj["items"] = itemsArr;
    obj["selectedIndex"] = m_selectedIndex;
    obj["orientation"] = orientation();
    obj["itemSpacing"] = m_itemSpacing;
    obj["barColor"] = serializeColor(m_barColor, colorStyleRef("barColor"));
    obj["activeItemColor"] = serializeColor(m_activeItemColor, colorStyleRef("activeItemColor"));
    obj["activeTextColor"] = serializeColor(m_activeTextColor, colorStyleRef("activeTextColor"));
    obj["inactiveTextColor"] = serializeColor(m_inactiveTextColor, colorStyleRef("inactiveTextColor"));
    obj["borderColor"] = serializeColor(m_borderColor, colorStyleRef("borderColor"));
    obj["borderWidth"] = m_borderWidth;
    return obj;
}

void NavigationBarComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);

    if (json.contains("items") && json.value("items").isArray()) {
        m_items.clear();
        for (const QJsonValue& v : json.value("items").toArray()) {
            m_items.append(NavItem::fromJson(v.toObject()));
        }
        if (m_items.isEmpty()) {
            m_items = {
                {"Home", "home", "screen_main", true},
                {"Diagnostics", "gauge", "screen_diag", true},
                {"Settings", "gear", "screen_settings", true}
            };
        }
    }
    if (json.contains("selectedIndex")) m_selectedIndex = json.value("selectedIndex").toInt(0);
    if (json.contains("orientation")) setOrientation(json.value("orientation").toString("Horizontal"));
    if (json.contains("itemSpacing")) m_itemSpacing = json.value("itemSpacing").toInt(4);

    if (json.contains("barColor")) {
        QString ref;
        deserializeColor(json.value("barColor"), m_barColor, ref);
        setColorStyleRef("barColor", ref);
    }
    if (json.contains("activeItemColor")) {
        QString ref;
        deserializeColor(json.value("activeItemColor"), m_activeItemColor, ref);
        setColorStyleRef("activeItemColor", ref);
    }
    if (json.contains("activeTextColor")) {
        QString ref;
        deserializeColor(json.value("activeTextColor"), m_activeTextColor, ref);
        setColorStyleRef("activeTextColor", ref);
    }
    if (json.contains("inactiveTextColor")) {
        QString ref;
        deserializeColor(json.value("inactiveTextColor"), m_inactiveTextColor, ref);
        setColorStyleRef("inactiveTextColor", ref);
    }
    if (json.contains("borderColor")) {
        QString ref;
        deserializeColor(json.value("borderColor"), m_borderColor, ref);
        setColorStyleRef("borderColor", ref);
    }
    m_borderWidth = json.value("borderWidth").toInt(m_borderWidth);
    update();
}

QString NavigationBarComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1// NavigationBar: %2\n").arg(indent, m_id);
    qml += QString("%1Item {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(m_width).arg(m_height);
    qml += QString("%1    property int currentIndex: %2\n").arg(indent).arg(m_selectedIndex);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString NavigationBarComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// NavigationBar: %2 (%3 items)\n").arg(indent, m_id).arg(m_items.size());
    return code;
}
