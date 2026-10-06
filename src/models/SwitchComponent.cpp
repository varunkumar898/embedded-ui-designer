#include "SwitchComponent.h"
#include <QPainterPath>
#include <QVariantAnimation>
#include <cmath>
#include <algorithm>

SwitchComponent::SwitchComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "Switch", parent)
{
    m_width = 50.0;
    m_height = 26.0;
}

void SwitchComponent::setChecked(bool checked) {
    if (m_transitionAnimation) {
        m_transitionAnimation->stop();
        m_transitionAnimation->deleteLater();
        m_transitionAnimation = nullptr;
    }
    m_transitionType = "Instant";
    m_transitionProgress = checked ? 1.0 : 0.0;
    if (m_checked != checked) {
        m_checked = checked;
        update();
        emit propertyChanged(this);
        emit interactionTriggered("On Change");
        emit interactionTriggered("On Value Changed");
    }
}

void SwitchComponent::animateChecked(bool checked, const QString& transition, int durationMs) {
    if (transition == "Instant" || durationMs <= 0 || m_checked == checked) {
        setChecked(checked);
        return;
    }
    if (m_transitionAnimation) {
        m_transitionAnimation->stop();
        m_transitionAnimation->deleteLater();
    }
    m_animationFromChecked = m_checked;
    m_transitionType = transition;
    m_transitionProgress = 0.0;
    m_checked = checked;
    emit propertyChanged(this);
    m_transitionAnimation = new QVariantAnimation(this);
    m_transitionAnimation->setDuration(durationMs);
    m_transitionAnimation->setStartValue(0.0);
    m_transitionAnimation->setEndValue(1.0);
    connect(m_transitionAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        m_transitionProgress = value.toReal();
        update();
    });
    connect(m_transitionAnimation, &QVariantAnimation::finished, this, [this]() {
        m_transitionProgress = 1.0;
        update();
        m_transitionAnimation->deleteLater();
        m_transitionAnimation = nullptr;
    });
    m_transitionAnimation->start();
}

void SwitchComponent::setOnColor(const QColor& color) {
    if (m_onColor != color) {
        m_onColor = color;
        update();
        emit propertyChanged(this);
    }
}

void SwitchComponent::setOffColor(const QColor& color) {
    if (m_offColor != color) {
        m_offColor = color;
        update();
        emit propertyChanged(this);
    }
}

void SwitchComponent::setThumbColor(const QColor& color) {
    if (m_thumbColor != color) {
        m_thumbColor = color;
        update();
        emit propertyChanged(this);
    }
}

void SwitchComponent::setBorderColor(const QColor& color) {
    if (m_borderColor != color) {
        m_borderColor = color;
        update();
        emit propertyChanged(this);
    }
}

void SwitchComponent::setBorderWidth(int width) {
    width = std::max(0, width);
    if (m_borderWidth != width) {
        m_borderWidth = width;
        update();
        emit propertyChanged(this);
    }
}

void SwitchComponent::setCornerRadius(int r) {
    int maxR = static_cast<int>(std::floor(std::min(m_width, m_height) / 2.0));
    r = std::clamp(r, 0, std::max(0, maxR));
    if (m_cornerRadius != r) {
        m_cornerRadius = r;
        update();
        emit propertyChanged(this);
    }
}

void SwitchComponent::setOnToggledHandler(const QString& handler) {
    if (m_onToggledHandler != handler) {
        m_onToggledHandler = handler;
        emit propertyChanged(this);
    }
}

void SwitchComponent::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (m_activeHandle == ResizeHandle::None && event->button() == Qt::LeftButton) {
        setChecked(!m_checked);
    }
    UIComponent::mousePressEvent(event);
}

void SwitchComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);
    qreal radius = m_cornerRadius > 0 ? m_cornerRadius : (m_height / 2.0);
    qreal thumbDiameter = m_height - 6.0;
    qreal thumbY = 3.0;
    const bool animating = m_transitionAnimation != nullptr;
    const qreal progress = animating ? m_transitionProgress : 1.0;
    const bool fromChecked = animating ? m_animationFromChecked : m_checked;
    const QColor fromColor = fromChecked ? m_onColor : m_offColor;
    const QColor toColor = m_checked ? m_onColor : m_offColor;

    painter->setPen(Qt::NoPen);

    if (m_transitionType == "Dissolve") {
        const qreal fromX = fromChecked ? (m_width - thumbDiameter - 3.0) : 3.0;
        const qreal toX = m_checked ? (m_width - thumbDiameter - 3.0) : 3.0;
        painter->setOpacity(1.0 - progress);
        painter->setBrush(fromColor);
        painter->drawRoundedRect(QRectF(0, 0, m_width, m_height), radius, radius);
        painter->setOpacity(progress);
        painter->setBrush(toColor);
        painter->drawRoundedRect(QRectF(0, 0, m_width, m_height), radius, radius);
        painter->setOpacity(1.0 - progress);
        painter->setBrush(m_thumbColor);
        painter->drawEllipse(QRectF(fromX, thumbY, thumbDiameter, thumbDiameter));
        painter->setOpacity(progress);
        painter->drawEllipse(QRectF(toX, thumbY, thumbDiameter, thumbDiameter));
    } else {
        const QColor blendedColor = QColor::fromRgbF(
            fromColor.redF() + (toColor.redF() - fromColor.redF()) * progress,
            fromColor.greenF() + (toColor.greenF() - fromColor.greenF()) * progress,
            fromColor.blueF() + (toColor.blueF() - fromColor.blueF()) * progress);
        painter->setOpacity(1.0);
        painter->setBrush(blendedColor);
        painter->drawRoundedRect(QRectF(0, 0, m_width, m_height), radius, radius);

        const qreal fromX = fromChecked ? (m_width - thumbDiameter - 3.0) : 3.0;
        const qreal toX = m_checked ? (m_width - thumbDiameter - 3.0) : 3.0;
        const qreal thumbX = fromX + (toX - fromX) * progress;
        painter->setBrush(m_thumbColor);
        painter->drawEllipse(QRectF(thumbX, thumbY, thumbDiameter, thumbDiameter));
    }

    // Border
    if (m_borderWidth > 0 && m_borderColor.alpha() > 0) {
        painter->setOpacity(1.0);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(m_borderColor, m_borderWidth));
        qreal inset = m_borderWidth / 2.0;
        QRectF borderRect(inset, inset, m_width - m_borderWidth, m_height - m_borderWidth);
        qreal r = std::max<qreal>(0, radius - inset);
        painter->drawRoundedRect(borderRect, r, r);
    }

    painter->setOpacity(1.0);
}

