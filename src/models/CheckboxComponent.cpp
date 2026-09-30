#include "CheckboxComponent.h"
#include <QPainterPath>
#include <QFontMetrics>

CheckboxComponent::CheckboxComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "Checkbox", parent)
{
    m_width = 120.0;
    m_height = 24.0;
}

void CheckboxComponent::setText(const QString& text) {
    if (m_text != text) {
        m_text = text;
        update();
        emit propertyChanged(this);
    }
}

void CheckboxComponent::setChecked(bool checked) {
    if (m_checked != checked) {
        m_checked = checked;
        update();
        emit propertyChanged(this);
    }
}

void CheckboxComponent::setTextColor(const QColor& color) {
    if (m_textColor != color) {
        m_textColor = color;
        update();
        emit propertyChanged(this);
    }
}

void CheckboxComponent::setCheckColor(const QColor& color) {
    if (m_checkColor != color) {
        m_checkColor = color;
        update();
        emit propertyChanged(this);
    }
}

void CheckboxComponent::setBoxColor(const QColor& color) {
    if (m_boxColor != color) {
        m_boxColor = color;
        update();
        emit propertyChanged(this);
    }
}

void CheckboxComponent::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void CheckboxComponent::setOnToggledHandler(const QString& handler) {
    if (m_onToggledHandler != handler) {
        m_onToggledHandler = handler;
        emit propertyChanged(this);
    }
}

void CheckboxComponent::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (m_activeHandle == ResizeHandle::None && event->button() == Qt::LeftButton) {
        setChecked(!m_checked);
    }
    UIComponent::mousePressEvent(event);
}

void CheckboxComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    // 1. Draw check box
    qreal boxDim = qMin(m_height - 6.0, 18.0);
    qreal boxY = (m_height - boxDim) / 2.0;
    QRectF boxRect(2.0, boxY, boxDim, boxDim);

    painter->setPen(QPen(m_borderColor, 1.5));
    painter->setBrush(m_boxColor);
    painter->drawRoundedRect(boxRect, 3.0, 3.0);

    // 2. Draw Checkmark if checked
    if (m_checked) {
        painter->setPen(QPen(m_checkColor, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);

        QPainterPath checkPath;
        checkPath.moveTo(boxRect.left() + boxDim * 0.22, boxRect.top() + boxDim * 0.52);
        checkPath.lineTo(boxRect.left() + boxDim * 0.44, boxRect.top() + boxDim * 0.74);
        checkPath.lineTo(boxRect.left() + boxDim * 0.78, boxRect.top() + boxDim * 0.28);
        painter->drawPath(checkPath);
    }

    // 3. Draw text label
    painter->setPen(m_textColor);
    QFont font = painter->font();
    font.setPointSize(9);
    painter->setFont(font);

    qreal textX = boxRect.right() + 8.0;
    QRectF textRect(textX, 0, m_width - textX, m_height);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_text);
}

void CheckboxComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    bool changed = false;
    if (colorStyleRef("textColor") == styleName) {
        m_textColor = color;
        changed = true;
    }
    if (colorStyleRef("checkColor") == styleName) {
        m_checkColor = color;
        changed = true;
    }
    if (colorStyleRef("boxColor") == styleName) {
        m_boxColor = color;
        changed = true;
    }
    if (colorStyleRef("borderColor") == styleName) {
        m_borderColor = color;
        changed = true;
    }
    if (changed) {
        update();
        emit propertyChanged(this);
    }
}

QJsonObject CheckboxComponent::toJson() const {
    QJsonObject json = UIComponent::toJson();
    json["text"] = m_text;
    json["checked"] = m_checked;
    json["textColor"] = serializeColor(m_textColor, colorStyleRef("textColor"));
    json["checkColor"] = serializeColor(m_checkColor, colorStyleRef("checkColor"));
    json["boxColor"] = serializeColor(m_boxColor, colorStyleRef("boxColor"));
    json["borderColor"] = serializeColor(m_borderColor, colorStyleRef("borderColor"));
    json["onToggled"] = m_onToggledHandler;
    return json;
}

void CheckboxComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_text = json.value("text").toString("Checkbox");
    m_checked = json.value("checked").toBool(false);
    if (json.contains("textColor")) {
        QString ref;
        deserializeColor(json.value("textColor"), m_textColor, ref);
        setColorStyleRef("textColor", ref);
    }
    if (json.contains("checkColor")) {
        QString ref;
        deserializeColor(json.value("checkColor"), m_checkColor, ref);
        setColorStyleRef("checkColor", ref);
    }
    if (json.contains("boxColor")) {
        QString ref;
        deserializeColor(json.value("boxColor"), m_boxColor, ref);
        setColorStyleRef("boxColor", ref);
    }
    if (json.contains("borderColor")) {
        QString ref;
        deserializeColor(json.value("borderColor"), m_borderColor, ref);
        setColorStyleRef("borderColor", ref);
    }
    m_onToggledHandler = json.value("onToggled").toString();
}

QString CheckboxComponent::toQmlSnippet(int indentSpaces) const {
    QString ind(indentSpaces, ' ');
    QString qml;
    qml += ind + "Item {\n";
    qml += ind + QString("    id: %1\n").arg(m_id);
    qml += ind + QString("    x: %1; y: %2; width: %3; height: %4\n")
        .arg(static_cast<int>(compX())).arg(static_cast<int>(compY()))
        .arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    qml += ind + QString("    property bool checked: %1\n").arg(m_checked ? "true" : "false");
    qml += ind + "    Rectangle {\n";
    qml += ind + QString("        id: %1_box\n").arg(m_id);
    qml += ind + "        width: 18; height: 18; radius: 3\n";
    qml += ind + "        anchors.verticalCenter: parent.verticalCenter\n";
    qml += ind + QString("        color: \"%1\"\n").arg(m_boxColor.name());
    qml += ind + QString("        border.color: \"%1\"; border.width: 1\n").arg(m_borderColor.name());
    qml += ind + "        Rectangle {\n";
    qml += ind + "            width: 10; height: 10; radius: 2\n";
    qml += ind + "            anchors.centerIn: parent\n";
    qml += ind + QString("            color: \"%1\"\n").arg(m_checkColor.name());
    qml += ind + "            visible: parent.parent.checked\n";
    qml += ind + "        }\n";
    qml += ind + "    }\n";
    qml += ind + "    Text {\n";
    qml += ind + QString("        anchors.left: %1_box.right; anchors.leftMargin: 8\n").arg(m_id);
    qml += ind + "        anchors.verticalCenter: parent.verticalCenter\n";
    qml += ind + QString("        text: \"%1\"\n").arg(m_text);
    qml += ind + QString("        color: \"%1\"\n").arg(m_textColor.name());
    qml += ind + "        font.pixelSize: 13\n";
    qml += ind + "    }\n";
    qml += ind + "    MouseArea {\n";
    qml += ind + "        anchors.fill: parent\n";
    qml += ind + "        onClicked: {\n";
    qml += ind + "            parent.checked = !parent.checked;\n";
    if (!m_onToggledHandler.isEmpty()) {
        qml += ind + QString("            root.%1(parent.checked);\n").arg(m_onToggledHandler);
    }
    qml += ind + "        }\n";
    qml += ind + "    }\n";
    qml += ind + "}\n";
    return qml;
}

QString CheckboxComponent::toUgfxSnippet(int indentSpaces) const {
    QString ind(indentSpaces, ' ');
    QString code;
    code += ind + QString("// Checkbox: %1\n").arg(m_id);
    code += ind + "gwinWidgetClearInit(&wi);\n";
    code += ind + "wi.g.show = gTrue;\n";
    code += ind + QString("wi.g.x = %1; wi.g.y = %2;\n").arg(static_cast<int>(compX())).arg(static_cast<int>(compY()));
    code += ind + QString("wi.g.width = %1; wi.g.height = %2;\n").arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    code += ind + QString("wi.text = \"%1\";\n").arg(m_text);
    code += ind + QString("ghChk_%1 = gwinCheckboxCreate(0, &wi);\n").arg(m_id);
    code += ind + QString("gwinCheckboxCheck(ghChk_%1, %2);\n").arg(m_id).arg(m_checked ? "gTrue" : "gFalse");
    return code;
}
