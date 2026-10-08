#pragma once

#include "ValueVisualizationComponent.h"

class RpmComponent : public ValueVisualizationComponent {
    Q_OBJECT

public:
    explicit RpmComponent(const QString& id = "rpm", QGraphicsItem* parent = nullptr);
    ~RpmComponent() override = default;

    int majorTicks() const { return m_majorTicks; }
    void setMajorTicks(int ticks);

    int minorTicks() const { return m_minorTicks; }
    void setMinorTicks(int ticks);

    bool showNeedle() const { return m_showNeedle; }
    void setShowNeedle(bool show);

    QColor needleColor() const { return m_needleColor; }
    void setNeedleColor(const QColor& color);

    QString presentation() const { return m_presentation; }
    void setPresentation(const QString& pres); // "Gauge", "Bar"

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    int m_startAngle = 225;
    int m_sweepAngle = 270;
    int m_majorTicks = 9; // 0, 1, 2, 3, 4, 5, 6, 7, 8 (x1000)
    int m_minorTicks = 3;
    bool m_showNeedle = true;
    QColor m_needleColor = QColor(239, 68, 68);
    QString m_presentation = "Gauge";
};
