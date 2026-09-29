#pragma once

#include "UIComponent.h"
#include <QColor>

class TextInputComponent : public UIComponent {
    Q_OBJECT

public:
    explicit TextInputComponent(const QString& id, QGraphicsItem* parent = nullptr);

    QString text() const { return m_text; }
    void setText(const QString& text);

    QString placeholder() const { return m_placeholder; }
    void setPlaceholder(const QString& placeholder);

    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor& color);

    QColor placeholderColor() const { return m_placeholderColor; }
    void setPlaceholderColor(const QColor& color);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    QColor borderColor() const { return m_borderColor; }
    void setBorderColor(const QColor& color);

    int borderWidth() const { return m_borderWidth; }
    void setBorderWidth(int width);

    int cornerRadius() const { return m_cornerRadius; }
    void setCornerRadius(int radius);

    int pixelSize() const { return m_pixelSize; }
    void setPixelSize(int size);

    bool isReadOnly() const { return m_readOnly; }
    void setReadOnly(bool ro);

    QString onTextChangedHandler() const { return m_onTextChangedHandler; }
    void setOnTextChangedHandler(const QString& handler);

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    // Code Generation
    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    QString m_text;
    QString m_placeholder = "Enter text...";
    QColor m_textColor = QColor("#FFFFFF");
    QColor m_placeholderColor = QColor("#717C8F");
    QColor m_backgroundColor = QColor("#1E212B");
    QColor m_borderColor = QColor("#3E4459");
    int m_borderWidth = 1;
    int m_cornerRadius = 4;
    int m_pixelSize = 13;
    bool m_readOnly = false;
    QString m_onTextChangedHandler;
};
