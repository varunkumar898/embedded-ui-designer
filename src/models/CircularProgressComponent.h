#pragma once

#include "ValueVisualizationComponent.h"

class CircularProgressComponent : public ValueVisualizationComponent {
    Q_OBJECT

public:
    explicit CircularProgressComponent(const QString& id = "circular_progress", QGraphicsItem* parent = nullptr);
    ~CircularProgressComponent() override = default;

    int thickness() const { return m_thickness; }
    void setThickness(int t);

    int startAngle() const { return m_startAngle; }
    void setStartAngle(int angle);

    int sweepAngle() const { return m_sweepAngle; }
    void setSweepAngle(int angle);

    bool showPercentage() const { return m_showPercentage; }
    void setShowPercentage(bool show);

    QString centerLabel() const { return m_centerLabel; }
    void setCenterLabel(const QString& label);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    int m_thickness = 10;
    int m_startAngle = 90;   // 90 degrees = 12 o'clock in standard math, or top
    int m_sweepAngle = 360;  // 360 for full circle
    bool m_showPercentage = true;
    QString m_centerLabel = "";
};
