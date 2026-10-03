#include "LabelComponent.h"
#include <QFont>
#include <QPen>

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

void LabelComponent::setAlignment(Qt::Alignment align) {
    if (m_alignment != align) {
        m_alignment = align;
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

const QStringList& LabelComponent::availableFonts() {
    static const QStringList fonts = {
        "Roboto", "Roboto Mono", "Noto Sans", "Open Sans", "Lato",
        "Raleway", "Oswald", "Merriweather", "Ubuntu", "PT Sans",
        "Fira Sans", "Inter", "Source Sans Pro", "Montserrat",
        "Arial", "Helvetica", "Segoe UI", "Tahoma"
    };
    return fonts;
}

void LabelComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::TextAntialiasing);

    QFont font(m_fontFamily);
    font.setPixelSize(m_pixelSize);
    font.setBold(m_bold);
    font.setItalic(m_italic);
    if (m_letterSpacing != 0.0)
        font.setLetterSpacing(QFont::AbsoluteSpacing, m_letterSpacing);
    painter->setFont(font);

    painter->setPen(QPen(m_color));
    painter->drawText(QRectF(0, 0, m_width, m_height), m_alignment, m_text);
}

void LabelComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    if (colorStyleRef("color") == styleName || colorStyleRef("textColor") == styleName) {
        m_color = color;
        update();
        emit propertyChanged(this);
    }
}

QJsonObject LabelComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["text"] = m_text;
    obj["color"] = serializeColor(m_color, colorStyleRef("color").isEmpty() ? colorStyleRef("textColor") : colorStyleRef("color"));

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
    qml += QString("%1    text: \"%2\"\n").arg(indent, m_text);
    qml += QString("%1    color: \"%2\"\n").arg(indent, m_color.name());
    qml += QString("%1    font.pixelSize: %2\n").arg(indent).arg(m_pixelSize);
    if (m_bold)   qml += QString("%1    font.bold: true\n").arg(indent);
    if (m_italic) qml += QString("%1    font.italic: true\n").arg(indent);
    if (!m_fontFamily.isEmpty() && m_fontFamily != "Roboto")
        qml += QString("%1    font.family: \"%2\"\n").arg(indent, m_fontFamily);
    if (m_letterSpacing != 0.0)
        qml += QString("%1    font.letterSpacing: %2\n").arg(indent).arg(m_letterSpacing, 0, 'f', 2);
    if (m_lineHeight > 0)
        qml += QString("%1    lineHeight: %2\n").arg(indent).arg(m_lineHeight / 100.0, 0, 'f', 2);
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
    if (m_letterSpacing != 0.0 || m_lineHeight > 0)
        code += QString("%1// Note: letterSpacing/lineHeight not natively supported in µGFX\n").arg(indent);
    code += QString("%1wi.g.x = %2; wi.g.y = %3;\n").arg(indent).arg(static_cast<int>(pos().x())).arg(static_cast<int>(pos().y()));
    code += QString("%1wi.g.width = %2; wi.g.height = %3;\n").arg(indent).arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    code += QString("%1wi.text = \"%2\";\n").arg(indent, m_text);
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
