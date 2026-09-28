#pragma once

#include "UIComponent.h"

class ButtonComponent : public UIComponent {
    Q_OBJECT

    Q_PROPERTY(QString targetScreenId READ targetScreenId WRITE setTargetScreenId NOTIFY targetScreenIdChanged)

public:
    explicit ButtonComponent(const QString& id = "button", QGraphicsItem* parent = nullptr);

    QString text() const { return m_text; }
    void setText(const QString& text);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor& color);

    int cornerRadius() const { return m_cornerRadius; }
    void setCornerRadius(int r);

    QString onClickedHandler() const { return m_onClickedHandler; }
    void setOnClickedHandler(const QString& handler);

    QString targetScreenId() const { return m_targetScreenId; }
    void setTargetScreenId(const QString& targetScreenId);

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toCppSignalSlotStub() const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

signals:
    void targetScreenIdChanged(const QString& targetScreenId);

protected:
    void paintComponent(QPainter* painter) override;

private:
    QString m_text = "Button";
    QColor m_backgroundColor = QColor(33, 150, 243); // Vibrant modern blue
    QColor m_textColor = Qt::white;
    int m_cornerRadius = 6;
    QString m_onClickedHandler = "buttonPressed";
    QString m_targetScreenId;
};
