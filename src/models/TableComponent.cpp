#include "TableComponent.h"
#include <QPen>
#include <QBrush>
#include <algorithm>

TableComponent::TableComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "Table", parent)
{
    m_width = 280;
    m_height = 140;
}

void TableComponent::setColumns(const QStringList& cols) {
    if (!cols.isEmpty()) {
        m_columns = cols;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setRows(const QList<QStringList>& rows) {
    m_rows = rows;
    if (m_selectedRow >= m_rows.size()) {
        m_selectedRow = m_rows.isEmpty() ? -1 : m_rows.size() - 1;
    }
    update();
    emit propertyChanged(this);
}

void TableComponent::addRow(const QStringList& row) {
    m_rows.append(row);
    update();
    emit propertyChanged(this);
}

void TableComponent::removeRow(int index) {
    if (index >= 0 && index < m_rows.size()) {
        m_rows.removeAt(index);
        if (m_selectedRow >= m_rows.size()) {
            m_selectedRow = m_rows.size() - 1;
        }
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setSelectedRow(int row) {
    if (row >= -1 && row < m_rows.size() && m_selectedRow != row) {
        m_selectedRow = row;
        update();
        emit propertyChanged(this);
        emit interactionTriggered("On Row Selected");
    }
}

void TableComponent::setHeaderHeight(int h) {
    h = std::clamp(h, 16, 60);
    if (m_headerHeight != h) {
        m_headerHeight = h;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setRowHeight(int h) {
    h = std::clamp(h, 16, 60);
    if (m_rowHeight != h) {
        m_rowHeight = h;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setShowHeader(bool show) {
    if (m_showHeader != show) {
        m_showHeader = show;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setAlternatingRows(bool alt) {
    if (m_alternatingRows != alt) {
        m_alternatingRows = alt;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setHeaderColor(const QColor& color) {
    if (m_headerColor != color) {
        m_headerColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setHeaderTextColor(const QColor& color) {
    if (m_headerTextColor != color) {
        m_headerTextColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setBackgroundColor(const QColor& color) {
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setAlternateRowColor(const QColor& color) {
    if (m_alternateRowColor != color) {
        m_alternateRowColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setSelectedRowColor(const QColor& color) {
    if (m_selectedRowColor != color) {
        m_selectedRowColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setTextColor(const QColor& color) {
    if (m_textColor != color) {
        m_textColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setGridColor(const QColor& color) {
    if (m_gridColor != color) {
        m_gridColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::setBorderWidth(int width) {
    width = std::max(0, width);
    if (m_borderWidth != width) {
        m_borderWidth = width;
        update();
        emit propertyChanged(this);
    }
}

void TableComponent::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !m_rows.isEmpty()) {
        QPointF p = event->pos();
        qreal startY = m_showHeader ? m_headerHeight : 0.0;
        if (p.y() >= startY && p.y() < m_height) {
            int clickedRow = static_cast<int>((p.y() - startY) / m_rowHeight);
            if (clickedRow >= 0 && clickedRow < m_rows.size()) {
                setSelectedRow(clickedRow);
                event->accept();
                return;
            }
        }
    }
    UIComponent::mousePressEvent(event);
}

void TableComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF frameRect(0, 0, m_width, m_height);

    // 1. Table Frame Background & Border
    painter->setPen(QPen(m_borderColor, m_borderWidth));
    painter->setBrush(QBrush(m_backgroundColor));
    painter->drawRoundedRect(frameRect, 6.0, 6.0);

    int colCount = std::max<int>(1, m_columns.size());
    qreal colW = m_width / colCount;

    qreal curY = 0.0;

    // 2. Draw Table Header
    if (m_showHeader) {
        QRectF headerRect(0, 0, m_width, m_headerHeight);
        painter->setPen(QPen(m_borderColor, m_borderWidth));
        painter->setBrush(QBrush(m_headerColor));
        painter->drawRoundedRect(headerRect, 6.0, 6.0);

        painter->setPen(m_headerTextColor);
        QFont hFont = painter->font();
        hFont.setPixelSize(std::clamp(static_cast<int>(m_headerHeight * 0.44), 9, 13));
        hFont.setBold(true);
        painter->setFont(hFont);

        for (int c = 0; c < colCount; ++c) {
            QRectF cellRect(c * colW + 6, 0, colW - 12, m_headerHeight);
            painter->drawText(cellRect, Qt::AlignLeft | Qt::AlignVCenter, m_columns.at(c));

            // Column separator
            if (c > 0) {
                painter->setPen(QPen(m_gridColor, 1.0));
                painter->drawLine(QPointF(c * colW, 0), QPointF(c * colW, m_height));
                painter->setPen(m_headerTextColor);
            }
        }
        curY += m_headerHeight;
    }

    // 3. Draw Table Data Rows
    int rowCount = m_rows.size();
    for (int r = 0; r < rowCount; ++r) {
        if (curY >= m_height) break;
        qreal rowH = std::min<qreal>(m_rowHeight, m_height - curY);
        QRectF rowRect(1, curY, m_width - 2, rowH);

        bool isSelected = (r == m_selectedRow);

        // Row background
        if (isSelected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QBrush(m_selectedRowColor));
            painter->drawRect(rowRect);
        } else if (m_alternatingRows && (r % 2 == 1)) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QBrush(m_alternateRowColor));
            painter->drawRect(rowRect);
        }

        // Horizontal gridline
        painter->setPen(QPen(m_gridColor, 1.0));
        painter->drawLine(QPointF(0, curY), QPointF(m_width, curY));

        // Row Cells
        const QStringList& rowData = m_rows.at(r);
        painter->setPen(isSelected ? QColor(255, 255, 255) : m_textColor);
        QFont cellFont = painter->font();
        cellFont.setPixelSize(std::clamp(static_cast<int>(m_rowHeight * 0.44), 9, 12));
        cellFont.setBold(isSelected);
        painter->setFont(cellFont);

        for (int c = 0; c < colCount; ++c) {
            QString cellStr = (c < rowData.size()) ? rowData.at(c) : "";
            QRectF cellRect(c * colW + 6, curY, colW - 12, rowH);
            painter->drawText(cellRect, Qt::AlignLeft | Qt::AlignVCenter, cellStr);
        }

        curY += m_rowHeight;
    }
}

QJsonObject TableComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    QJsonArray colsArr;
    for (const QString& c : m_columns) colsArr.append(c);
    obj["columns"] = colsArr;

    QJsonArray rowsArr;
    for (const QStringList& r : m_rows) {
        QJsonArray rArr;
        for (const QString& cell : r) rArr.append(cell);
        rowsArr.append(rArr);
    }
    obj["rows"] = rowsArr;
    obj["selectedRow"] = m_selectedRow;
    obj["headerHeight"] = m_headerHeight;
    obj["rowHeight"] = m_rowHeight;
    obj["showHeader"] = m_showHeader;
    obj["alternatingRows"] = m_alternatingRows;

    obj["headerColor"] = serializeColor(m_headerColor, colorStyleRef("headerColor"));
    obj["headerTextColor"] = serializeColor(m_headerTextColor, colorStyleRef("headerTextColor"));
    obj["backgroundColor"] = serializeColor(m_backgroundColor, colorStyleRef("backgroundColor"));
    obj["alternateRowColor"] = serializeColor(m_alternateRowColor, colorStyleRef("alternateRowColor"));
    obj["selectedRowColor"] = serializeColor(m_selectedRowColor, colorStyleRef("selectedRowColor"));
    obj["textColor"] = serializeColor(m_textColor, colorStyleRef("textColor"));
    obj["gridColor"] = serializeColor(m_gridColor, colorStyleRef("gridColor"));
    obj["borderColor"] = serializeColor(m_borderColor, colorStyleRef("borderColor"));
    obj["borderWidth"] = m_borderWidth;
    return obj;
}

void TableComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);

    if (json.contains("columns") && json.value("columns").isArray()) {
        m_columns.clear();
        for (const QJsonValue& v : json.value("columns").toArray()) {
            m_columns.append(v.toString());
        }
    }
    if (json.contains("rows") && json.value("rows").isArray()) {
        m_rows.clear();
        for (const QJsonValue& rv : json.value("rows").toArray()) {
            QStringList row;
            for (const QJsonValue& cv : rv.toArray()) {
                row.append(cv.toString());
            }
            m_rows.append(row);
        }
    }
    if (json.contains("selectedRow")) m_selectedRow = json.value("selectedRow").toInt(0);
    if (json.contains("headerHeight")) m_headerHeight = json.value("headerHeight").toInt(26);
    if (json.contains("rowHeight")) m_rowHeight = json.value("rowHeight").toInt(24);
    if (json.contains("showHeader")) m_showHeader = json.value("showHeader").toBool(true);
    if (json.contains("alternatingRows")) m_alternatingRows = json.value("alternatingRows").toBool(true);

    if (json.contains("headerColor")) {
        QString ref;
        deserializeColor(json.value("headerColor"), m_headerColor, ref);
        setColorStyleRef("headerColor", ref);
    }
    if (json.contains("headerTextColor")) {
        QString ref;
        deserializeColor(json.value("headerTextColor"), m_headerTextColor, ref);
        setColorStyleRef("headerTextColor", ref);
    }
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
    if (json.contains("gridColor")) {
        QString ref;
        deserializeColor(json.value("gridColor"), m_gridColor, ref);
        setColorStyleRef("gridColor", ref);
    }
    if (json.contains("borderColor")) {
        QString ref;
        deserializeColor(json.value("borderColor"), m_borderColor, ref);
        setColorStyleRef("borderColor", ref);
    }
    m_borderWidth = json.value("borderWidth").toInt(m_borderWidth);
    update();
}

QString TableComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1// Table: %2\n").arg(indent, m_id);
    qml += QString("%1TableView {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(m_width).arg(m_height);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString TableComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Table: %2 (%3 columns, %4 rows)\n").arg(indent, m_id).arg(m_columns.size()).arg(m_rows.size());
    return code;
}
