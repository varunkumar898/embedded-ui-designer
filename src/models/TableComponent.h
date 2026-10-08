#pragma once

#include "UIComponent.h"
#include <QStringList>
#include <QList>
#include <QColor>

class TableComponent : public UIComponent {
    Q_OBJECT

public:
    explicit TableComponent(const QString& id = "table", QGraphicsItem* parent = nullptr);
    ~TableComponent() override = default;

    QStringList columns() const { return m_columns; }
    void setColumns(const QStringList& cols);

    QList<QStringList> rows() const { return m_rows; }
    void setRows(const QList<QStringList>& rows);
    void addRow(const QStringList& row);
    void removeRow(int index);

    int selectedRow() const { return m_selectedRow; }
    void setSelectedRow(int row);

    int headerHeight() const { return m_headerHeight; }
    void setHeaderHeight(int h);

    int rowHeight() const { return m_rowHeight; }
    void setRowHeight(int h);

    bool showHeader() const { return m_showHeader; }
    void setShowHeader(bool show);

    bool alternatingRows() const { return m_alternatingRows; }
    void setAlternatingRows(bool alt);

    QColor headerColor() const { return m_headerColor; }
    void setHeaderColor(const QColor& color);

    QColor headerTextColor() const { return m_headerTextColor; }
    void setHeaderTextColor(const QColor& color);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    QColor alternateRowColor() const { return m_alternateRowColor; }
    void setAlternateRowColor(const QColor& color);

    QColor selectedRowColor() const { return m_selectedRowColor; }
    void setSelectedRowColor(const QColor& color);

    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor& color);

    QColor gridColor() const { return m_gridColor; }
    void setGridColor(const QColor& color);

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
    QStringList m_columns = {"ID", "Parameter", "Value", "Status"};
    QList<QStringList> m_rows = {
        {"1", "Bus Voltage", "12.4 V", "OK"},
        {"2", "Current", "1.8 A", "Normal"},
        {"3", "Core Temp", "48 °C", "OK"},
        {"4", "Fan Speed", "2400 RPM", "Active"}
    };
    int m_selectedRow = 0;
    int m_headerHeight = 26;
    int m_rowHeight = 24;
    bool m_showHeader = true;
    bool m_alternatingRows = true;

    QColor m_headerColor = QColor(30, 41, 59);        // Slate-800 (#1e293b)
    QColor m_headerTextColor = QColor(241, 245, 249);  // Slate-100 (#f1f5f9)
    QColor m_backgroundColor = QColor(15, 23, 42);     // Slate-900 (#0f172a)
    QColor m_alternateRowColor = QColor(24, 32, 47);   // Slate-850
    QColor m_selectedRowColor = QColor(37, 99, 235);   // Blue-600 (#2563eb)
    QColor m_textColor = QColor(226, 232, 240);        // Slate-200
    QColor m_gridColor = QColor(51, 65, 85, 120);      // Slate-700
    QColor m_borderColor = QColor(51, 65, 85);         // Slate-700
    int m_borderWidth = 1;
};
