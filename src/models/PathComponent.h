#pragma once

#include "UIComponent.h"
#include <QPolygonF>

class PathComponent : public UIComponent {
    Q_OBJECT

public:
    explicit PathComponent(const QString& id);

    QPolygonF points() const { return m_points; }
    QColor strokeColor() const { return m_strokeColor; }
    qreal strokeThickness() const { return m_strokeThickness; }
    int opacityPercent() const { return m_opacityPercent; }

    void setPoints(const QPolygonF& points);
    void setStrokeColor(const QColor& color);
    void setStrokeThickness(qreal thickness);
    void setOpacityPercent(int opacity);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;
    QString toQmlSnippet(int indentSpaces = 8) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    QPolygonF m_points;
    QColor m_strokeColor = QColor("#e4583e");
    qreal m_strokeThickness = 3.0;
    int m_opacityPercent = 100;
};