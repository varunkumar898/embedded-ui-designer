#include "CanvasView.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include <QMenu>
#include <QMimeData>
#include <QScrollBar>
#include <algorithm>

CanvasView::CanvasView(CanvasScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_canvasScene(scene)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setRenderHint(QPainter::TextAntialiasing);
    setAcceptDrops(true);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
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
    if (event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier))) {
        m_isPanning = true;
        m_panStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
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
    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_isPanning && (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton)) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        QList<QGraphicsItem*> sel = scene()->selectedItems();
        for (QGraphicsItem* item : sel) {
            if (auto comp = dynamic_cast<UIComponent*>(item)) {
                m_canvasScene->removeUIComponent(comp);
                delete comp;
            }
        }
        event->accept();
        return;
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
            m_canvasScene->removeUIComponent(comp);
            delete comp;
        } else if (selected == actDuplicate) {
            QJsonObject json = comp->toJson();
            UIComponent* dup = createComponentByType(comp->componentType(), comp->pos() + QPointF(20, 20));
            if (dup) {
                dup->fromJson(json);
                dup->setComponentId(comp->componentId() + "_copy");
                dup->setCompPos(comp->pos().x() + 20, comp->pos().y() + 20);
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
    }

    if (comp) {
        comp->setPos(pos);
        m_canvasScene->addUIComponent(comp);
    }
    return comp;
}
