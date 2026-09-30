#include "CanvasView.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include "ImageComponent.h"
#include "AddComponentCommand.h"
#include "DeleteComponentCommand.h"
#include "MoveComponentCommand.h"
#include <QUndoStack>
#include <QMenu>
#include <QMimeData>
#include <QScrollBar>
#include <QRubberBand>
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

void CanvasView::mouseReleaseEvent(QMouseEvent* event) {
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
