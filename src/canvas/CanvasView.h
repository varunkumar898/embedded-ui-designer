#pragma once

#include <QGraphicsView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include "CanvasScene.h"

class CanvasView : public QGraphicsView {
    Q_OBJECT

public:
    explicit CanvasView(CanvasScene* scene, QWidget* parent = nullptr);

    void zoomIn();
    void zoomOut();
    void resetZoom();
    void setZoomFactor(qreal factor);
    qreal zoomFactor() const { return m_zoomFactor; }

    void setUndoStack(class QUndoStack* stack) { m_undoStack = stack; }
    class QUndoStack* undoStack() const { return m_undoStack; }

    void setProject(class Project* project) { m_project = project; }
    class Project* project() const { return m_project; }

signals:
    void zoomChanged(qreal factor);
    void statusMessageRequested(const QString& msg);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    CanvasScene* m_canvasScene = nullptr;
    qreal m_zoomFactor = 1.0;
    bool m_isPanning = false;
    QPoint m_panStartPos;

    void applyZoom(qreal factor);
    UIComponent* createComponentByType(const QString& compType, const QPointF& pos);
    class QUndoStack* m_undoStack = nullptr;
    class Project* m_project = nullptr;
};
