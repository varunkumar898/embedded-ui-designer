#pragma once

#include "UIComponent.h"
#include <QColor>

class CheckboxComponent : public UIComponent {
    Q_OBJECT

public:
    explicit CheckboxComponent(const QString& id, QGraphicsItem* parent = nullptr);

    QString text() const { return m_text; }
    void setText(const QString& text);

    bool isChecked() const { return m_checked; }
    void setChecked(bool checked);

    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor& color);

    QColor checkColor() const { return m_checkColor; }
    void setCheckColor(const QColor& color);

    QColor boxColor() const { return m_boxColor; }
    void setBoxColor(const QColor& color);

    QColor borderColor() const { return m_borderColor; }
    void setBorderColor(const QColor& color);

    int borderWidth() const { return m_borderWidth; }
    void setBorderWidth(int width);

    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString& family);

    int pixelSize() const { return m_pixelSize; }
    void setPixelSize(int size);

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
    QString m_text = "Checkbox";
    bool m_checked = false;
    QColor m_textColor = QColor("#E0E0E0");
    QColor m_checkColor = QColor("#10B981");
    QColor m_boxColor = QColor("#1E222A");
    QColor m_borderColor = QColor("#4E5569");
    int m_borderWidth = 1;
    QString m_fontFamily = "Roboto";
    int m_pixelSize = 13;
    QString m_onToggledHandler;
};
