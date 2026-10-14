#pragma once

#include "UIComponent.h"
#include <QJsonObject>

enum class TextEffectType {
    None,
    DropShadow,
    Glow,
    Echo,
    Outline,
    Background,
    Hollow,
    Neon,
    Splice
};

struct TextEffect {
    TextEffectType type = TextEffectType::None;
    QColor color = QColor(0, 0, 0, 180);
    QColor secondaryColor = QColor(0, 150, 255);
    qreal offsetX = 3.0;
    qreal offsetY = 3.0;
    qreal blur = 4.0;
    qreal thickness = 2.0;
    qreal opacity = 1.0;
    int count = 2;
    int bgPadding = 4;
    int bgCornerRadius = 4;
    QColor bgColor = QColor(250, 204, 21);

    QJsonObject toJson() const {
        QJsonObject obj;
        QString tStr = "none";
        switch (type) {
            case TextEffectType::DropShadow: tStr = "shadow"; break;
            case TextEffectType::Glow:       tStr = "glow"; break;
            case TextEffectType::Echo:       tStr = "echo"; break;
            case TextEffectType::Outline:    tStr = "outline"; break;
            case TextEffectType::Background: tStr = "background"; break;
            case TextEffectType::Hollow:     tStr = "hollow"; break;
            case TextEffectType::Neon:       tStr = "neon"; break;
            case TextEffectType::Splice:     tStr = "splice"; break;
            default:                         tStr = "none"; break;
        }
        obj["type"] = tStr;
        obj["color"] = color.name(QColor::HexArgb);
        obj["secondaryColor"] = secondaryColor.name(QColor::HexArgb);
        obj["offsetX"] = offsetX;
        obj["offsetY"] = offsetY;
        obj["blur"] = blur;
        obj["thickness"] = thickness;
        obj["opacity"] = opacity;
        obj["count"] = count;
        obj["bgPadding"] = bgPadding;
        obj["bgCornerRadius"] = bgCornerRadius;
        obj["bgColor"] = bgColor.name(QColor::HexArgb);
        return obj;
    }

    static TextEffect fromJson(const QJsonObject& obj) {
        TextEffect eff;
        QString tStr = obj.value("type").toString("none").toLower();
        if (tStr == "shadow" || tStr == "dropshadow") eff.type = TextEffectType::DropShadow;
        else if (tStr == "glow")                      eff.type = TextEffectType::Glow;
        else if (tStr == "echo")                      eff.type = TextEffectType::Echo;
        else if (tStr == "outline")                   eff.type = TextEffectType::Outline;
        else if (tStr == "background")                eff.type = TextEffectType::Background;
        else if (tStr == "hollow")                    eff.type = TextEffectType::Hollow;
        else if (tStr == "neon")                      eff.type = TextEffectType::Neon;
        else if (tStr == "splice")                    eff.type = TextEffectType::Splice;
        else                                          eff.type = TextEffectType::None;

        if (obj.contains("color")) eff.color = QColor(obj.value("color").toString());
        if (obj.contains("secondaryColor")) eff.secondaryColor = QColor(obj.value("secondaryColor").toString());
        eff.offsetX = obj.value("offsetX").toDouble(3.0);
        eff.offsetY = obj.value("offsetY").toDouble(3.0);
        eff.blur = obj.value("blur").toDouble(4.0);
        eff.thickness = obj.value("thickness").toDouble(2.0);
        eff.opacity = obj.value("opacity").toDouble(1.0);
        eff.count = obj.value("count").toInt(2);
        eff.bgPadding = obj.value("bgPadding").toInt(4);
        eff.bgCornerRadius = obj.value("bgCornerRadius").toInt(4);
        if (obj.contains("bgColor")) eff.bgColor = QColor(obj.value("bgColor").toString());
        return eff;
    }
};

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

    bool underline() const { return m_underline; }
    void setUnderline(bool u);

    bool strikeThrough() const { return m_strikeThrough; }
    void setStrikeThrough(bool s);

    QString textCasing() const { return m_textCasing; }
    void setTextCasing(const QString& casing); // "none", "uppercase", "lowercase", "titlecase"

    Qt::Alignment alignment() const { return m_alignment; }
    void setAlignment(Qt::Alignment align);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    bool hasCornerRadius() const override { return false; }
    int cornerRadius() const override { return m_cornerRadius; }
    void setCornerRadius(int r) override;

    // ── Extended typography ───────────────────────────────────────────
    qreal letterSpacing() const { return m_letterSpacing; }
    void setLetterSpacing(qreal sp);

    int lineHeight() const { return m_lineHeight; } ///< 0=default; else % of pixel-size
    void setLineHeight(int pct);

    // ── Curvature ─────────────────────────────────────────────────────
    int curvature() const { return m_curvature; } ///< -100 to +100 (0 = straight)
    void setCurvature(int curve);

    // ── Text Effects ──────────────────────────────────────────────────
    TextEffect textEffect() const { return m_textEffect; }
    void setTextEffect(const TextEffect& effect);

    static const QStringList& availableFonts();

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

    void applyColorStyle(const QString& styleName, const QColor& color) override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    QString transformedText() const;

    QString m_text = "Label";
    QColor m_color = Qt::black;
    QColor m_backgroundColor = Qt::transparent;
    int m_cornerRadius = 0;
    QString m_fontFamily = "Roboto";
    int m_pixelSize = 16;
    bool m_bold = false;
    bool m_italic = false;
    bool m_underline = false;
    bool m_strikeThrough = false;
    QString m_textCasing = "none";
    Qt::Alignment m_alignment = Qt::AlignLeft | Qt::AlignVCenter;
    qreal m_letterSpacing = 0.0;
    int m_lineHeight = 0;
    int m_curvature = 0;
    TextEffect m_textEffect;
};
