#pragma once

#include "UIComponent.h"
#include <QColor>
#include <QVariantAnimation>

class SwitchComponent : public UIComponent {
    Q_OBJECT

public:
    explicit SwitchComponent(const QString& id, QGraphicsItem* parent = nullptr);

    bool isChecked() const { return m_checked; }
    void setChecked(bool checked);
    void animateChecked(bool checked, const QString& transition, int durationMs);
    qreal transitionProgress() const { return m_transitionProgress; }

    QColor onColor() const { return m_onColor; }
    void setOnColor(const QColor& color);

    QColor offColor() const { return m_offColor; }
    void setOffColor(const QColor& color);

    QColor thumbColor() const { return m_thumbColor; }
    void setThumbColor(const QColor& color);

    QString onToggledHandler() const { return m_onToggledHandler; }
    void setOnToggledHandler(const QString& handler);

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

private:
    bool m_checked = false;
    QColor m_onColor = QColor("#4CAF50");
    QColor m_offColor = QColor("#3B404E");
    QColor m_thumbColor = QColor("#FFFFFF");
    QString m_onToggledHandler;
    bool m_animationFromChecked = false;
    qreal m_transitionProgress = 0.0;
    QString m_transitionType = "Instant";
    QVariantAnimation* m_transitionAnimation = nullptr;
};
