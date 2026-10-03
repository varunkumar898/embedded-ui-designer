#include "CanvasView.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include "ImageComponent.h"
#include "PathComponent.h"
#include "CircleComponent.h"
#include "AddComponentCommand.h"
#include "DeleteComponentCommand.h"
#include "MoveComponentCommand.h"
#include <QUndoStack>
#include <QMenu>
#include <QMimeData>
#include <QScrollBar>
#include <QRubberBand>
#include <QLineF>
#include <algorithm>

CanvasView::CanvasView(CanvasScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_canvasScene(scene)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setRenderHint(QPainter::TextAntialiasing);
    setAcceptDrops(true);
    // Use NoDrag — we manually implement rubber-band so we can distinguish
    // empty-space drags (rubber-band) from component drags (move).
    setDragMode(QGraphicsView::NoDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_rubberBand = new QRubberBand(QRubberBand::Rectangle, viewport());
    m_rubberBand->setStyleSheet(
        "border: 1px dashed #1a96ff; background-color: rgba(26, 150, 255, 40);"
    );
}

void CanvasView::zoomIn() {
    applyZoom(1.15);
}

void CanvasView::zoomOut() {
    applyZoom(1.0 / 1.15);
}

void CanvasView::resetZoom() {
    resetTransform();
    m_zoomFactor = 1.0;
    emit zoomChanged(m_zoomFactor);
}

void CanvasView::setZoomFactor(qreal factor) {
    factor = std::clamp(factor, 0.25, 4.0);
    resetTransform();
    scale(factor, factor);
    m_zoomFactor = factor;
    emit zoomChanged(m_zoomFactor);
}

void CanvasView::setActiveDrawingTool(const QString& type) {
    m_activeDrawingTool = type;
    setCursor(type.isEmpty() ? Qt::ArrowCursor : Qt::CrossCursor);
    if (!type.isEmpty()) setFocus(Qt::OtherFocusReason);
    emit statusMessageRequested(type.isEmpty() ? "Ready" : QString("Active shape tool: %1").arg(type));
}

void CanvasView::applyZoom(qreal factor) {
    qreal newZoom = m_zoomFactor * factor;
    if (newZoom >= 0.25 && newZoom <= 4.0) {
        scale(factor, factor);
        m_zoomFactor = newZoom;
        emit zoomChanged(m_zoomFactor);
    }
}

void CanvasView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void CanvasView::mousePressEvent(QMouseEvent* event) {
    if (m_activeDrawingTool == "Custom" && event->button() == Qt::LeftButton) {
        m_pathCursorScenePos = m_canvasScene->snapPoint(mapToScene(event->pos()));
        m_isDraggingNewPathAnchor = false;
        m_pendingDragAnchorIndex = -1;

        QGraphicsItem* hitItem = itemAt(event->pos());
        while (hitItem && !dynamic_cast<PathComponent*>(hitItem)) hitItem = hitItem->parentItem();
        if (auto* path = dynamic_cast<PathComponent*>(hitItem)) {
            const QPointF localPosition = path->mapFromScene(m_pathCursorScenePos);
            bool handleIn = false;
            const int handleIndex = path->handleAt(localPosition, &handleIn);
            const int anchorIndex = handleIndex >= 0 ? handleIndex : path->anchorAt(localPosition);
            if (anchorIndex >= 0) {
                m_canvasScene->clearSelection();
                path->setSelected(true);
                m_editingPath = path;
                m_editingAnchorIndex = anchorIndex;
                m_editingHandleIn = handleIn;
                m_editingFromAnchor = handleIndex < 0;
                m_isEditingPathControl = true;
                event->accept();
                return;
            }
        }

        if (m_pendingPathAnchors.isEmpty() ||
            QLineF(m_pendingPathAnchors.last().position, m_pathCursorScenePos).length() > 1.0) {
            m_pendingPathAnchors.append({m_pathCursorScenePos, QPointF(), QPointF(), false, false});
            m_pendingDragAnchorIndex = m_pendingPathAnchors.size() - 1;
            m_newAnchorDragStart = m_pathCursorScenePos;
            m_isDraggingNewPathAnchor = true;
        }
        const QPolygonF preview = PathComponent::flattenAnchors(m_pendingPathAnchors,
                                                                PathComponent::DefaultFlattenSubdivisions, false);
        m_canvasScene->setPathPreview(preview, m_pathCursorScenePos, !m_pendingPathAnchors.isEmpty());
        event->accept();
        return;
    }

    if (!m_activeDrawingTool.isEmpty() && event->button() == Qt::LeftButton) {
        m_isShapeDragging = true;
        m_shapeDragStart = m_canvasScene->snapPoint(mapToScene(event->pos()));
        m_canvasScene->setPathPreview({m_shapeDragStart}, m_shapeDragStart, true);
        event->accept();
        return;
    }

    // Middle button or Alt+Left = pan
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier))) {
        m_isPanning = true;
        m_panStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        QGraphicsItem* hitItem = itemAt(event->pos());
        while (hitItem && !dynamic_cast<UIComponent*>(hitItem)) {
            hitItem = hitItem->parentItem();
        }
        auto hitComp = dynamic_cast<UIComponent*>(hitItem);

        bool shiftOrCtrl = (event->modifiers() & Qt::ShiftModifier) ||
                           (event->modifiers() & Qt::ControlModifier);

        if (hitComp) {
            // Shift/Ctrl-click: toggle this component in/out of selection
            if (shiftOrCtrl) {
                hitComp->setSelected(!hitComp->isSelected());
                event->accept();
                return;
            }
            // Plain click on a component:
            // If it isn't selected yet, clear others and select it, then accept
            // (don't call base class — that triggers Qt's internal selection state
            // machine which can race with our manual setSelected and clear it on
            // the subsequent mouseRelease).
            if (!hitComp->isSelected()) {
                scene()->clearSelection();
                hitComp->setSelected(true);
                event->accept();
                return;
            }
            // Already selected: fall through to base class so the item can be dragged.
            QGraphicsView::mousePressEvent(event);
            return;
        }

        // Click on empty space
        if (!shiftOrCtrl) {
            // Clear selection only if clicking empty space without modifier
            scene()->clearSelection();
        }

        // Start rubber-band
        m_isRubberBanding = true;
        m_rubberBandOrigin = event->pos();
        m_rubberBand->setGeometry(QRect(m_rubberBandOrigin, QSize()));
        m_rubberBand->show();
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseMoveEvent(QMouseEvent* event) {
    if (m_isEditingPathControl && m_editingPath && (event->buttons() & Qt::LeftButton)) {
        const QPointF localCursor = m_editingPath->mapFromScene(m_canvasScene->snapPoint(mapToScene(event->pos())));
        const PathAnchor anchor = m_editingPath->anchors().at(m_editingAnchorIndex);
        const QPointF offset = localCursor - anchor.position;
        const bool symmetric = m_editingFromAnchor || !(event->modifiers() & Qt::AltModifier);
        m_editingPath->setAnchorHandle(m_editingAnchorIndex, m_editingHandleIn, offset, symmetric);
        event->accept();
        return;
    }

    if (m_activeDrawingTool == "Custom" && m_isDraggingNewPathAnchor &&
        m_pendingDragAnchorIndex >= 0 && (event->buttons() & Qt::LeftButton)) {
        const QPointF cursor = m_canvasScene->snapPoint(mapToScene(event->pos()));
        const QPointF offset = cursor - m_newAnchorDragStart;
        if (QLineF(m_newAnchorDragStart, cursor).length() >= 2.0) {
            PathAnchor& anchor = m_pendingPathAnchors[m_pendingDragAnchorIndex];
            anchor.handleOut = offset;
            anchor.handleIn = -offset;
            anchor.hasHandleIn = true;
            anchor.hasHandleOut = true;
        }
        const QPolygonF preview = PathComponent::flattenAnchors(m_pendingPathAnchors,
                                                                PathComponent::DefaultFlattenSubdivisions, false);
        m_canvasScene->setPathPreview(preview, m_pendingPathAnchors.at(m_pendingDragAnchorIndex).position, true);
        event->accept();
        return;
    }

    if (m_isShapeDragging) {
        const QPointF cursor = m_canvasScene->snapPoint(mapToScene(event->pos()));
        m_canvasScene->setPathPreview({m_shapeDragStart}, cursor, true);
        event->accept();
        return;
    }

    if (m_activeDrawingTool == "Custom") {
        m_pathCursorScenePos = m_canvasScene->snapPoint(mapToScene(event->pos()));
        const QPolygonF preview = PathComponent::flattenAnchors(m_pendingPathAnchors,
                                                                PathComponent::DefaultFlattenSubdivisions, false);
        m_canvasScene->setPathPreview(preview, m_pathCursorScenePos, !m_pendingPathAnchors.isEmpty());
    }

    if (m_isPanning) {
        QPoint delta = event->pos() - m_panStartPos;
        m_panStartPos = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    if (m_isRubberBanding) {
        m_rubberBand->setGeometry(QRect(m_rubberBandOrigin, event->pos()).normalized());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (m_activeDrawingTool == "Custom" && event->button() == Qt::LeftButton) {
        finishCustomPath();
        event->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_isEditingPathControl && event->button() == Qt::LeftButton) {
        m_isEditingPathControl = false;
        m_editingPath = nullptr;
        m_editingAnchorIndex = -1;
        event->accept();
        return;
    }

    if (m_isDraggingNewPathAnchor && event->button() == Qt::LeftButton) {
        m_isDraggingNewPathAnchor = false;
        m_pendingDragAnchorIndex = -1;
        const QPolygonF preview = PathComponent::flattenAnchors(m_pendingPathAnchors,
                                                                PathComponent::DefaultFlattenSubdivisions, false);
        const QPointF cursor = m_pendingPathAnchors.isEmpty() ? QPointF() : m_pendingPathAnchors.last().position;
        m_canvasScene->setPathPreview(preview, cursor, !m_pendingPathAnchors.isEmpty());
        event->accept();
        return;
    }

    if (m_isShapeDragging && event->button() == Qt::LeftButton) {
        m_isShapeDragging = false;
        finishShapeDrag(m_canvasScene->snapPoint(mapToScene(event->pos())));
        event->accept();
        return;
    }

    if (m_isPanning && (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton)) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }

    if (m_isRubberBanding && event->button() == Qt::LeftButton) {
        m_isRubberBanding = false;
        m_rubberBand->hide();

        // Convert rubber-band rect from viewport to scene coordinates
        QRect viewRect = QRect(m_rubberBandOrigin, event->pos()).normalized();
        QRectF sceneRect = mapToScene(viewRect).boundingRect();

        bool shiftOrCtrl = (event->modifiers() & Qt::ShiftModifier) ||
                           (event->modifiers() & Qt::ControlModifier);

        // Select all UIComponents whose bounding rects intersect the drag rect
        for (UIComponent* comp : m_canvasScene->uiComponents()) {
            QRectF compSceneRect = comp->mapToScene(comp->boundingRect()).boundingRect();
            if (sceneRect.intersects(compSceneRect)) {
                if (shiftOrCtrl) {
                    // Toggle
                    comp->setSelected(!comp->isSelected());
                } else {
                    comp->setSelected(true);
                }
            }
        }

        event->accept();
        return;
    }

    bool shiftOrCtrl = (event->modifiers() & Qt::ShiftModifier) ||
                       (event->modifiers() & Qt::ControlModifier);
    if (shiftOrCtrl) {
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::keyPressEvent(QKeyEvent* event) {
    if (m_activeDrawingTool == "Custom" && event->key() == Qt::Key_Return) {
        finishCustomPath();
        event->accept();
        return;
    }
    if (m_activeDrawingTool == "Custom" && event->key() == Qt::Key_Escape) {
        cancelCustomPath();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        QList<UIComponent*> selComps;
        for (QGraphicsItem* item : scene()->selectedItems()) {
            if (auto comp = dynamic_cast<UIComponent*>(item)) {
                selComps.append(comp);
            }
        }
        if (!selComps.isEmpty()) {
            if (m_undoStack) {
                m_undoStack->push(new DeleteComponentCommand(m_canvasScene, selComps));
            } else {
                for (UIComponent* comp : selComps) {
                    m_canvasScene->removeUIComponent(comp);
                    delete comp;
                }
            }
            event->accept();
            return;
        }
    }

    // Arrow keys nudge: 1px default, 10px with Shift
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right ||
        event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
        qreal step = (event->modifiers() & Qt::ShiftModifier) ? 10.0 : 1.0;
        qreal dx = 0.0, dy = 0.0;
        if (event->key() == Qt::Key_Left) dx = -step;
        else if (event->key() == Qt::Key_Right) dx = step;
        else if (event->key() == Qt::Key_Up) dy = -step;
        else if (event->key() == Qt::Key_Down) dy = step;

        QList<UIComponent*> selComps;
        for (QGraphicsItem* item : scene()->selectedItems()) {
            if (auto comp = dynamic_cast<UIComponent*>(item)) {
                selComps.append(comp);
            }
        }

        if (!selComps.isEmpty()) {
            QList<MoveComponentCommand::MoveEntry> moves;
            for (UIComponent* comp : selComps) {
                QPointF oldPos = comp->pos();
                QPointF newPos = oldPos + QPointF(dx, dy);
                moves.append({comp, oldPos, newPos});
            }
            if (m_undoStack) {
                m_undoStack->push(new MoveComponentCommand(moves));
            } else {
                for (const auto& m : moves) {
                    m.comp->setCompPos(m.newPos.x(), m.newPos.y());
                }
            }
            event->accept();
            return;
        }
    }

    QGraphicsView::keyPressEvent(event);
}

void CanvasView::finishCustomPath() {
    if (m_pendingPathAnchors.size() >= 3) {
        const QRectF bounds = PathComponent::anchorBounds(m_pendingPathAnchors);
        QList<PathAnchor> localAnchors = m_pendingPathAnchors;
        for (PathAnchor& anchor : localAnchors) anchor.position -= bounds.topLeft();
        PathComponent* path = new PathComponent("path_new");
        path->setCompPos(bounds.left(), bounds.top());
        path->setAnchors(localAnchors);
        m_canvasScene->clearSelection();
        if (m_undoStack) {
            m_undoStack->push(new AddComponentCommand(m_canvasScene, path));
        } else {
            m_canvasScene->addUIComponent(path);
            path->setSelected(true);
        }
        emit statusMessageRequested(QString("Added custom path with %1 anchors").arg(m_pendingPathAnchors.size()));
    }
    m_pendingPathAnchors.clear();
    m_isDraggingNewPathAnchor = false;
    m_pendingDragAnchorIndex = -1;
    m_isEditingPathControl = false;
    m_editingPath = nullptr;
    m_canvasScene->setPathPreview({}, QPointF(), false);
    setActiveDrawingTool(QString());
}

void CanvasView::cancelCustomPath() {
    m_pendingPathAnchors.clear();
    m_isDraggingNewPathAnchor = false;
    m_pendingDragAnchorIndex = -1;
    m_isEditingPathControl = false;
    m_editingPath = nullptr;
    m_canvasScene->setPathPreview({}, QPointF(), false);
    setActiveDrawingTool(QString());
}

void CanvasView::finishShapeDrag(const QPointF& end) {
    QRectF bounds(m_shapeDragStart, end);
    bounds = bounds.normalized();
    if (bounds.width() < 10.0 && bounds.height() < 10.0) {
        bounds.setSize(QSizeF(60.0, 60.0));
    }
    if (m_activeDrawingTool == "Square" || m_activeDrawingTool == "Circle") {
        const qreal side = std::max<qreal>(10.0, std::min(bounds.width(), bounds.height()));
        bounds.setSize(QSizeF(side, side));
    }

    UIComponent* component = nullptr;
    const QString id = m_activeDrawingTool.toLower() + "_new";
    if (m_activeDrawingTool == "Circle") {
        auto* circle = new CircleComponent(id);
        circle->setCompSize(bounds.width(), bounds.height());
        component = circle;
    } else if (m_activeDrawingTool == "Triangle") {
        auto* path = new PathComponent(id);
        path->setPoints(QPolygonF({QPointF(bounds.width() / 2.0, 0),
                                   QPointF(bounds.width(), bounds.height()),
                                   QPointF(0, bounds.height())}));
        component = path;
    } else {
        auto* rectangle = new RectangleComponent(id);
        rectangle->setCompSize(std::max<qreal>(10.0, bounds.width()), std::max<qreal>(10.0, bounds.height()));
        component = rectangle;
    }

    if (component) {
        component->setCompPos(bounds.left(), bounds.top());
        m_canvasScene->clearSelection();
        if (m_undoStack) m_undoStack->push(new AddComponentCommand(m_canvasScene, component));
        else m_canvasScene->addUIComponent(component);
        component->setSelected(true);
    }
    m_canvasScene->setPathPreview({}, QPointF(), false);
    setActiveDrawingTool(QString());
}

void CanvasView::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("application/x-embedded-ui-component")) {
        event->acceptProposedAction();
    } else {
        QGraphicsView::dragEnterEvent(event);
    }
}

void CanvasView::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasFormat("application/x-embedded-ui-component")) {
        event->acceptProposedAction();
    } else {
        QGraphicsView::dragMoveEvent(event);
    }
}

