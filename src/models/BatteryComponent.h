#pragma once

#include "ValueVisualizationComponent.h"

class BatteryComponent : public ValueVisualizationComponent {
    Q_OBJECT

public:
    explicit BatteryComponent(const QString& id = "battery", QGraphicsItem* parent = nullptr);
    ~BatteryComponent() override = default;

    bool isCharging() const { return m_charging; }
    void setCharging(bool charging);

    QString orientation() const { return m_orientation == Qt::Vertical ? "Vertical" : "Horizontal"; }
    void setOrientation(const QString& orient);
    Qt::Orientation orientationEnum() const { return m_orientation; }

    bool showPercentage() const { return m_showPercentage; }
    void setShowPercentage(bool show);

    bool segmented() const { return m_segmented; }
    void setSegmented(bool seg);

    int segmentCount() const { return m_segmentCount; }
    void setSegmentCount(int count);

    QColor effectiveValueColor() const override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    bool m_charging = false;
    Qt::Orientation m_orientation = Qt::Horizontal;
    bool m_showPercentage = true;
    bool m_segmented = true;
    int m_segmentCount = 5;
    QColor m_chargingColor = QColor(56, 189, 248); // Cyan-400 (#38bdf8)
};
