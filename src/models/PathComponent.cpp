#include "PathComponent.h"
#include <QJsonArray>
#include <algorithm>
#include <cmath>

// ── Construction ───────────────────────────────────────────────────────────

PathComponent::PathComponent(const QString& id)
    : UIComponent(id, "Path")
{
    m_width = 10.0;
    m_height = 10.0;
    m_contours.append(PathContour()); // always have at least one (empty) contour
}

// ── Geometry helpers ───────────────────────────────────────────────────────

QRectF PathComponent::anchorBounds(const QList<PathAnchor>& anchors) {
    QPointF minimum, maximum;
    bool initialized = false;
    auto include = [&](const QPointF& p) {
        if (!initialized) { minimum = maximum = p; initialized = true; return; }
        minimum.setX(std::min(minimum.x(), p.x()));
        minimum.setY(std::min(minimum.y(), p.y()));
        maximum.setX(std::max(maximum.x(), p.x()));
        maximum.setY(std::max(maximum.y(), p.y()));
    };
    for (const PathAnchor& a : anchors) {
        include(a.position);
        if (a.hasHandleIn)  include(a.position + a.handleIn);
        if (a.hasHandleOut) include(a.position + a.handleOut);
    }
    return initialized ? QRectF(minimum, maximum) : QRectF();
}

void PathComponent::rebuildBounds() {
    // Collect all anchor positions across every contour to compute the
    // component's bounding rect (used by UIComponent for resize handles).
    QList<PathAnchor> all;
    for (const PathContour& c : m_contours)
        all.append(c);
    QRectF bounds = anchorBounds(all);

    const QPointF offset = bounds.topLeft();
    prepareGeometryChange();
    // Shift every anchor so the top-left is at local origin (0,0)
    for (PathContour& contour : m_contours)
        for (PathAnchor& a : contour)
            a.position -= offset;
    m_width  = std::max<qreal>(10.0, bounds.width());
    m_height = std::max<qreal>(10.0, bounds.height());
    update();
}

// ── Single-contour convenience wrappers ───────────────────────────────────

QPolygonF PathComponent::points() const {
    QPolygonF result;
    if (m_contours.isEmpty()) return result;
    const PathContour& c = m_contours.first();
    result.reserve(c.size());
    for (const PathAnchor& a : c) result.append(a.position);
    return result;
}

QList<PathAnchor> PathComponent::anchors() const {
    return m_contours.isEmpty() ? QList<PathAnchor>() : m_contours.first();
}

void PathComponent::setPoints(const QPolygonF& pts) {
    if (pts.size() < 3) return;
    QList<PathAnchor> anchors;
    anchors.reserve(pts.size());
    for (const QPointF& p : pts) anchors.append({p, {}, {}, false, false});
    setAnchors(anchors);
}

void PathComponent::setAnchors(const QList<PathAnchor>& anchors) {
    if (anchors.size() < 3) return;
    if (m_contours.isEmpty()) m_contours.append(PathContour());
    m_contours[0] = anchors;
    rebuildBounds();
    // Shift back: rebuildBounds already zeroed, we need world pos preserved.
    // (setAnchors is used during finishCustomPath where pos is set separately,
    //  so we do NOT move the item here — consistent with old behavior.)
    emit geometryChangedSignal(this);
}

void PathComponent::setAnchorHandles(int index, const QPointF& handleOut) {
    setAnchorHandle(index, false, handleOut, true);
}

void PathComponent::setAnchorHandle(int index, bool incoming, const QPointF& offset, bool symmetric) {
    if (m_contours.isEmpty()) return;
    PathContour& contour = m_contours[0];
    if (index < 0 || index >= contour.size()) return;
    PathAnchor& anchor = contour[index];
    if (incoming) {
        anchor.handleIn = offset;
        anchor.hasHandleIn = true;
        if (symmetric) { anchor.handleOut = -offset; anchor.hasHandleOut = true; }
    } else {
        anchor.handleOut = offset;
        anchor.hasHandleOut = true;
        if (symmetric) { anchor.handleIn = -offset; anchor.hasHandleIn = true; }
    }
    rebuildBounds();
    emit propertyChanged(this);
    emit geometryChangedSignal(this);
}

int PathComponent::anchorAt(const QPointF& localPos, qreal radius) const {
    if (m_contours.isEmpty()) return -1;
    const qreal r2 = radius * radius;
    const PathContour& c = m_contours.first();
    for (int i = 0; i < c.size(); ++i) {
        const QPointF d = c.at(i).position - localPos;
        if (QPointF::dotProduct(d, d) <= r2) return i;
    }
    return -1;
}

