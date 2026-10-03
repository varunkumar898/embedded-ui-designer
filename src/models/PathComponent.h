#pragma once

#include "UIComponent.h"
#include <QPolygonF>
#include <QPainterPath>

struct PathAnchor {
    QPointF position;
    QPointF handleIn;
    QPointF handleOut;
    bool hasHandleIn = false;
    bool hasHandleOut = false;
};

/// A contour is an ordered list of anchors that forms one closed sub-path.
/// PathComponent now stores one or more contours so that Boolean operations
/// (e.g. Subtract) can produce shapes with holes: the outer boundary is
/// contour 0 with counter-clockwise winding, hole boundaries use clockwise
/// winding and QPainterPath::OddEvenFill does the right thing automatically.
using PathContour = QList<PathAnchor>;

class PathComponent : public UIComponent {
    Q_OBJECT

public:
    explicit PathComponent(const QString& id);

    static constexpr int DefaultFlattenSubdivisions = 16;

    // ── Single-contour convenience API (used by the pen tool) ──────────────
    /// Returns the anchor positions of contour 0 as a flat polygon.
    QPolygonF points() const;
    /// Returns all anchors of contour 0.
    QList<PathAnchor> anchors() const;
    void setPoints(const QPolygonF& points);
    void setAnchors(const QList<PathAnchor>& anchors);
    void setAnchorHandles(int index, const QPointF& handleOut);
    void setAnchorHandle(int index, bool incoming, const QPointF& offset, bool symmetric = true);
    int anchorAt(const QPointF& localPosition, qreal radius = 8.0) const;
    int handleAt(const QPointF& localPosition, bool* incoming = nullptr, qreal radius = 8.0) const;

    // ── Multi-contour API (used by Boolean ops) ───────────────────────────
    int contourCount() const { return m_contours.size(); }
    const QList<PathContour>& contours() const { return m_contours; }
    void setContours(const QList<PathContour>& contours);

    // ── Shared properties ─────────────────────────────────────────────────
    QColor strokeColor() const { return m_strokeColor; }
    qreal strokeThickness() const { return m_strokeThickness; }
    int opacityPercent() const { return m_opacityPercent; }
    int flattenSubdivisions() const { return m_flattenSubdivisions; }

    void setStrokeColor(const QColor& color);
    void setStrokeThickness(qreal thickness);
    void setOpacityPercent(int opacity);
    void setFlattenSubdivisions(int subdivisions);

    // ── Geometry helpers ──────────────────────────────────────────────────
    /// Flatten one contour to a polyline using cubic subdivision.
    static QPolygonF flattenAnchors(const QList<PathAnchor>& anchors, int subdivisions, bool closed);
    /// Flatten ALL contours and return them as separate polygons.
    QList<QPolygonF> flattenedContours(int subdivisions = -1) const;
    /// Flatten contour 0 only (backward-compat helper).
    QPolygonF flattenedPoints(int subdivisions = -1) const;

    QPainterPath painterPath() const;

    static QRectF anchorBounds(const QList<PathAnchor>& anchors);

    // ── Serialization ─────────────────────────────────────────────────────
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    // ── Code generation ───────────────────────────────────────────────────
    QString toQmlSnippet(int indentSpaces = 8) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    /// All sub-paths. contour 0 is the primary/outer boundary; additional
    /// contours are holes (opposite winding under OddEvenFill rule).
    QList<PathContour> m_contours;

    QColor m_strokeColor = QColor("#e4583e");
    qreal m_strokeThickness = 3.0;
    int m_opacityPercent = 100;
    int m_flattenSubdivisions = DefaultFlattenSubdivisions;

    void rebuildBounds();
};