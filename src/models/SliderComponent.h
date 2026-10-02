#pragma once

#include "UIComponent.h"
#include <QColor>

class SliderComponent : public UIComponent {
    Q_OBJECT

public:
    explicit SliderComponent(const QString& id, QGraphicsItem* parent = nullptr);

    int value() const { return m_value; }
    void setValue(int val);

    int minimum() const { return m_minimum; }
    void setMinimum(int min);

    int maximum() const { return m_maximum; }
    void setMaximum(int max);

    QColor trackColor() const { return m_trackColor; }
    void setTrackColor(const QColor& color);

    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor& color);

    QColor handleColor() const { return m_handleColor; }
    void setHandleColor(const QColor& color);

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    // Code Generation
    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

    void applyColorStyle(const QString& styleName, const QColor& color) override;

protected:
    void paintComponent(QPainter* painter) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

private:
    int m_value = 50;
    int m_minimum = 0;
    int m_maximum = 100;
    QColor m_trackColor = QColor("#2F333E");
    QColor m_fillColor = QColor("#2196F3");
    QColor m_handleColor = QColor("#FFFFFF");

    void updateValueFromPos(const QPointF& pos);
};
