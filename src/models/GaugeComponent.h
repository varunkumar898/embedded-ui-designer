#pragma once

#include "ValueVisualizationComponent.h"

class GaugeComponent : public ValueVisualizationComponent {
    Q_OBJECT

public:
    explicit GaugeComponent(const QString& id = "gauge", QGraphicsItem* parent = nullptr);
    ~GaugeComponent() override = default;

    int startAngle() const { return m_startAngle; }
    void setStartAngle(int angle);

    int sweepAngle() const { return m_sweepAngle; }
    void setSweepAngle(int angle);

    int thickness() const { return m_thickness; }
    void setThickness(int t);

    int majorTicks() const { return m_majorTicks; }
    void setMajorTicks(int ticks);

    int minorTicks() const { return m_minorTicks; }
    void setMinorTicks(int ticks);

    bool showNeedle() const { return m_showNeedle; }
    void setShowNeedle(bool show);

    QColor needleColor() const { return m_needleColor; }
    void setNeedleColor(const QColor& color);

    // Preset Gauge styles: "Arc" (270 deg), "SemiCircle" (180 deg), "FullCircle" (360 deg)
    QString gaugeStyle() const { return m_gaugeStyle; }
    void setGaugeStyle(const QString& style);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    int m_startAngle = 225; // 225 deg (bottom-left)
    int m_sweepAngle = 270; // 270 deg sweep clockwise to bottom-right (-45 deg)
    int m_thickness = 12;
    int m_majorTicks = 5;
    int m_minorTicks = 4;
    bool m_showNeedle = true;
    QColor m_needleColor = QColor(239, 68, 68); // Red-500
    QString m_gaugeStyle = "Arc";
};
