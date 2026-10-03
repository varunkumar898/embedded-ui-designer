#include "CustomComponentInstance.h"
#include <QPainter>
#include <QJsonObject>

CustomComponentInstance::CustomComponentInstance(const QString& id,
                                                 const QString& definitionId,
                                                 QGraphicsItem* parent)
    : UIComponent(id, parent)
    , m_definitionId(definitionId)
    , m_variantName("Primary")
{
    setCompSize(100, 40);
}

void CustomComponentInstance::setActiveVariantName(const QString& name) {
    if (m_variantName != name) {
        m_variantName = name;
        update();
        emit propertyChanged();
    }
}

void CustomComponentInstance::applyVariant(const ComponentVariant& variant) {
    m_variantName = variant.name;
    if (variant.useFill)   m_effectiveFill   = variant.fillColor;
    if (variant.useStroke) m_effectiveStroke = variant.strokeColor;
    if (variant.useAccent) m_effectiveAccent = variant.accentColor;
    update();
    emit propertyChanged();
}

// ── Painting ──────────────────────────────────────────────────────────────

void CustomComponentInstance::paintComponent(QPainter* painter) {
    const qreal w = compWidth();
    const qreal h = compHeight();
    const QRectF r(0, 0, w, h);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Fill
    painter->setBrush(m_effectiveFill);
    painter->setPen(QPen(m_effectiveStroke, 1.5));
    painter->drawRoundedRect(r.adjusted(0.75, 0.75, -0.75, -0.75), 6, 6);

    // Label: definition ID + variant tag
    painter->setPen(m_effectiveAccent);
    QFont font = painter->font();
    font.setPixelSize(11);
    font.setBold(true);
    painter->setFont(font);
    const QString label = m_variantName.isEmpty()
                          ? m_definitionId
                          : QString("%1 [%2]").arg(m_definitionId, m_variantName);
    painter->drawText(r, Qt::AlignCenter, label);

    painter->restore();
}

// ── Serialisation ─────────────────────────────────────────────────────────

QJsonObject CustomComponentInstance::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["type"]         = "CustomInstance";
    obj["definitionId"] = m_definitionId;
    obj["variant"]      = m_variantName;
    return obj;
}

void CustomComponentInstance::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_definitionId = json.value("definitionId").toString();
    m_variantName  = json.value("variant").toString("Primary");
}

// ── Code generation (placeholders — real output depends on base shape) ────

QString CustomComponentInstance::toQmlSnippet(int indentSpaces) const {
    const QString indent(indentSpaces, ' ');
    return indent + QString("// CustomInstance \"%1\" variant \"%2\" — expand base shape\n")
                    .arg(m_definitionId, m_variantName);
}
QString CustomComponentInstance::toUgfxSnippet(int) const {
    return QString("// CustomInstance \"%1\" variant \"%2\" — expand base shape\n")
           .arg(m_definitionId, m_variantName);
}
QString CustomComponentInstance::toLvglSnippet(int) const {
    return QString("// CustomInstance \"%1\" variant \"%2\" — expand base shape\n")
           .arg(m_definitionId, m_variantName);
}
