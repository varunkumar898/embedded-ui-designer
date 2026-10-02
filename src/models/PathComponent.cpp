#include "PathComponent.h"
#include <QJsonArray>
#include <algorithm>

PathComponent::PathComponent(const QString& id)
    : UIComponent(id, "Path")
{
    m_width = 10.0;
    m_height = 10.0;
}

void PathComponent::setPoints(const QPolygonF& points) {
    if (points.size() < 3) return;
    prepareGeometryChange();
    m_points = points;
    QRectF bounds = m_points.boundingRect();
    const QPointF offset = bounds.topLeft();
    for (QPointF& point : m_points) point -= offset;
    m_width = std::max<qreal>(10.0, bounds.width());
    m_height = std::max<qreal>(10.0, bounds.height());
    update();
    emit geometryChangedSignal(this);
}

void PathComponent::setStrokeColor(const QColor& color) {
    if (color.isValid() && m_strokeColor != color) {
        m_strokeColor = color;
        update();
        emit propertyChanged(this);
    }
}

void PathComponent::setStrokeThickness(qreal thickness) {
    thickness = std::clamp(thickness, 0.5, 64.0);
    if (!qFuzzyCompare(m_strokeThickness, thickness)) {
        prepareGeometryChange();
        m_strokeThickness = thickness;
        update();
        emit propertyChanged(this);
    }
}

void PathComponent::setOpacityPercent(int opacity) {
    opacity = std::clamp(opacity, 0, 100);
    if (m_opacityPercent != opacity) {
        m_opacityPercent = opacity;
        update();
        emit propertyChanged(this);
    }
}

void PathComponent::paintComponent(QPainter* painter) {
    if (m_points.size() < 3) return;
    painter->setRenderHint(QPainter::Antialiasing, true);
    QColor color = m_strokeColor;
    color.setAlphaF(m_opacityPercent / 100.0);
    painter->setPen(QPen(color, m_strokeThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    painter->drawPolygon(m_points);
}

QJsonObject PathComponent::toJson() const {
    QJsonObject json = UIComponent::toJson();
    QJsonArray pointsJson;
    for (const QPointF& point : m_points) {
        QJsonArray pair;
        pair.append(point.x());
        pair.append(point.y());
        pointsJson.append(pair);
    }
    json["points"] = pointsJson;
    json["strokeColor"] = m_strokeColor.name(QColor::HexArgb);
    json["strokeThickness"] = m_strokeThickness;
    json["opacity"] = m_opacityPercent;
    return json;
}

void PathComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    QPolygonF points;
    const QJsonArray pointsJson = json.value("points").toArray();
    for (const QJsonValue& pointValue : pointsJson) {
        const QJsonArray pair = pointValue.toArray();
        if (pair.size() == 2) points.append(QPointF(pair[0].toDouble(), pair[1].toDouble()));
    }
    if (points.size() >= 3) {
        m_points = points;
        QRectF bounds = m_points.boundingRect();
        for (QPointF& point : m_points) point -= bounds.topLeft();
        m_width = std::max<qreal>(10.0, bounds.width());
        m_height = std::max<qreal>(10.0, bounds.height());
    }
    m_strokeColor = QColor(json.value("strokeColor").toString(m_strokeColor.name(QColor::HexArgb)));
    m_strokeThickness = json.value("strokeThickness").toDouble(m_strokeThickness);
    m_opacityPercent = json.value("opacity").toInt(m_opacityPercent);
    update();
}

QString PathComponent::toQmlSnippet(int indentSpaces) const {
    const QString indent(indentSpaces, QLatin1Char(' '));
    return QString("%1// Path %2 remains editor-only until target polygon emission is implemented\n")
        .arg(indent, componentId());
}