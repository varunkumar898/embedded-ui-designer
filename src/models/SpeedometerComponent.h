#pragma once

#include "ValueVisualizationComponent.h"

class SpeedometerComponent : public ValueVisualizationComponent {
    Q_OBJECT

public:
    explicit SpeedometerComponent(const QString& id = "speedometer", QGraphicsItem* parent = nullptr);
    ~SpeedometerComponent() override = default;

    int majorTicks() const { return m_majorTicks; }
    void setMajorTicks(int ticks);

    int minorTicks() const { return m_minorTicks; }
    void setMinorTicks(int ticks);

    bool showNeedle() const { return m_showNeedle; }
    void setShowNeedle(bool show);

    QColor needleColor() const { return m_needleColor; }
    void setNeedleColor(const QColor& color);

    QString stylePreset() const { return m_stylePreset; }
    void setStylePreset(const QString& preset);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    int m_startAngle = 225;
    int m_sweepAngle = 270;
    int m_majorTicks = 9; // 0, 30, 60, 90, 120, 150, 180, 210, 240
    int m_minorTicks = 2;
    bool m_showNeedle = true;
    QColor m_needleColor = QColor(239, 68, 68);
    QString m_stylePreset = "Modern"; // Modern, Classic, Compact
};
