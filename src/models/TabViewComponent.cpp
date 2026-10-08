#include "TabViewComponent.h"
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <algorithm>

TabViewComponent::TabViewComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "TabView", parent)
{
    m_width = 240;
    m_height = 160;
}

void TabViewComponent::setTabs(const QStringList& tabs) {
    if (tabs.isEmpty()) return;
    m_tabs = tabs;
    if (m_activeTabIndex >= m_tabs.size()) {
        m_activeTabIndex = m_tabs.size() - 1;
    }
    update();
    emit propertyChanged(this);
}

void TabViewComponent::addTab(const QString& title) {
    QString t = title.trimmed().isEmpty() ? QString("Tab %1").arg(m_tabs.size() + 1) : title;
    m_tabs.append(t);
    update();
    emit propertyChanged(this);
}

void TabViewComponent::removeTab(int index) {
    if (index >= 0 && index < m_tabs.size() && m_tabs.size() > 1) {
        m_tabs.removeAt(index);
        if (m_activeTabIndex >= m_tabs.size()) {
            m_activeTabIndex = m_tabs.size() - 1;
        }
        update();
        emit propertyChanged(this);
    }
}

void TabViewComponent::setTabTitle(int index, const QString& title) {
    if (index >= 0 && index < m_tabs.size() && !title.isEmpty()) {
        m_tabs[index] = title;
        update();
        emit propertyChanged(this);
    }
}

void TabViewComponent::setActiveTabIndex(int index) {
    if (index >= 0 && index < m_tabs.size() && m_activeTabIndex != index) {
        m_activeTabIndex = index;
        update();
        emit propertyChanged(this);
        emit interactionTriggered("On Tab Changed");
    }
}

void TabViewComponent::setTabHeight(int h) {
    h = std::clamp(h, 18, 60);
    if (m_tabHeight != h) {
        m_tabHeight = h;
        update();
        emit propertyChanged(this);
    }
}

void TabViewComponent::setTabPosition(const QString& pos) {
    QString p = pos.compare("Bottom", Qt::CaseInsensitive) == 0 ? "Bottom" : "Top";
    if (m_tabPosition != p) {
        m_tabPosition = p;
        update();
        emit propertyChanged(this);
    }
}

