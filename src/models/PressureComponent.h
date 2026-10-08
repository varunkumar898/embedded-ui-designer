#pragma once

#include "ValueVisualizationComponent.h"

class PressureComponent : public ValueVisualizationComponent {
    Q_OBJECT

public:
    explicit PressureComponent(const QString& id = "pressure", QGraphicsItem* parent = nullptr);
    ~PressureComponent() override = default;

    QString displayStyle() const { return m_displayStyle; }
    void setDisplayStyle(const QString& style);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    QString m_displayStyle = "Dial"; // Dial, Bar, Compact
};
