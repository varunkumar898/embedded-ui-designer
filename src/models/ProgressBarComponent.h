#pragma once

#include "UIComponent.h"

class ProgressBarComponent : public UIComponent {
    Q_OBJECT

public:
    explicit ProgressBarComponent(const QString& id = "progress_bar", QGraphicsItem* parent = nullptr);

    // Normalized ratio 0.0 to 1.0
    double value() const { return m_value; }
    void setValue(double val);

    // Range [minimum, maximum]
    double minimum() const { return m_minimum; }
    void setMinimum(double min);

    double maximum() const { return m_maximum; }
    void setMaximum(double max);

    // Actual engineering value between minimum and maximum
    double actualValue() const;
    void setActualValue(double val);

    // Orientation: "Horizontal" or "Vertical"
    QString orientation() const { return m_orientation == Qt::Vertical ? "Vertical" : "Horizontal"; }
    void setOrientation(const QString& orient);
    Qt::Orientation orientationEnum() const { return m_orientation; }

    // Colors
    QColor barColor() const { return m_barColor; }
    void setBarColor(const QColor& color);

    QColor fillColor() const { return m_barColor; }
    void setFillColor(const QColor& color) { setBarColor(color); }

    QColor trackColor() const { return m_trackColor; }
    void setTrackColor(const QColor& color);

    QColor borderColor() const { return m_borderColor; }
    void setBorderColor(const QColor& color);

    int borderWidth() const { return m_borderWidth; }
    void setBorderWidth(int width);

    bool hasCornerRadius() const override { return true; }
    int cornerRadius() const override { return m_cornerRadius; }
    void setCornerRadius(int r) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

    void applyColorStyle(const QString& styleName, const QColor& color) override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    double m_value = 0.5; // Normalized ratio 0.0 to 1.0
    double m_minimum = 0.0;
    double m_maximum = 100.0;
    Qt::Orientation m_orientation = Qt::Horizontal;

    QColor m_barColor = QColor(16, 185, 129); // Modern Emerald Green (#10b981)
    QColor m_trackColor = QColor(28, 32, 40); // Dark rounded track (#1c2028)
    QColor m_borderColor = QColor(45, 52, 65); // Subtle contrast border (#2d3441)
    int m_borderWidth = 1;
    int m_cornerRadius = 6;
};
