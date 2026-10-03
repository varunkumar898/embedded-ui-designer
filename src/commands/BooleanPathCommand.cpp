#include "BooleanPathCommand.h"
#include "CanvasScene.h"
#include "PathComponent.h"
#include "UIComponent.h"
#include "RectangleComponent.h"
#include "CircleComponent.h"

// Clipper2 header — fetched via CMake FetchContent (see CMakeLists.txt).
// The single-header distribution is used; it lives in clipper2/clipper.h
// after the FetchContent populates the build tree.
#include <clipper2/clipper.h>

#include <QPainterPath>
#include <cmath>

// ── Internal helpers ──────────────────────────────────────────────────────

/// Scale factor: Clipper2 works in integer coordinates. We scale by 1000 to
/// preserve sub-pixel precision (3 decimal places in scene units).
static constexpr double SCALE = 1000.0;

static Clipper2Lib::PathD polygonToClipperPath(const QPolygonF& poly) {
    Clipper2Lib::PathD path;
    path.reserve(static_cast<size_t>(poly.size()));
    for (const QPointF& p : poly)
        path.push_back({p.x() * SCALE, p.y() * SCALE});
    return path;
}

static QPolygonF clipperPathToPolygon(const Clipper2Lib::PathD& path) {
    QPolygonF poly;
    poly.reserve(static_cast<int>(path.size()));
    for (const auto& p : path)
        poly.append(QPointF(p.x / SCALE, p.y / SCALE));
    return poly;
}

/// Flatten any UIComponent to world-space polygons (one per contour).
/// For non-path types we approximate the shape as a simple rectangle/ellipse.
static QList<QPolygonF> toWorldPolygons(const UIComponent* comp) {
    QList<QPolygonF> result;
    const QPointF origin = comp->pos();
    const qreal w = comp->compWidth();
    const qreal h = comp->compHeight();

    if (const auto* path = dynamic_cast<const PathComponent*>(comp)) {
        // Reuse the existing flattening pipeline; shift to world coords.
        const auto contours = path->flattenedContours();
        result.reserve(contours.size());
        for (QPolygonF c : contours) {
            for (QPointF& p : c) p += origin;
            result.append(c);
        }
    } else if (dynamic_cast<const CircleComponent*>(comp)) {
        // Approximate circle with 64 line segments
        QPolygonF poly;
        poly.reserve(64);
        const QPointF centre = origin + QPointF(w / 2.0, h / 2.0);
        for (int i = 0; i < 64; ++i) {
            const double angle = 2.0 * M_PI * i / 64.0;
            poly.append(centre + QPointF((w / 2.0) * std::cos(angle),
                                         (h / 2.0) * std::sin(angle)));
        }
        result.append(poly);
    } else {
        // Default: axis-aligned rectangle
        result.append(QPolygonF(QRectF(origin, QSizeF(w, h))));
    }
    return result;
}

// ── Static compute ────────────────────────────────────────────────────────

PathComponent* BooleanPathCommand::compute(const QList<UIComponent*>& inputs, Operation op) {
    using namespace Clipper2Lib;

    if (inputs.size() < 2) return nullptr;

    // Subject = first shape; Clips = all remaining shapes
    const QList<QPolygonF> subjectPolys = toWorldPolygons(inputs.first());

    PathsD subject, clip;
    for (const QPolygonF& p : subjectPolys)
        subject.push_back(polygonToClipperPath(p));
    for (int i = 1; i < inputs.size(); ++i) {
        for (const QPolygonF& p : toWorldPolygons(inputs.at(i)))
            clip.push_back(polygonToClipperPath(p));
    }

    PathsD solution;
    const FillRule fr = FillRule::EvenOdd;
    switch (op) {
        case Operation::Union:
            solution = Union(subject, clip, fr);
            break;
        case Operation::Subtract:
            solution = Difference(subject, clip, fr);
            break;
        case Operation::Intersect:
            solution = Intersect(subject, clip, fr);
            break;
        case Operation::Xor:
            solution = Xor(subject, clip, fr);
            break;
    }

    if (solution.empty()) return nullptr;

    // Convert Clipper2 solution paths → PathContours in local space
    // Compute the world bounding rect of all result paths first.
    QPointF worldMin(std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    for (const PathD& p : solution)
        for (const PointD& pt : p) {
            worldMin.setX(std::min(worldMin.x(), pt.x / SCALE));
            worldMin.setY(std::min(worldMin.y(), pt.y / SCALE));
        }

    QList<PathContour> contours;
    contours.reserve(static_cast<int>(solution.size()));
    for (const PathD& clipPath : solution) {
        if (clipPath.size() < 3) continue;
        PathContour contour;
        contour.reserve(static_cast<int>(clipPath.size()));
        for (const PointD& pt : clipPath) {
            PathAnchor a;
            a.position = QPointF(pt.x / SCALE - worldMin.x(),
                                 pt.y / SCALE - worldMin.y());
            contour.append(a);
        }
        contours.append(contour);
    }

    if (contours.isEmpty()) return nullptr;

    // Build result PathComponent
    // Use the first input's ID as the seed for the new ID
    const QString newId = inputs.first()->componentId() + "_bool";
    PathComponent* result = new PathComponent(newId);
    result->setContours(contours);
    result->setCompPos(worldMin.x(), worldMin.y());

    // Inherit stroke style from first input if it is a PathComponent
    if (const auto* srcPath = dynamic_cast<const PathComponent*>(inputs.first())) {
        result->setStrokeColor(srcPath->strokeColor());
        result->setStrokeThickness(srcPath->strokeThickness());
    }

    return result;
}

// ── Command ───────────────────────────────────────────────────────────────

static const char* opName(BooleanPathCommand::Operation op) {
    switch (op) {
        case BooleanPathCommand::Operation::Union:     return "Boolean Union";
        case BooleanPathCommand::Operation::Subtract:  return "Boolean Subtract";
        case BooleanPathCommand::Operation::Intersect: return "Boolean Intersect";
        case BooleanPathCommand::Operation::Xor:       return "Boolean XOR";
    }
    return "Boolean Operation";
}

BooleanPathCommand::BooleanPathCommand(CanvasScene* scene,
                                       const QList<UIComponent*>& inputs,
                                       Operation op,
                                       QUndoCommand* parent)
    : QUndoCommand(opName(op), parent)
    , m_scene(scene)
    , m_inputs(inputs)
    , m_op(op)
{
    m_result = compute(inputs, op);
}

BooleanPathCommand::~BooleanPathCommand() {
    if (m_ownsResult) delete m_result;
    if (m_ownsInputs) {
        for (UIComponent* c : m_inputs) delete c;
    }
}

void BooleanPathCommand::redo() {
    if (!m_scene || !m_result) return;
    // Remove all input shapes from the scene
    for (UIComponent* comp : m_inputs)
        m_scene->removeUIComponent(comp);
    m_ownsInputs = true;
    // Add the result shape
    m_scene->addUIComponent(m_result);
    m_result->setSelected(true);
    m_ownsResult = false;
}

void BooleanPathCommand::undo() {
    if (!m_scene || !m_result) return;
    // Remove the result shape
    m_scene->removeUIComponent(m_result);
    m_ownsResult = true;
    // Restore all input shapes
    for (UIComponent* comp : m_inputs) {
        m_scene->addUIComponent(comp);
        comp->setSelected(true);
    }
    m_ownsInputs = false;
}
