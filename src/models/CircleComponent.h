#pragma once

#include "UIComponent.h"
#include <QColor>

class CircleComponent : public UIComponent {
    Q_OBJECT

public:
    explicit CircleComponent(const QString& id, QGraphicsItem* parent = nullptr);

    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor& color);

    QColor strokeColor() const { return m_strokeColor; }
    void setStrokeColor(const QColor& color);

    int strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(int width);

    bool isFilled() const { return m_isFilled; }
    void setFilled(bool filled);

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    // Code Generation
    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

    void applyColorStyle(const QString& styleName, const QColor& color) override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    QColor m_fillColor = QColor("#00BCD4");
    QColor m_strokeColor = QColor("#FFFFFF");
    int m_strokeWidth = 1;
    bool m_isFilled = true;
};
