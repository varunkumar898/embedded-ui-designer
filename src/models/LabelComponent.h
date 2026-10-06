#pragma once

#include "UIComponent.h"

class LabelComponent : public UIComponent {
    Q_OBJECT

public:
    explicit LabelComponent(const QString& id = "label", QGraphicsItem* parent = nullptr);

    QString text() const { return m_text; }
    void setText(const QString& text);

    QColor color() const { return m_color; }
    void setColor(const QColor& color);

    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString& family);

    int pixelSize() const { return m_pixelSize; }
    void setPixelSize(int size);

    bool bold() const { return m_bold; }
    void setBold(bool b);

    bool italic() const { return m_italic; }
    void setItalic(bool i);

    Qt::Alignment alignment() const { return m_alignment; }
    void setAlignment(Qt::Alignment align);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    bool hasCornerRadius() const override { return false; }
    int cornerRadius() const override { return m_cornerRadius; }
    void setCornerRadius(int r) override;

    // ── Extended typography (Task 5) ───────────────────────────────────────────
    qreal letterSpacing() const { return m_letterSpacing; }
    void  setLetterSpacing(qreal sp);

    int lineHeight() const { return m_lineHeight; } ///< 0=default; else % of pixel-size
    void setLineHeight(int pct);

    static const QStringList& availableFonts(); ///< Curated embedded font list

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

    void applyColorStyle(const QString& styleName, const QColor& color) override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    QString m_text = "Label";
    QColor m_color = Qt::black;
    QColor m_backgroundColor = Qt::transparent;
    int m_cornerRadius = 0;
    QString m_fontFamily = "Roboto";
    int m_pixelSize = 16;
    bool m_bold = false;
    bool m_italic = false;
    Qt::Alignment m_alignment = Qt::AlignLeft | Qt::AlignVCenter;
    qreal m_letterSpacing = 0.0;  ///< Extra px between chars (QFont::AbsoluteSpacing)
    int   m_lineHeight    = 0;    ///< 0=default, else % of pixelSize
};
