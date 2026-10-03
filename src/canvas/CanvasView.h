#pragma once

#include <QGraphicsView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPolygonF>
#include <QRubberBand>
#include "CanvasScene.h"
#include "PathComponent.h"

class CanvasView : public QGraphicsView {
    Q_OBJECT

public:
    explicit CanvasView(CanvasScene* scene, QWidget* parent = nullptr);

    void zoomIn();
    void zoomOut();
    void resetZoom();
    void setZoomFactor(qreal factor);
    qreal zoomFactor() const { return m_zoomFactor; }
    void setActiveDrawingTool(const QString& type);
    QString activeDrawingTool() const { return m_activeDrawingTool; }

    void setUndoStack(class QUndoStack* stack) { m_undoStack = stack; }
    class QUndoStack* undoStack() const { return m_undoStack; }

signals:
    void zoomChanged(qreal factor);
    void statusMessageRequested(const QString& msg);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    CanvasScene* m_canvasScene = nullptr;
    qreal m_zoomFactor = 1.0;
    QString m_activeDrawingTool;
    QList<PathAnchor> m_pendingPathAnchors;
    QPointF m_pathCursorScenePos;
    QPointF m_newAnchorDragStart;
    bool m_isDraggingNewPathAnchor = false;
    bool m_isEditingPathControl = false;
    bool m_editingFromAnchor = false;
    bool m_editingHandleIn = false;
    int m_pendingDragAnchorIndex = -1;
    int m_editingAnchorIndex = -1;
    PathComponent* m_editingPath = nullptr;
    bool m_isShapeDragging = false;
    QPointF m_shapeDragStart;
    bool m_isPanning = false;
    QPoint m_panStartPos;

    // Rubber-band drag-select
    bool m_isRubberBanding = false;
    QPoint m_rubberBandOrigin;
    QRubberBand* m_rubberBand = nullptr;

    void applyZoom(qreal factor);
    UIComponent* createComponentByType(const QString& compType, const QPointF& pos);
    void finishCustomPath();
    void cancelCustomPath();
    void finishShapeDrag(const QPointF& end);
    class QUndoStack* m_undoStack = nullptr;
};
