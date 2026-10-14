#include "LabelComponent.h"
#include <QFont>
#include <QFontMetrics>
#include <QPen>
#include <QPainterPath>
#include <cmath>

LabelComponent::LabelComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "Text", parent)
{
    m_width = 140;
    m_height = 30;
}

void LabelComponent::setText(const QString& text) {
    if (m_text != text) {
        m_text = text;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setColor(const QColor& color) {
    if (m_color != color) {
        m_color = color;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setFontFamily(const QString& family) {
    if (m_fontFamily != family) {
        m_fontFamily = family;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setPixelSize(int size) {
    size = std::max(6, size);
    if (m_pixelSize != size) {
        m_pixelSize = size;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setBold(bool b) {
    if (m_bold != b) {
        m_bold = b;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setItalic(bool i) {
    if (m_italic != i) {
        m_italic = i;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setUnderline(bool u) {
    if (m_underline != u) {
        m_underline = u;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setStrikeThrough(bool s) {
    if (m_strikeThrough != s) {
        m_strikeThrough = s;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setTextCasing(const QString& casing) {
    if (m_textCasing != casing) {
        m_textCasing = casing;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setAlignment(Qt::Alignment align) {
    if (m_alignment != align) {
        m_alignment = align;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setBackgroundColor(const QColor& color) {
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setCornerRadius(int r) {
    int maxR = static_cast<int>(std::floor(std::min(m_width, m_height) / 2.0));
    r = std::clamp(r, 0, std::max(0, maxR));
    if (m_cornerRadius != r) {
        m_cornerRadius = r;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setLetterSpacing(qreal sp) {
    if (!qFuzzyCompare(m_letterSpacing, sp)) {
        m_letterSpacing = sp;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setLineHeight(int pct) {
    if (m_lineHeight != pct) {
        m_lineHeight = pct;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setCurvature(int curve) {
    curve = std::clamp(curve, -100, 100);
    if (m_curvature != curve) {
        m_curvature = curve;
        update();
        emit propertyChanged(this);
    }
}

void LabelComponent::setTextEffect(const TextEffect& effect) {
    m_textEffect = effect;
    update();
    emit propertyChanged(this);
}

const QStringList& LabelComponent::availableFonts() {
    static const QStringList fonts = {
        "Roboto", "Roboto Mono", "Noto Sans", "Open Sans", "Lato",
        "Raleway", "Oswald", "Merriweather", "Ubuntu", "PT Sans",
        "Fira Sans", "Inter", "Source Sans Pro", "Montserrat",
        "Arial", "Helvetica", "Segoe UI", "Tahoma"
    };
    return fonts;
}

QString LabelComponent::transformedText() const {
    if (m_textCasing == "uppercase") {
        return m_text.toUpper();
    } else if (m_textCasing == "lowercase") {
        return m_text.toLower();
    } else if (m_textCasing == "titlecase") {
        QString result = m_text;
        bool capitalizeNext = true;
        for (int i = 0; i < result.length(); ++i) {
            if (result[i].isSpace()) {
                capitalizeNext = true;
            } else if (capitalizeNext) {
                result[i] = result[i].toUpper();
                capitalizeNext = false;
            } else {
                result[i] = result[i].toLower();
            }
        }
        return result;
    }
    return m_text;
}

void LabelComponent::paintComponent(QPainter* painter) {
    const QString displayText = transformedText();

    // 1. Background Fill
    if (m_backgroundColor.isValid() && m_backgroundColor.alpha() > 0) {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(m_backgroundColor));
        painter->drawRoundedRect(QRectF(0, 0, m_width, m_height), m_cornerRadius, m_cornerRadius);
    }

    // 2. Setup Font
    QFont font(m_fontFamily);
    font.setPixelSize(m_pixelSize);
    font.setBold(m_bold);
    font.setItalic(m_italic);
    font.setUnderline(m_underline);
    font.setStrikeOut(m_strikeThrough);
    if (m_letterSpacing != 0.0) {
        font.setLetterSpacing(QFont::AbsoluteSpacing, m_letterSpacing);
    }
    painter->setFont(font);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setRenderHint(QPainter::Antialiasing, true);

    QFontMetrics fm(font);
    const int totalTextWidth = fm.horizontalAdvance(displayText);
    const int fontAscent = fm.ascent();

    // 3. Render Curved Text or Straight Text
    if (m_curvature != 0 && !displayText.isEmpty()) {
        const qreal spanAngle = (static_cast<qreal>(m_curvature) / 100.0) * M_PI; // -pi to +pi
        const qreal radius = std::max(20.0, totalTextWidth / std::abs(spanAngle));
        const bool isConvex = m_curvature > 0;

        // Arc center
        const QPointF center(m_width / 2.0, isConvex ? (m_height / 2.0 + radius - fontAscent / 2.0)
                                                     : (m_height / 2.0 - radius + fontAscent / 2.0));

        qreal currentDistance = 0.0;
        for (int i = 0; i < displayText.length(); ++i) {
            const QChar ch = displayText[i];
            const qreal charW = fm.horizontalAdvance(ch);
            const qreal midCharDist = currentDistance + charW / 2.0;
            currentDistance += charW;

            // Angle fraction along arc (-0.5 to +0.5 of span)
            const qreal fraction = (totalTextWidth > 0) ? (midCharDist / totalTextWidth - 0.5) : 0.0;
            const qreal charAngle = fraction * spanAngle; // angle offset from vertical

            // Calculate position on circle
            qreal theta;
            qreal rotationDeg;
            if (isConvex) {
                theta = -M_PI / 2.0 + charAngle;
                rotationDeg = charAngle * 180.0 / M_PI;
            } else {
                theta = M_PI / 2.0 - charAngle;
                rotationDeg = -charAngle * 180.0 / M_PI;
            }

            const qreal gx = center.x() + radius * std::cos(theta);
            const qreal gy = center.y() + radius * std::sin(theta);

            painter->save();
            painter->translate(gx, gy);
            painter->rotate(rotationDeg);

            // Text Effect Passes for glyph
            if (m_textEffect.type == TextEffectType::DropShadow) {
                painter->setPen(QPen(m_textEffect.color));
                painter->drawText(QPointF(-charW / 2.0 + m_textEffect.offsetX, m_textEffect.offsetY), QString(ch));
            } else if (m_textEffect.type == TextEffectType::Glow || m_textEffect.type == TextEffectType::Neon) {
                QColor glowCol = m_textEffect.color;
                glowCol.setAlpha(static_cast<int>(80 * m_textEffect.opacity));
                painter->setPen(QPen(glowCol));
                for (int g = 1; g <= 3; ++g) {
                    painter->drawText(QPointF(-charW / 2.0 - g, 0), QString(ch));
                    painter->drawText(QPointF(-charW / 2.0 + g, 0), QString(ch));
                    painter->drawText(QPointF(-charW / 2.0, -g), QString(ch));
                    painter->drawText(QPointF(-charW / 2.0, g), QString(ch));
                }
            } else if (m_textEffect.type == TextEffectType::Outline) {
                QPainterPath path;
                path.addText(QPointF(-charW / 2.0, 0), font, QString(ch));
                painter->strokePath(path, QPen(m_textEffect.color, m_textEffect.thickness));
            } else if (m_textEffect.type == TextEffectType::Hollow) {
                QPainterPath path;
                path.addText(QPointF(-charW / 2.0, 0), font, QString(ch));
                painter->strokePath(path, QPen(m_color, m_textEffect.thickness));
            }

            if (m_textEffect.type != TextEffectType::Hollow) {
                painter->setPen(QPen(m_color));
                painter->drawText(QPointF(-charW / 2.0, 0), QString(ch));
            }

            painter->restore();
        }
    } else {
        // Standard Straight Text Rendering
        const QRectF textRect(0, 0, m_width, m_height);

        // Effect: Background box
        if (m_textEffect.type == TextEffectType::Background) {
            QRectF bgRect = fm.boundingRect(textRect.toRect(), m_alignment, displayText);
            bgRect.adjust(-m_textEffect.bgPadding, -m_textEffect.bgPadding, m_textEffect.bgPadding, m_textEffect.bgPadding);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QBrush(m_textEffect.bgColor));
            painter->drawRoundedRect(bgRect, m_textEffect.bgCornerRadius, m_textEffect.bgCornerRadius);
        }

        // Effect: Drop Shadow
        if (m_textEffect.type == TextEffectType::DropShadow) {
            QRectF shadowRect = textRect.translated(m_textEffect.offsetX, m_textEffect.offsetY);
            painter->setPen(QPen(m_textEffect.color));
            painter->drawText(shadowRect, m_alignment, displayText);
        }

        // Effect: Echo
        if (m_textEffect.type == TextEffectType::Echo) {
            for (int k = m_textEffect.count; k >= 1; --k) {
                QRectF echoRect = textRect.translated(m_textEffect.offsetX * k, m_textEffect.offsetY * k);
                QColor echoCol = m_textEffect.color;
                echoCol.setAlpha(static_cast<int>(m_textEffect.color.alpha() * (0.3 + 0.7 * (1.0 - static_cast<double>(k) / (m_textEffect.count + 1)))));
                painter->setPen(QPen(echoCol));
                painter->drawText(echoRect, m_alignment, displayText);
            }
        }

        // Effect: Glow or Neon
        if (m_textEffect.type == TextEffectType::Glow || m_textEffect.type == TextEffectType::Neon) {
            QColor glowCol = m_textEffect.color;
            glowCol.setAlpha(static_cast<int>(70 * m_textEffect.opacity));
            painter->setPen(QPen(glowCol));
            for (int g = 1; g <= 3; ++g) {
                painter->drawText(textRect.translated(-g, 0), m_alignment, displayText);
                painter->drawText(textRect.translated(g, 0), m_alignment, displayText);
                painter->drawText(textRect.translated(0, -g), m_alignment, displayText);
                painter->drawText(textRect.translated(0, g), m_alignment, displayText);
            }
        }

        // Effect: Outline / Hollow / Main Text
        if (m_textEffect.type == TextEffectType::Outline) {
            QPainterPath path;
            QRectF bound = fm.boundingRect(textRect.toRect(), m_alignment, displayText);
            qreal tx = bound.left();
            qreal ty = bound.top() + fm.ascent();
            path.addText(tx, ty, font, displayText);
            painter->strokePath(path, QPen(m_textEffect.color, m_textEffect.thickness));
            painter->fillPath(path, QBrush(m_color));
        } else if (m_textEffect.type == TextEffectType::Hollow) {
            QPainterPath path;
            QRectF bound = fm.boundingRect(textRect.toRect(), m_alignment, displayText);
            qreal tx = bound.left();
            qreal ty = bound.top() + fm.ascent();
            path.addText(tx, ty, font, displayText);
            painter->strokePath(path, QPen(m_color, m_textEffect.thickness));
        } else {
            painter->setPen(QPen(m_color));
            painter->drawText(textRect, m_alignment, displayText);
        }
    }
}

void LabelComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    bool changed = false;
    if (colorStyleRef("color") == styleName || colorStyleRef("textColor") == styleName) {
        m_color = color;
        changed = true;
    }
    if (colorStyleRef("backgroundColor") == styleName) {
        m_backgroundColor = color;
        changed = true;
    }
    if (changed) {
        update();
        emit propertyChanged(this);
    }
}

QJsonObject LabelComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["text"] = m_text;
    obj["color"] = serializeColor(m_color, colorStyleRef("color").isEmpty() ? colorStyleRef("textColor") : colorStyleRef("color"));
    obj["backgroundColor"] = serializeColor(m_backgroundColor, colorStyleRef("backgroundColor"));
    obj["cornerRadius"] = m_cornerRadius;
    obj["underline"] = m_underline;
    obj["strikeThrough"] = m_strikeThrough;
    obj["casing"] = m_textCasing;
    obj["curvature"] = m_curvature;

    if (m_textEffect.type != TextEffectType::None) {
        obj["effect"] = m_textEffect.toJson();
    }

    QJsonObject fontObj;
    fontObj["family"]        = m_fontFamily;
    fontObj["pixelSize"]     = m_pixelSize;
    fontObj["bold"]          = m_bold;
    fontObj["italic"]        = m_italic;
    fontObj["letterSpacing"] = m_letterSpacing;
    fontObj["lineHeight"]    = m_lineHeight;
    obj["font"] = fontObj;

    if (m_alignment & Qt::AlignHCenter) {
        obj["alignment"] = "center";
    } else if (m_alignment & Qt::AlignRight) {
        obj["alignment"] = "right";
    } else {
        obj["alignment"] = "left";
    }

    return obj;
}

void LabelComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_text = json.value("text").toString(m_text);
    if (json.contains("color")) {
        QString ref;
        deserializeColor(json.value("color"), m_color, ref);
        setColorStyleRef("color", ref);
    }
    if (json.contains("backgroundColor")) {
        QString ref;
        deserializeColor(json.value("backgroundColor"), m_backgroundColor, ref);
        setColorStyleRef("backgroundColor", ref);
    }
    m_cornerRadius = json.value("cornerRadius").toInt(m_cornerRadius);
    m_underline = json.value("underline").toBool(false);
    m_strikeThrough = json.value("strikeThrough").toBool(false);
    m_textCasing = json.value("casing").toString("none");
    m_curvature = json.value("curvature").toInt(0);

    if (json.contains("effect") && json.value("effect").isObject()) {
        m_textEffect = TextEffect::fromJson(json.value("effect").toObject());
    } else {
        m_textEffect = TextEffect();
    }

    if (json.contains("font") && json.value("font").isObject()) {
        QJsonObject fontObj = json.value("font").toObject();
        m_fontFamily    = fontObj.value("family").toString(m_fontFamily);
        m_pixelSize     = fontObj.value("pixelSize").toInt(m_pixelSize);
        m_bold          = fontObj.value("bold").toBool(m_bold);
        m_italic        = fontObj.value("italic").toBool(m_italic);
        m_letterSpacing = fontObj.value("letterSpacing").toDouble(0.0);
        m_lineHeight    = fontObj.value("lineHeight").toInt(0);
    }
    if (json.contains("alignment")) {
        QString a = json.value("alignment").toString().toLower();
        if (a == "center") {
            m_alignment = Qt::AlignHCenter | Qt::AlignVCenter;
        } else if (a == "right") {
            m_alignment = Qt::AlignRight | Qt::AlignVCenter;
        } else {
            m_alignment = Qt::AlignLeft | Qt::AlignVCenter;
        }
    }
    update();
}

QString LabelComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1Text {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    x: %2\n").arg(indent).arg(static_cast<int>(pos().x()));
    qml += QString("%1    y: %2\n").arg(indent).arg(static_cast<int>(pos().y()));
    qml += QString("%1    width: %2\n").arg(indent).arg(static_cast<int>(m_width));
    qml += QString("%1    height: %2\n").arg(indent).arg(static_cast<int>(m_height));
    qml += QString("%1    text: \"%2\"\n").arg(indent, transformedText());
    qml += QString("%1    color: \"%2\"\n").arg(indent, m_color.name());
    qml += QString("%1    font.pixelSize: %2\n").arg(indent).arg(m_pixelSize);
    if (m_bold)          qml += QString("%1    font.bold: true\n").arg(indent);
    if (m_italic)        qml += QString("%1    font.italic: true\n").arg(indent);
    if (m_underline)     qml += QString("%1    font.underline: true\n").arg(indent);
    if (m_strikeThrough) qml += QString("%1    font.strikeout: true\n").arg(indent);
    if (!m_fontFamily.isEmpty() && m_fontFamily != "Roboto")
        qml += QString("%1    font.family: \"%2\"\n").arg(indent, m_fontFamily);
    if (m_letterSpacing != 0.0)
        qml += QString("%1    font.letterSpacing: %2\n").arg(indent).arg(m_letterSpacing, 0, 'f', 2);
    if (m_lineHeight > 0)
        qml += QString("%1    lineHeight: %2\n").arg(indent).arg(m_lineHeight / 100.0, 0, 'f', 2);
    if (m_curvature != 0)
        qml += QString("%1    // Note: Curved text (%2%) requires custom PathText component on MCU\n").arg(indent).arg(m_curvature);
    if (m_alignment & Qt::AlignHCenter) {
        qml += QString("%1    horizontalAlignment: Text.AlignHCenter\n").arg(indent);
    } else if (m_alignment & Qt::AlignRight) {
        qml += QString("%1    horizontalAlignment: Text.AlignRight\n").arg(indent);
    } else {
        qml += QString("%1    horizontalAlignment: Text.AlignLeft\n").arg(indent);
    }
    qml += QString("%1    verticalAlignment: Text.AlignVCenter\n").arg(indent);
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString LabelComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Label: %2\n").arg(indent, m_id);
    if (m_letterSpacing != 0.0 || m_lineHeight > 0 || m_curvature != 0 || m_textEffect.type != TextEffectType::None)
        code += QString("%1// Note: Advanced typography/effects/curvature exported as standard µGFX label\n").arg(indent);
    code += QString("%1wi.g.x = %2; wi.g.y = %3;\n").arg(indent).arg(static_cast<int>(pos().x())).arg(static_cast<int>(pos().y()));
    code += QString("%1wi.g.width = %2; wi.g.height = %3;\n").arg(indent).arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    code += QString("%1wi.text = \"%2\";\n").arg(indent, transformedText());
    code += QString("%1wi.customDraw = NULL;\n").arg(indent);
    code += QString("%1GHandle lbl_%2 = gwinLabelCreate(NULL, &wi);\n").arg(indent, m_id);
    if (m_alignment & Qt::AlignHCenter) {
        code += QString("%1gwinSetAlignment(lbl_%2, GJustifyCenter);\n").arg(indent, m_id);
    } else if (m_alignment & Qt::AlignRight) {
        code += QString("%1gwinSetAlignment(lbl_%2, GJustifyRight);\n").arg(indent, m_id);
    } else {
        code += QString("%1gwinSetAlignment(lbl_%2, GJustifyLeft);\n").arg(indent, m_id);
    }
    code += QString("%1gwinSetVisible(lbl_%2, gTrue);\n").arg(indent, m_id);
    return code;
}