void TabViewComponent::setTabColor(const QColor& color) {
    if (m_tabColor != color) {
        m_tabColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TabViewComponent::setActiveTabColor(const QColor& color) {
    if (m_activeTabColor != color) {
        m_activeTabColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TabViewComponent::setActiveTabTextColor(const QColor& color) {
    if (m_activeTabTextColor != color) {
        m_activeTabTextColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TabViewComponent::setInactiveTabTextColor(const QColor& color) {
    if (m_inactiveTabTextColor != color) {
        m_inactiveTabTextColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TabViewComponent::setContentAreaColor(const QColor& color) {
    if (m_contentAreaColor != color) {
        m_contentAreaColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TabViewComponent::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TabViewComponent::setBorderWidth(int width) {
    width = std::max(0, width);
    if (m_borderWidth != width) {
        m_borderWidth = width;
        update();
        emit propertyChanged(this);
    }
}

void TabViewComponent::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !m_tabs.isEmpty()) {
        QPointF p = event->pos();
        bool isTop = (m_tabPosition == "Top");
        QRectF tabStripRect = isTop ? QRectF(0, 0, m_width, m_tabHeight)
                                    : QRectF(0, m_height - m_tabHeight, m_width, m_tabHeight);
        
        if (tabStripRect.contains(p)) {
            qreal tabW = m_width / m_tabs.size();
            int clickedIdx = static_cast<int>(p.x() / tabW);
            if (clickedIdx >= 0 && clickedIdx < m_tabs.size()) {
                setActiveTabIndex(clickedIdx);
                event->accept();
                return;
            }
        }
    }
    UIComponent::mousePressEvent(event);
}

void TabViewComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    bool isTop = (m_tabPosition == "Top");
    QRectF contentRect = isTop ? QRectF(0, m_tabHeight, m_width, m_height - m_tabHeight)
                               : QRectF(0, 0, m_width, m_height - m_tabHeight);

    // 1. Draw Content Area Background
    painter->setPen(QPen(m_borderColor, m_borderWidth));
    painter->setBrush(QBrush(m_contentAreaColor));
    painter->drawRoundedRect(contentRect, 4.0, 4.0);

    // 2. Draw Tabs
    if (!m_tabs.isEmpty()) {
        qreal tabW = m_width / m_tabs.size();
        qreal tabY = isTop ? 0 : (m_height - m_tabHeight);

        for (int i = 0; i < m_tabs.size(); ++i) {
            bool isActive = (i == m_activeTabIndex);
            QRectF tabRect(i * tabW, tabY, tabW, m_tabHeight);

            // Tab background
            painter->setPen(QPen(m_borderColor, m_borderWidth));
            painter->setBrush(QBrush(isActive ? m_activeTabColor : m_tabColor));
            painter->drawRoundedRect(tabRect.adjusted(1, 1, -1, 0), 4.0, 4.0);

            // Active indicator bar
            if (isActive) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(QColor(96, 165, 250)); // Light Blue
                if (isTop) {
                    painter->drawRect(QRectF(tabRect.left() + 4, tabRect.bottom() - 2, tabW - 8, 2));
                } else {
                    painter->drawRect(QRectF(tabRect.left() + 4, tabRect.top(), tabW - 8, 2));
                }
            }

            // Tab Text
            painter->setPen(isActive ? m_activeTabTextColor : m_inactiveTabTextColor);
            QFont f = painter->font();
            f.setPixelSize(std::clamp(static_cast<int>(m_tabHeight * 0.42), 9, 13));
            f.setBold(isActive);
            painter->setFont(f);
            painter->drawText(tabRect, Qt::AlignCenter, m_tabs.at(i));
        }
    }

    // 3. Content Mockup Preview inside Content Rect
    if (contentRect.height() >= 30) {
        painter->setPen(QColor(100, 116, 139));
        QFont f = painter->font();
        f.setPixelSize(11);
        f.setItalic(true);
        painter->setFont(f);
        QString activeName = (m_activeTabIndex >= 0 && m_activeTabIndex < m_tabs.size())
                             ? m_tabs.at(m_activeTabIndex) : "Content";
        painter->drawText(contentRect, Qt::AlignCenter, QString("[%1 Container Area]").arg(activeName));
    }
}

QJsonObject TabViewComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    QJsonArray tabArr;
    for (const QString& t : m_tabs) {
        tabArr.append(t);
    }
    obj["tabs"] = tabArr;
    obj["activeTabIndex"] = m_activeTabIndex;
    obj["tabHeight"] = m_tabHeight;
    obj["tabPosition"] = m_tabPosition;
    obj["tabColor"] = serializeColor(m_tabColor, colorStyleRef("tabColor"));
    obj["activeTabColor"] = serializeColor(m_activeTabColor, colorStyleRef("activeTabColor"));
    obj["activeTabTextColor"] = serializeColor(m_activeTabTextColor, colorStyleRef("activeTabTextColor"));
    obj["inactiveTabTextColor"] = serializeColor(m_inactiveTabTextColor, colorStyleRef("inactiveTabTextColor"));
    obj["contentAreaColor"] = serializeColor(m_contentAreaColor, colorStyleRef("contentAreaColor"));
    obj["borderColor"] = serializeColor(m_borderColor, colorStyleRef("borderColor"));
    obj["borderWidth"] = m_borderWidth;
    return obj;
}

void TabViewComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);

    if (json.contains("tabs") && json.value("tabs").isArray()) {
        m_tabs.clear();
        for (const QJsonValue& v : json.value("tabs").toArray()) {
            m_tabs.append(v.toString());
        }
        if (m_tabs.isEmpty()) m_tabs = {"General", "Display", "Network"};
    }
    if (json.contains("activeTabIndex")) m_activeTabIndex = json.value("activeTabIndex").toInt(0);
    if (json.contains("tabHeight")) m_tabHeight = json.value("tabHeight").toInt(30);
    if (json.contains("tabPosition")) m_tabPosition = json.value("tabPosition").toString("Top");

    if (json.contains("tabColor")) {
        QString ref;
        deserializeColor(json.value("tabColor"), m_tabColor, ref);
        setColorStyleRef("tabColor", ref);
    }
    if (json.contains("activeTabColor")) {
        QString ref;
        deserializeColor(json.value("activeTabColor"), m_activeTabColor, ref);
        setColorStyleRef("activeTabColor", ref);
    }
    if (json.contains("activeTabTextColor")) {
        QString ref;
        deserializeColor(json.value("activeTabTextColor"), m_activeTabTextColor, ref);
        setColorStyleRef("activeTabTextColor", ref);
    }
    if (json.contains("inactiveTabTextColor")) {
        QString ref;
        deserializeColor(json.value("inactiveTabTextColor"), m_inactiveTabTextColor, ref);
        setColorStyleRef("inactiveTabTextColor", ref);
    }
    if (json.contains("contentAreaColor")) {
        QString ref;
        deserializeColor(json.value("contentAreaColor"), m_contentAreaColor, ref);
        setColorStyleRef("contentAreaColor", ref);
    }
    if (json.contains("borderColor")) {
        QString ref;
        deserializeColor(json.value("borderColor"), m_borderColor, ref);
        setColorStyleRef("borderColor", ref);
    }
    m_borderWidth = json.value("borderWidth").toInt(m_borderWidth);
    update();
}

QString TabViewComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1// TabView: %2\n").arg(indent, m_id);
    qml += QString("%1Item {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(m_width).arg(m_height);
    qml += QString("%1    property int currentIndex: %2\n").arg(indent).arg(m_activeTabIndex);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString TabViewComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// TabView: %2 (%3 tabs)\n").arg(indent, m_id).arg(m_tabs.size());
    return code;
}
