#pragma once

#include "ValueVisualizationComponent.h"

class TemperatureComponent : public ValueVisualizationComponent {
    Q_OBJECT

public:
    explicit TemperatureComponent(const QString& id = "temperature", QGraphicsItem* parent = nullptr);
    ~TemperatureComponent() override = default;

    QString displayStyle() const { return m_displayStyle; }
    void setDisplayStyle(const QString& style); // "Thermometer", "Dial", "Compact"

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    QString m_displayStyle = "Thermometer";
};