int PathComponent::handleAt(const QPointF& localPos, bool* incoming, qreal radius) const {
    if (m_contours.isEmpty()) return -1;
    const qreal r2 = radius * radius;
    const PathContour& c = m_contours.first();
    for (int i = 0; i < c.size(); ++i) {
        const PathAnchor& a = c.at(i);
        if (a.hasHandleIn) {
            const QPointF d = a.position + a.handleIn - localPos;
            if (QPointF::dotProduct(d, d) <= r2) { if (incoming) *incoming = true;  return i; }
        }
        if (a.hasHandleOut) {
            const QPointF d = a.position + a.handleOut - localPos;
            if (QPointF::dotProduct(d, d) <= r2) { if (incoming) *incoming = false; return i; }
        }
    }
    return -1;
}

// ── Multi-contour API ─────────────────────────────────────────────────────

void PathComponent::setContours(const QList<PathContour>& contours) {
    m_contours = contours;
    if (m_contours.isEmpty()) m_contours.append(PathContour());
    // Compute world-space bounding rect of ALL contours, move item, re-zero.
    QList<PathAnchor> all;
    for (const PathContour& c : m_contours) all.append(c);
    const QRectF bounds = anchorBounds(all);
    setPos(pos() + bounds.topLeft());   // preserve world top-left
    prepareGeometryChange();
    for (PathContour& c : m_contours)
        for (PathAnchor& a : c)
            a.position -= bounds.topLeft();
    m_width  = std::max<qreal>(10.0, bounds.width());
    m_height = std::max<qreal>(10.0, bounds.height());
    update();
    emit geometryChangedSignal(this);
}

// ── Flattening ─────────────────────────────────────────────────────────────

static QPointF cubicPoint(const QPointF& p0, const QPointF& p1,
                           const QPointF& p2, const QPointF& p3, qreal t) {
    const qreal u = 1.0 - t;
    return u*u*u*p0 + 3.0*u*u*t*p1 + 3.0*u*t*t*p2 + t*t*t*p3;
}

QPolygonF PathComponent::flattenAnchors(const QList<PathAnchor>& anchors, int subdivisions, bool closed) {
    QPolygonF flat;
    if (anchors.size() < 2) return flat;
    subdivisions = std::clamp(subdivisions, 1, 256);
    flat.append(anchors.first().position);
    const int segCount = closed ? anchors.size() : anchors.size() - 1;
    for (int i = 0; i < segCount; ++i) {
        const PathAnchor& s = anchors.at(i);
        const PathAnchor& e = anchors.at((i + 1) % anchors.size());
        const bool curved = s.hasHandleOut || e.hasHandleIn;
        const QPointF c1 = s.hasHandleOut ? s.position + s.handleOut : s.position;
        const QPointF c2 = e.hasHandleIn  ? e.position + e.handleIn  : e.position;
        for (int step = 1; step <= subdivisions; ++step) {
            if (closed && i == segCount - 1 && step == subdivisions) continue;
            const qreal t = static_cast<qreal>(step) / subdivisions;
            flat.append(curved ? cubicPoint(s.position, c1, c2, e.position, t)
                               : s.position + (e.position - s.position) * t);
        }
    }
    return flat;
}

QPolygonF PathComponent::flattenedPoints(int subdivisions) const {
    if (m_contours.isEmpty()) return {};
    return flattenAnchors(m_contours.first(),
                          subdivisions < 1 ? m_flattenSubdivisions : subdivisions, true);
}

QList<QPolygonF> PathComponent::flattenedContours(int subdivisions) const {
    const int subs = subdivisions < 1 ? m_flattenSubdivisions : subdivisions;
    QList<QPolygonF> result;
    result.reserve(m_contours.size());
    for (const PathContour& c : m_contours)
        result.append(flattenAnchors(c, subs, true));
    return result;
}

// ── QPainterPath ──────────────────────────────────────────────────────────

QPainterPath PathComponent::painterPath() const {
    QPainterPath path;
    path.setFillRule(Qt::OddEvenFill);
    for (const PathContour& contour : m_contours) {
        if (contour.size() < 2) continue;
        path.moveTo(contour.first().position);
        const int segCount = contour.size() >= 3 ? contour.size() : contour.size() - 1;
        for (int i = 0; i < segCount; ++i) {
            const PathAnchor& s = contour.at(i);
            const PathAnchor& e = contour.at((i + 1) % contour.size());
            if (s.hasHandleOut || e.hasHandleIn) {
                path.cubicTo(s.hasHandleOut ? s.position + s.handleOut : s.position,
                             e.hasHandleIn  ? e.position + e.handleIn  : e.position,
                             e.position);
            } else {
                path.lineTo(e.position);
            }
        }
        if (contour.size() >= 3) path.closeSubpath();
    }
    return path;
}

// ── Property setters ──────────────────────────────────────────────────────

