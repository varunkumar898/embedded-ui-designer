#include "ComponentDefinition.h"
#include <QJsonArray>

// ── ComponentVariant ──────────────────────────────────────────────────────

QJsonObject ComponentVariant::toJson() const {
    QJsonObject obj;
    obj["name"]       = name;
    obj["fill"]       = fillColor.name(QColor::HexArgb);
    obj["stroke"]     = strokeColor.name(QColor::HexArgb);
    obj["accent"]     = accentColor.name(QColor::HexArgb);
    obj["useFill"]    = useFill;
    obj["useStroke"]  = useStroke;
    obj["useAccent"]  = useAccent;
    return obj;
}

ComponentVariant ComponentVariant::fromJson(const QJsonObject& obj) {
    ComponentVariant v;
    v.name        = obj.value("name").toString("Variant");
    v.fillColor   = QColor(obj.value("fill").toString("#1a73e8"));
    v.strokeColor = QColor(obj.value("stroke").toString("#0d47a1"));
    v.accentColor = QColor(obj.value("accent").toString("#ffffff"));
    v.useFill     = obj.value("useFill").toBool(true);
    v.useStroke   = obj.value("useStroke").toBool(true);
    v.useAccent   = obj.value("useAccent").toBool(false);
    return v;
}

// ── ComponentDefinition ───────────────────────────────────────────────────

ComponentDefinition::ComponentDefinition(const QString& id, const QString& name)
    : m_id(id), m_name(name)
{}

const ComponentVariant* ComponentDefinition::findVariant(const QString& name) const {
    for (const ComponentVariant& v : m_variants) {
        if (v.name == name) return &v;
    }
    return m_variants.isEmpty() ? nullptr : &m_variants.first();
}

QJsonObject ComponentDefinition::toJson() const {
    QJsonObject obj;
    obj["id"]        = m_id;
    obj["name"]      = m_name;
    obj["baseShape"] = m_baseShapeJson;

    QJsonArray varArr;
    for (const ComponentVariant& v : m_variants)
        varArr.append(v.toJson());
    obj["variants"]  = varArr;
    return obj;
}

ComponentDefinition ComponentDefinition::fromJson(const QJsonObject& obj) {
    ComponentDefinition def;
    def.m_id            = obj.value("id").toString();
    def.m_name          = obj.value("name").toString("Custom Component");
    def.m_baseShapeJson = obj.value("baseShape").toObject();

    for (const QJsonValue& v : obj.value("variants").toArray())
        def.m_variants.append(ComponentVariant::fromJson(v.toObject()));
    return def;
}

QList<ComponentVariant> ComponentDefinition::defaultVariants() {
    ComponentVariant primary;
    primary.name        = "Primary";
    primary.fillColor   = QColor("#1a73e8");
    primary.strokeColor = QColor("#0d47a1");
    primary.accentColor = QColor("#ffffff");
    primary.useFill = primary.useStroke = true;
    primary.useAccent = true;

    ComponentVariant secondary;
    secondary.name        = "Secondary";
    secondary.fillColor   = QColor("#34a853");
    secondary.strokeColor = QColor("#1b6e2e");
    secondary.accentColor = QColor("#ffffff");
    secondary.useFill = secondary.useStroke = true;
    secondary.useAccent = true;

    ComponentVariant danger;
    danger.name        = "Danger";
    danger.fillColor   = QColor("#ea4335");
    danger.strokeColor = QColor("#b31412");
    danger.accentColor = QColor("#ffffff");
    danger.useFill = danger.useStroke = true;
    danger.useAccent = true;

    return {primary, secondary, danger};
}
