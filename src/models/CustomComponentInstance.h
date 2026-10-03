#pragma once

#include "UIComponent.h"
#include "ComponentDefinition.h"
#include <QString>
#include <QColor>
#include <QJsonObject>

/**
 * CustomComponentInstance
 *
 * A live canvas item that references a ComponentDefinition by ID and renders
 * itself using the definition's base shape geometry plus the selected variant's
 * color overrides. Shape/geometry is stored locally so the instance can be
 * independently moved/resized; style comes from the variant.
 *
 * If the referenced definition is not found (e.g., after load without the
 * project's custom library), the instance falls back to a plain rectangle
 * with a warning label.
 */
class CustomComponentInstance : public UIComponent {
    Q_OBJECT

public:
    explicit CustomComponentInstance(const QString& id,
                                     const QString& definitionId = QString(),
                                     QGraphicsItem* parent = nullptr);

    // ── Definition link ───────────────────────────────────────────────────
    QString definitionId() const   { return m_definitionId; }
    void setDefinitionId(const QString& id) { m_definitionId = id; }

    // ── Variant ───────────────────────────────────────────────────────────
    QString activeVariantName() const           { return m_variantName; }
    void    setActiveVariantName(const QString& name);

    /// Apply the colors from the given variant.
    void applyVariant(const ComponentVariant& variant);

    // ── Effective colors (after variant override) ─────────────────────────
    QColor effectiveFillColor()   const { return m_effectiveFill; }
    QColor effectiveStrokeColor() const { return m_effectiveStroke; }

    // ── UIComponent interface ─────────────────────────────────────────────
    // componentType() is non-virtual, set to "CustomInstance" in constructor
    QJsonObject toJson()    const override;
    void fromJson(const QJsonObject& json)  override;
    QString toQmlSnippet(int indentSpaces = 8)   const override;
    QString toUgfxSnippet(int indentSpaces = 4)  const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    QString m_definitionId;
    QString m_variantName  = "Primary";
    QColor  m_effectiveFill   = QColor("#1a73e8");
    QColor  m_effectiveStroke = QColor("#0d47a1");
    QColor  m_effectiveAccent = QColor("#ffffff");
};