void PathComponent::setStrokeColor(const QColor& color) {
    if (color.isValid() && m_strokeColor != color) {
        m_strokeColor = color; update(); emit propertyChanged(this);
    }
}

void PathComponent::applyColorStyle(const QString& styleName, const QColor& color) {
    if (colorStyleRef("strokeColor") == styleName) {
        m_strokeColor = color;
        update();
        emit propertyChanged(this);
    }
}

void PathComponent::setStrokeThickness(qreal thickness) {
    thickness = std::clamp(thickness, 0.5, 64.0);
    if (!qFuzzyCompare(m_strokeThickness, thickness)) {
        prepareGeometryChange(); m_strokeThickness = thickness;
        update(); emit propertyChanged(this);
    }
}

void PathComponent::setOpacityPercent(int opacity) {
    opacity = std::clamp(opacity, 0, 100);
    if (m_opacityPercent != opacity) {
        m_opacityPercent = opacity; update(); emit propertyChanged(this);
    }
}

void PathComponent::setFlattenSubdivisions(int subdivisions) {
    subdivisions = std::clamp(subdivisions, 1, 256);
    if (m_flattenSubdivisions != subdivisions) {
        m_flattenSubdivisions = subdivisions; emit propertyChanged(this);
    }
}

// ── Painting ──────────────────────────────────────────────────────────────

