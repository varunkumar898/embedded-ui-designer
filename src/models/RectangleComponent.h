#pragma once

#include "UIComponent.h"

class RectangleComponent : public UIComponent {
    Q_OBJECT

public:
    explicit RectangleComponent(const QString& id = "rect", QGraphicsItem* parent = nullptr);

    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor& color);

    QColor strokeColor() const { return m_strokeColor; }
    void setStrokeColor(const QColor& color);

    // Alias for strokeColor to match border naming
    QColor borderColor() const { return m_strokeColor; }
    void setBorderColor(const QColor& color) { setStrokeColor(color); }

    int strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(int w);

    int borderWidth() const { return m_strokeWidth; }
    void setBorderWidth(int w) { setStrokeWidth(w); }

    bool hasCornerRadius() const override { return true; }
    int cornerRadius() const override { return m_cornerRadius; }
    void setCornerRadius(int r) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

    void applyColorStyle(const QString& styleName, const QColor& color) override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    QColor m_fillColor = QColor(30, 34, 42);   // Sleek dark rounded surface (#1e222a)
    QColor m_strokeColor = QColor(48, 54, 66); // Subtle border (#303642)
    int m_strokeWidth = 1;
    int m_cornerRadius = 8;
};
