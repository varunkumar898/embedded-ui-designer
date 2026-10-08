#include "ListComponent.h"
#include <QPen>
#include <QBrush>
#include <algorithm>

ListComponent::ListComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "List", parent)
{
    m_width = 200;
    m_height = 140;
}

void ListComponent::setItems(const QStringList& items) {
    m_items = items;
    if (m_selectedIndex >= m_items.size()) {
        m_selectedIndex = m_items.isEmpty() ? -1 : m_items.size() - 1;
    }
    update();
    emit propertyChanged(this);
}

void ListComponent::addItem(const QString& item) {
    m_items.append(item.trimmed().isEmpty() ? QString("Item %1").arg(m_items.size() + 1) : item);
    update();
    emit propertyChanged(this);
}

void ListComponent::removeItem(int index) {
    if (index >= 0 && index < m_items.size()) {
        m_items.removeAt(index);
        if (m_selectedIndex >= m_items.size()) {
            m_selectedIndex = m_items.size() - 1;
        }
        update();
        emit propertyChanged(this);
    }
}

void ListComponent::setItem(int index, const QString& text) {
    if (index >= 0 && index < m_items.size()) {
        m_items[index] = text;
        update();
        emit propertyChanged(this);
    }
}

void ListComponent::setSelectedIndex(int idx) {
    if (idx >= -1 && idx < m_items.size() && m_selectedIndex != idx) {
        m_selectedIndex = idx;
        update();
        emit propertyChanged(this);
        emit interactionTriggered("On Selection Changed");
    }
}

void ListComponent::setRowHeight(int h) {
    h = std::clamp(h, 16, 80);
    if (m_rowHeight != h) {
        m_rowHeight = h;
        update();
        emit propertyChanged(this);
    }
}

void ListComponent::setAlternatingRows(bool alt) {
    if (m_alternatingRows != alt) {
        m_alternatingRows = alt;
        update();
        emit propertyChanged(this);
    }
}

void ListComponent::setBackgroundColor(const QColor& color) {
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ListComponent::setAlternateRowColor(const QColor& color) {
    if (m_alternateRowColor != color) {
        m_alternateRowColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ListComponent::setSelectedRowColor(const QColor& color) {
    if (m_selectedRowColor != color) {
        m_selectedRowColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ListComponent::setTextColor(const QColor& color) {
    if (m_textColor != color) {
        m_textColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ListComponent::setSelectedTextColor(const QColor& color) {
    if (m_selectedTextColor != color) {
        m_selectedTextColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ListComponent::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void ListComponent::setBorderWidth(int width) {
    width = std::max(0, width);
    if (m_borderWidth != width) {
        m_borderWidth = width;
        update();
        emit propertyChanged(this);
    }
}

void ListComponent::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !m_items.isEmpty()) {
        QPointF p = event->pos();
        if (p.y() >= 0 && p.y() < m_height) {
            int clickedRow = static_cast<int>(p.y() / m_rowHeight);
            if (clickedRow >= 0 && clickedRow < m_items.size()) {
                setSelectedIndex(clickedRow);
                event->accept();
                return;
            }
        }
    }
    UIComponent::mousePressEvent(event);
}

void ListComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF frameRect(0, 0, m_width, m_height);

    // 1. Frame Background & Border
    painter->setPen(QPen(m_borderColor, m_borderWidth));
    painter->setBrush(QBrush(m_backgroundColor));
    painter->drawRoundedRect(frameRect, 6.0, 6.0);

    // 2. Draw Rows
    int visibleRows = static_cast<int>(m_height / m_rowHeight);
    int count = std::min<int>(m_items.size(), visibleRows + 1);

    for (int i = 0; i < count; ++i) {
        qreal rowY = i * m_rowHeight;
        if (rowY >= m_height) break;
        qreal currentH = std::min<qreal>(m_rowHeight, m_height - rowY);

        QRectF rowRect(1, rowY + 1, m_width - 2, currentH - 2);
        bool isSelected = (i == m_selectedIndex);

        // Row background
        if (isSelected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QBrush(m_selectedRowColor));
            painter->drawRoundedRect(rowRect, 4.0, 4.0);
        } else if (m_alternatingRows && (i % 2 == 1)) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QBrush(m_alternateRowColor));
            painter->drawRoundedRect(rowRect, 2.0, 2.0);
        }

        // Row text
        painter->setPen(isSelected ? m_selectedTextColor : m_textColor);
        QFont f = painter->font();
        f.setPixelSize(std::clamp(static_cast<int>(m_rowHeight * 0.44), 9, 14));
        f.setBold(isSelected);
        painter->setFont(f);

        QRectF textRect(rowRect.left() + 10, rowRect.top(), rowRect.width() - 20, rowRect.height());
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_items.at(i));
    }

    // 3. Scrollbar indicator on the right if total items exceed visible height
    if (m_items.size() * m_rowHeight > m_height) {
        qreal scrollW = 4.0;
        qreal scrollX = m_width - scrollW - 3.0;
        qreal thumbH = std::max<qreal>(16.0, (m_height / (m_items.size() * m_rowHeight)) * m_height);
        QRectF thumbRect(scrollX, 4.0, scrollW, thumbH);

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(100, 116, 139, 160));
        painter->drawRoundedRect(thumbRect, 2.0, 2.0);
    }
}

QJsonObject ListComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    QJsonArray itemsArr;
    for (const QString& it : m_items) itemsArr.append(it);
    obj["items"] = itemsArr;
    obj["selectedIndex"] = m_selectedIndex;
    obj["rowHeight"] = m_rowHeight;
    obj["alternatingRows"] = m_alternatingRows;
    obj["backgroundColor"] = serializeColor(m_backgroundColor, colorStyleRef("backgroundColor"));
    obj["alternateRowColor"] = serializeColor(m_alternateRowColor, colorStyleRef("alternateRowColor"));
    obj["selectedRowColor"] = serializeColor(m_selectedRowColor, colorStyleRef("selectedRowColor"));
    obj["textColor"] = serializeColor(m_textColor, colorStyleRef("textColor"));
    obj["selectedTextColor"] = serializeColor(m_selectedTextColor, colorStyleRef("selectedTextColor"));
    obj["borderColor"] = serializeColor(m_borderColor, colorStyleRef("borderColor"));
    obj["borderWidth"] = m_borderWidth;
    return obj;
}

void ListComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);

    if (json.contains("items") && json.value("items").isArray()) {
        m_items.clear();
        for (const QJsonValue& v : json.value("items").toArray()) {
            m_items.append(v.toString());
        }
    }
    if (json.contains("selectedIndex")) m_selectedIndex = json.value("selectedIndex").toInt(0);
    if (json.contains("rowHeight")) m_rowHeight = json.value("rowHeight").toInt(28);
    if (json.contains("alternatingRows")) m_alternatingRows = json.value("alternatingRows").toBool(true);

    if (json.contains("backgroundColor")) {
        QString ref;
        deserializeColor(json.value("backgroundColor"), m_backgroundColor, ref);
        setColorStyleRef("backgroundColor", ref);
    }
    if (json.contains("alternateRowColor")) {
        QString ref;
        deserializeColor(json.value("alternateRowColor"), m_alternateRowColor, ref);
        setColorStyleRef("alternateRowColor", ref);
    }
    if (json.contains("selectedRowColor")) {
        QString ref;
        deserializeColor(json.value("selectedRowColor"), m_selectedRowColor, ref);
        setColorStyleRef("selectedRowColor", ref);
    }
    if (json.contains("textColor")) {
        QString ref;
        deserializeColor(json.value("textColor"), m_textColor, ref);
        setColorStyleRef("textColor", ref);
    }
    if (json.contains("selectedTextColor")) {
        QString ref;
        deserializeColor(json.value("selectedTextColor"), m_selectedTextColor, ref);
        setColorStyleRef("selectedTextColor", ref);
    }
    if (json.contains("borderColor")) {
        QString ref;
        deserializeColor(json.value("borderColor"), m_borderColor, ref);
        setColorStyleRef("borderColor", ref);
    }
    m_borderWidth = json.value("borderWidth").toInt(m_borderWidth);
    update();
}

QString ListComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1// List: %2\n").arg(indent, m_id);
    qml += QString("%1ListView {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(m_width).arg(m_height);
    qml += QString("%1    currentIndex: %2\n").arg(indent).arg(m_selectedIndex);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString ListComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// List: %2 (%3 items)\n").arg(indent, m_id).arg(m_items.size());
    return code;
}