void PathComponent::paintComponent(QPainter* painter) {
    bool hasVisibleContour = false;
    for (const PathContour& c : m_contours)
        if (c.size() >= 3) { hasVisibleContour = true; break; }
    if (!hasVisibleContour) return;

    painter->setRenderHint(QPainter::Antialiasing, true);
    QColor color = m_strokeColor;
    color.setAlphaF(m_opacityPercent / 100.0);
    painter->setPen(QPen(color, m_strokeThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(painterPath());

    if (isSelected()) {
        // Draw handles for contour 0 (the pen-tool contour)
        painter->setPen(QPen(QColor("#168cff"), 1.0));
        painter->setBrush(QColor("#f4f8ff"));
        if (!m_contours.isEmpty()) {
            for (const PathAnchor& anchor : m_contours.first()) {
                if (anchor.hasHandleIn) {
                    const QPointF h = anchor.position + anchor.handleIn;
                    painter->drawLine(anchor.position, h);
                    painter->drawEllipse(h, 3.0, 3.0);
                }
                if (anchor.hasHandleOut) {
                    const QPointF h = anchor.position + anchor.handleOut;
                    painter->drawLine(anchor.position, h);
                    painter->drawEllipse(h, 3.0, 3.0);
                }
                painter->drawEllipse(anchor.position, 4.0, 4.0);
            }
        }
    }
}

// ── Serialization ─────────────────────────────────────────────────────────

static QJsonArray pointToArray(const QPointF& p) {
    QJsonArray a; a.append(p.x()); a.append(p.y()); return a;
}

static bool arrayToPoint(const QJsonValue& v, QPointF* out) {
    const QJsonArray a = v.toArray();
    if (a.size() != 2) return false;
    *out = QPointF(a.at(0).toDouble(), a.at(1).toDouble());
    return true;
}

static QJsonObject anchorToJson(const PathAnchor& a) {
    QJsonObject o;
    o["position"]    = pointToArray(a.position);
    o["handleIn"]    = pointToArray(a.handleIn);
    o["handleOut"]   = pointToArray(a.handleOut);
    o["hasHandleIn"] = a.hasHandleIn;
    o["hasHandleOut"]= a.hasHandleOut;
    return o;
}

static PathAnchor anchorFromJson(const QJsonObject& o) {
    PathAnchor a;
    arrayToPoint(o.value("position"), &a.position);
    arrayToPoint(o.value("handleIn"),  &a.handleIn);
    arrayToPoint(o.value("handleOut"), &a.handleOut);
    a.hasHandleIn  = o.value("hasHandleIn").toBool(false);
    a.hasHandleOut = o.value("hasHandleOut").toBool(false);
    return a;
}

QJsonObject PathComponent::toJson() const {
    QJsonObject json = UIComponent::toJson();

    // Write the new multi-contour format if more than one contour
    if (m_contours.size() > 1) {
        QJsonArray contoursJson;
        for (const PathContour& contour : m_contours) {
            QJsonArray contourJson;
            for (const PathAnchor& a : contour)
                contourJson.append(anchorToJson(a));
            contoursJson.append(contourJson);
        }
        json["contours"] = contoursJson;
    }

    // Also emit legacy "anchors" and "points" for contour 0 so older readers don't break
    if (!m_contours.isEmpty()) {
        QJsonArray anchorsJson;
        QJsonArray pointsJson;
        for (const PathAnchor& a : m_contours.first()) {
            anchorsJson.append(anchorToJson(a));
            pointsJson.append(pointToArray(a.position));
        }
        json["anchors"] = anchorsJson;
        json["points"] = pointsJson;
    }

    json["strokeColor"]       = serializeColor(m_strokeColor, colorStyleRef("strokeColor"));
    json["strokeThickness"]   = m_strokeThickness;
    json["opacity"]           = m_opacityPercent;
    json["flattenSubdivisions"] = m_flattenSubdivisions;
    return json;
}

void PathComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_contours.clear();

    // ── New multi-contour format ──────────────────────────────────────────
    const QJsonArray contoursJson = json.value("contours").toArray();
    if (!contoursJson.isEmpty()) {
        for (const QJsonValue& cv : contoursJson) {
            PathContour contour;
            for (const QJsonValue& av : cv.toArray())
                contour.append(anchorFromJson(av.toObject()));
            if (contour.size() >= 3)
                m_contours.append(contour);
        }
    }

    // ── Migration: legacy single-contour "anchors" array ─────────────────
    if (m_contours.isEmpty()) {
        PathContour contour;
        const QJsonArray anchorsJson = json.value("anchors").toArray();
        if (!anchorsJson.isEmpty()) {
            for (const QJsonValue& av : anchorsJson)
                contour.append(anchorFromJson(av.toObject()));
        } else {
            // Even older format: plain "points" flat array
            for (const QJsonValue& pv : json.value("points").toArray()) {
                PathAnchor a;
                if (arrayToPoint(pv, &a.position)) contour.append(a);
            }
        }
        if (contour.size() >= 3)
            m_contours.append(contour);
    }

    if (m_contours.isEmpty())
        m_contours.append(PathContour()); // always keep at least one slot

    if (json.contains("strokeColor")) {
        QString ref;
        deserializeColor(json.value("strokeColor"), m_strokeColor, ref);
        if (ref.isEmpty())
            m_strokeColor = QColor(json.value("strokeColor").toString(m_strokeColor.name(QColor::HexArgb)));
        setColorStyleRef("strokeColor", ref);
    }
    m_strokeThickness    = json.value("strokeThickness").toDouble(m_strokeThickness);
    m_opacityPercent     = json.value("opacity").toInt(m_opacityPercent);
    m_flattenSubdivisions = std::clamp(
        json.value("flattenSubdivisions").toInt(DefaultFlattenSubdivisions), 1, 256);
    rebuildBounds();
    update();
}

// ── QML code generation ───────────────────────────────────────────────────

QString PathComponent::toQmlSnippet(int indentSpaces) const {
    const QString indent(indentSpaces, QLatin1Char(' '));
    bool hasContent = false;
    for (const PathContour& c : m_contours) if (c.size() >= 3) { hasContent = true; break; }
    if (!hasContent) return {};

    QString qml = indent + "Shape {\n";
    qml += indent + QString("    id: %1\n").arg(componentId());
    qml += indent + QString("    x: %1; y: %2; width: %3; height: %4\n")
        .arg(static_cast<int>(pos().x())).arg(static_cast<int>(pos().y()))
        .arg(static_cast<int>(compWidth())).arg(static_cast<int>(compHeight()));

    for (const PathContour& contour : m_contours) {
        if (contour.size() < 3) continue;
        qml += indent + "    ShapePath {\n";
        qml += indent + QString("        strokeColor: \"%1\"\n").arg(m_strokeColor.name());
        qml += indent + "        fillColor: \"transparent\"\n";
        qml += indent + QString("        strokeWidth: %1\n").arg(m_strokeThickness);
        qml += indent + QString("        startX: %1; startY: %2\n")
            .arg(contour.first().position.x()).arg(contour.first().position.y());

        for (int i = 0; i < contour.size(); ++i) {
            const PathAnchor& s = contour.at(i);
            const PathAnchor& e = contour.at((i + 1) % contour.size());
            if (s.hasHandleOut || e.hasHandleIn) {
                const QPointF c1 = s.hasHandleOut ? s.position + s.handleOut : s.position;
                const QPointF c2 = e.hasHandleIn  ? e.position + e.handleIn  : e.position;
                qml += indent + QString("        PathCubic { x: %1; y: %2; control1X: %3; control1Y: %4; control2X: %5; control2Y: %6 }\n")
                    .arg(e.position.x()).arg(e.position.y())
                    .arg(c1.x()).arg(c1.y()).arg(c2.x()).arg(c2.y());
            } else {
                qml += indent + QString("        PathLine { x: %1; y: %2 }\n")
                    .arg(e.position.x()).arg(e.position.y());
            }
        }
        qml += indent + "    }\n";
    }
    qml += indent + "}\n";
    return qml;
}