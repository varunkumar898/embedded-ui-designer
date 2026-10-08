#pragma once

#include "UIComponent.h"
#include <QStringList>
#include <QColor>

class ListComponent : public UIComponent {
    Q_OBJECT

public:
    explicit ListComponent(const QString& id = "list", QGraphicsItem* parent = nullptr);
    ~ListComponent() override = default;

    QStringList items() const { return m_items; }
    void setItems(const QStringList& items);
    void addItem(const QString& item);
    void removeItem(int index);
    void setItem(int index, const QString& text);

    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int idx);

    int rowHeight() const { return m_rowHeight; }
    void setRowHeight(int h);

    bool alternatingRows() const { return m_alternatingRows; }
    void setAlternatingRows(bool alt);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    QColor alternateRowColor() const { return m_alternateRowColor; }
    void setAlternateRowColor(const QColor& color);

    QColor selectedRowColor() const { return m_selectedRowColor; }
    void setSelectedRowColor(const QColor& color);

    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor& color);

    QColor selectedTextColor() const { return m_selectedTextColor; }
    void setSelectedTextColor(const QColor& color);

    QColor borderColor() const { return m_borderColor; }
    void setBorderColor(const QColor& color);

    int borderWidth() const { return m_borderWidth; }
    void setBorderWidth(int width);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QStringList m_items = {"Sensor 1: OK", "Sensor 2: Active", "Sensor 3: Standby", "Sensor 4: Calibrating"};
    int m_selectedIndex = 0;
    int m_rowHeight = 28;
    bool m_alternatingRows = true;

    QColor m_backgroundColor = QColor(15, 23, 42);     // Slate-900 (#0f172a)
    QColor m_alternateRowColor = QColor(30, 41, 59);    // Slate-800 (#1e293b)
    QColor m_selectedRowColor = QColor(37, 99, 235);    // Blue-600 (#2563eb)
    QColor m_textColor = QColor(226, 232, 240);         // Slate-200
    QColor m_selectedTextColor = QColor(255, 255, 255);
    QColor m_borderColor = QColor(51, 65, 85);          // Slate-700
    int m_borderWidth = 1;
};