QJsonObject SwitchComponent::toJson() const {
    QJsonObject json = UIComponent::toJson();
    json["checked"] = m_checked;
    json["onColor"] = serializeColor(m_onColor, colorStyleRef("onColor"));
    json["offColor"] = serializeColor(m_offColor, colorStyleRef("offColor"));
    json["thumbColor"] = serializeColor(m_thumbColor, colorStyleRef("thumbColor"));
    json["borderColor"] = serializeColor(m_borderColor, colorStyleRef("borderColor"));
    json["borderWidth"] = m_borderWidth;
    json["cornerRadius"] = m_cornerRadius;
    json["onToggled"] = m_onToggledHandler;
    return json;
}

void SwitchComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_checked = json.value("checked").toBool(false);
    if (json.contains("onColor")) {
        QString ref;
        deserializeColor(json.value("onColor"), m_onColor, ref);
        setColorStyleRef("onColor", ref);
    }
    if (json.contains("offColor")) {
        QString ref;
        deserializeColor(json.value("offColor"), m_offColor, ref);
        setColorStyleRef("offColor", ref);
    }
    if (json.contains("thumbColor")) {
        QString ref;
        deserializeColor(json.value("thumbColor"), m_thumbColor, ref);
        setColorStyleRef("thumbColor", ref);
    }
    if (json.contains("borderColor")) {
        QString ref;
        deserializeColor(json.value("borderColor"), m_borderColor, ref);
        setColorStyleRef("borderColor", ref);
    }
    m_borderWidth = json.value("borderWidth").toInt(m_borderWidth);
    m_cornerRadius = json.value("cornerRadius").toInt(m_cornerRadius);
    m_onToggledHandler = json.value("onToggled").toString();
}

void SwitchComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    bool changed = false;
    if (colorStyleRef("onColor") == styleName) {
        m_onColor = color;
        changed = true;
    }
    if (colorStyleRef("offColor") == styleName) {
        m_offColor = color;
        changed = true;
    }
    if (colorStyleRef("thumbColor") == styleName) {
        m_thumbColor = color;
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

QString SwitchComponent::toQmlSnippet(int indentSpaces) const {
    QString ind(indentSpaces, ' ');
    QString qml;
    qml += ind + "Rectangle {\n";
    qml += ind + QString("    id: %1\n").arg(m_id);
    qml += ind + QString("    x: %1; y: %2; width: %3; height: %4\n")
        .arg(static_cast<int>(compX())).arg(static_cast<int>(compY()))
        .arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    qml += ind + QString("    property bool checked: %1\n").arg(m_checked ? "true" : "false");
    qml += ind + "    radius: height / 2\n";
    qml += ind + QString("    color: checked ? \"%1\" : \"%2\"\n")
        .arg(m_onColor.name()).arg(m_offColor.name());
    qml += ind + "    Rectangle {\n";
    qml += ind + "        width: parent.height - 6; height: width; radius: width / 2\n";
    qml += ind + "        anchors.verticalCenter: parent.verticalCenter\n";
    qml += ind + "        x: parent.checked ? parent.width - width - 3 : 3\n";
    qml += ind + QString("        color: \"%1\"\n").arg(m_thumbColor.name());
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

QString SwitchComponent::toUgfxSnippet(int indentSpaces) const {
    QString ind(indentSpaces, ' ');
    QString code;
    code += ind + QString("// Switch: %1\n").arg(m_id);
    code += ind + "gwinWidgetClearInit(&wi);\n";
    code += ind + "wi.g.show = gTrue;\n";
    code += ind + QString("wi.g.x = %1; wi.g.y = %2;\n").arg(static_cast<int>(compX())).arg(static_cast<int>(compY()));
    code += ind + QString("wi.g.width = %1; wi.g.height = %2;\n").arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    code += ind + QString("ghSwitch_%1 = gwinCheckboxCreate(0, &wi);\n").arg(m_id);
    code += ind + QString("gwinCheckboxCheck(ghSwitch_%1, %2);\n").arg(m_id).arg(m_checked ? "gTrue" : "gFalse");
    return code;
}
