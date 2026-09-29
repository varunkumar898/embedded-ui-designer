#pragma once

#include "UIComponent.h"

class ProgressBarComponent : public UIComponent {
    Q_OBJECT

public:
    explicit ProgressBarComponent(const QString& id = "progress_bar", QGraphicsItem* parent = nullptr);

    double value() const { return m_value; }
    void setValue(double val);

    QColor barColor() const { return m_barColor; }
    void setBarColor(const QColor& color);

    QColor trackColor() const { return m_trackColor; }
    void setTrackColor(const QColor& color);

    bool hasCornerRadius() const override { return true; }
    int cornerRadius() const override { return m_cornerRadius; }
    void setCornerRadius(int r) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    double m_value = 0.5; // 0.0 to 1.0
    QColor m_barColor = QColor(76, 175, 80); // Material Green
    QColor m_trackColor = QColor(220, 224, 230);
    int m_cornerRadius = 4;
};