void CanvasView::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasFormat("application/x-embedded-ui-component")) {
        QString compType = QString::fromUtf8(event->mimeData()->data("application/x-embedded-ui-component"));
        QPointF scenePos = mapToScene(event->position().toPoint());
        QPointF snapped = m_canvasScene->snapPoint(scenePos);

        UIComponent* comp = createComponentByType(compType, snapped);
        if (comp) {
            scene()->clearSelection();
            if (m_undoStack) {
                m_undoStack->push(new AddComponentCommand(m_canvasScene, comp));
            } else {
                m_canvasScene->addUIComponent(comp);
            }
            comp->setSelected(true);
            emit statusMessageRequested(QString("Added %1 component (%2)").arg(compType, comp->componentId()));
        }
        event->acceptProposedAction();
    } else {
        QGraphicsView::dropEvent(event);
    }
}

void CanvasView::contextMenuEvent(QContextMenuEvent* event) {
    QGraphicsItem* item = itemAt(event->pos());
    auto comp = dynamic_cast<UIComponent*>(item);

    QMenu menu(this);
    if (comp) {
        comp->setSelected(true);
        QAction* actDuplicate = menu.addAction("Duplicate");
        QAction* actToFront = menu.addAction("Bring to Front");
        QAction* actToBack = menu.addAction("Send to Back");
        menu.addSeparator();
        QAction* actDelete = menu.addAction("Delete");

        QAction* selected = menu.exec(event->globalPos());
        if (selected == actDelete) {
            if (m_undoStack) {
                m_undoStack->push(new DeleteComponentCommand(m_canvasScene, {comp}));
            } else {
                m_canvasScene->removeUIComponent(comp);
                delete comp;
            }
        } else if (selected == actDuplicate) {
            QJsonObject json = comp->toJson();
            UIComponent* dup = createComponentByType(comp->componentType(), comp->pos() + QPointF(20, 20));
            if (dup) {
                dup->fromJson(json);
                dup->setComponentId(comp->componentId() + "_copy");
                dup->setCompPos(comp->pos().x() + 20, comp->pos().y() + 20);
                if (m_undoStack) {
                    m_undoStack->push(new AddComponentCommand(m_canvasScene, dup));
                } else {
                    m_canvasScene->addUIComponent(dup);
                }
                scene()->clearSelection();
                dup->setSelected(true);
            }
        } else if (selected == actToFront) {
            qreal maxZ = 0.0;
            for (auto c : m_canvasScene->uiComponents()) {
                maxZ = std::max(maxZ, c->zValue());
            }
            comp->setZValue(maxZ + 1.0);
        } else if (selected == actToBack) {
            qreal minZ = 0.0;
            for (auto c : m_canvasScene->uiComponents()) {
                minZ = std::min(minZ, c->zValue());
            }
            comp->setZValue(minZ - 1.0);
        }
    } else {
        QAction* actZoomReset = menu.addAction("Reset Zoom (100%)");
        QAction* selected = menu.exec(event->globalPos());
        if (selected == actZoomReset) {
            resetZoom();
        }
    }
}

UIComponent* CanvasView::createComponentByType(const QString& compType, const QPointF& pos) {
    static int idCounter = 1;
    QString id = QString("%1_%2").arg(compType.toLower()).arg(idCounter++);

    UIComponent* comp = nullptr;
    if (compType == "Button") {
        comp = new ButtonComponent(id);
    } else if (compType == "Text" || compType == "Label") {
        comp = new LabelComponent(id);
    } else if (compType == "Rectangle") {
        comp = new RectangleComponent(id);
    } else if (compType == "ProgressBar") {
        comp = new ProgressBarComponent(id);
    } else if (compType == "Image") {
        comp = new ImageComponent(id);
    }

    if (comp) {
        comp->setPos(pos);
    }
    return comp;
}
