#pragma once

#include <QString>
#include <QColor>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

/**
 * ComponentVariant
 *
 * Holds per-variant style overrides for a ComponentDefinition.
 * Only fill/stroke/accent colors are overridden; geometry is inherited
 * from the base definition (which is a concrete UIComponent shape).
 */
struct ComponentVariant {
    QString name;           ///< e.g. "Primary", "Danger"
    QColor  fillColor;      ///< Override fill / background color
    QColor  strokeColor;    ///< Override border / stroke color
    QColor  accentColor;    ///< Override accent color (text, thumb, etc.)
    bool    useFill   = true;
    bool    useStroke = true;
    bool    useAccent = false;

    QJsonObject toJson() const;
    static ComponentVariant fromJson(const QJsonObject& obj);
};

/**
 * ComponentDefinition
 *
 * A named reusable component template stored in the project's custom-component
 * library. It contains:
 *  - A unique ID and display name
 *  - A serialised base shape (any UIComponent JSON blob)
 *  - An ordered list of named variants, each with style overrides
 *
 * Definitions are owned by the Project and shared across all instances.
 */
class ComponentDefinition {
public:
    ComponentDefinition() = default;
    explicit ComponentDefinition(const QString& id, const QString& name);

    // ── Identity ──────────────────────────────────────────────────────────
    QString definitionId() const   { return m_id; }
    QString displayName()  const   { return m_name; }
    void setDisplayName(const QString& name) { m_name = name; }

    // ── Base shape (stored as raw JSON for round-tripping) ────────────────
    QJsonObject baseShapeJson() const            { return m_baseShapeJson; }
    void setBaseShapeJson(const QJsonObject& j)  { m_baseShapeJson = j; }

    // ── Variants ──────────────────────────────────────────────────────────
    int variantCount() const                     { return m_variants.size(); }
    const QList<ComponentVariant>& variants() const { return m_variants; }
    void addVariant(const ComponentVariant& v)   { m_variants.append(v); }
    void setVariants(const QList<ComponentVariant>& v) { m_variants = v; }
    const ComponentVariant* findVariant(const QString& name) const;

    // ── Serialisation ─────────────────────────────────────────────────────
    QJsonObject toJson() const;
    static ComponentDefinition fromJson(const QJsonObject& obj);

    // ── Default variants ─────────────────────────────────────────────────
    static QList<ComponentVariant> defaultVariants();

private:
    QString m_id;
    QString m_name;
    QJsonObject m_baseShapeJson;
    QList<ComponentVariant> m_variants;
};
