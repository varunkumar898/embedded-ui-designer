#pragma once

#include "ValueVisualizationComponent.h"

class ProgressBarComponent : public ValueVisualizationComponent {
    Q_OBJECT

public:
    explicit ProgressBarComponent(const QString& id = "progress_bar", QGraphicsItem* parent = nullptr);
    ~ProgressBarComponent() override = default;

    // Backward-compatible barColor aliases
    QColor barColor() const { return m_valueColor; }
    void setBarColor(const QColor& color) { setValueColor(color); }

    QColor fillColor() const { return m_valueColor; }
    void setFillColor(const QColor& color) { setValueColor(color); }

    // Actual engineering value between minimum and maximum
    double actualValue() const { return m_value; }
    void setActualValue(double val) { setValue(val); }

    // Normalized ratio 0.0 to 1.0
    double value() const override { return normalizedValue(); }
    void setValue(double val) override;

    // Orientation: "Horizontal" or "Vertical"
    QString orientation() const { return m_orientation == Qt::Vertical ? "Vertical" : "Horizontal"; }
    void setOrientation(const QString& orient);
    Qt::Orientation orientationEnum() const { return m_orientation; }

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
    Qt::Orientation m_orientation = Qt::Horizontal;
    int m_cornerRadius = 4;
};
