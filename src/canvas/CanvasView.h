#pragma once

#include <QGraphicsView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPolygonF>
#include <QRubberBand>
#include <QTimer>
#include "CanvasScene.h"
#include "PathComponent.h"

#include "BoardHAL.h"
#include "OpenOCDManager.h"
class UIComponent;

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

    // Live Test Mode & OpenOCD Hardware Control
    void setOpenOCDManager(OpenOCDManager* openocd);
    bool isLiveTestMode() const { return m_liveTestMode; }
    void onComponentClicked(UIComponent* component);

    /// Selects the component and scrolls it into the viewport
    /// (used by Problems panel navigation).
    void revealComponent(UIComponent* component);

public slots:
    void setLiveTestMode(bool enabled);

signals:
    void zoomChanged(qreal factor);
    void statusMessageRequested(const QString& msg);
    void gpioStateChanged(const QString& pin, int newState, int latency);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

    static constexpr int RULER_SIZE = 20; ///< px width/height of ruler strips

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

    // ── Ruler guide drag state ──────────────────────────────────────────
    enum class GuideDragMode { None, DraggingH, DraggingV, MovingH, MovingV };
    GuideDragMode m_guideDragMode = GuideDragMode::None;
    qreal m_guideDragPos = 0.0;   ///< Current scene-coord position of guide being dragged
    qreal m_guideOrigPos = 0.0;   ///< Original position (for move operations)
    bool m_guideOnCanvas = false; ///< Whether the guide has been dragged onto the canvas

    // Snap-highlight auto-clear timer
    QTimer* m_snapHighlightTimer = nullptr;

    void applyZoom(qreal factor);
    UIComponent* createComponentByType(const QString& compType, const QPointF& pos);
    void finishCustomPath();
    void cancelCustomPath();
    void finishShapeDrag(const QPointF& end);
    void drawRulers(QPainter* painter);
    bool hitTestGuide(const QPoint& viewPos, bool* isHorizontal, qreal* guidePos) const;
    class QUndoStack* m_undoStack = nullptr;

    // NEW: Live Test Mode state and GPIO control
    bool m_liveTestMode = false;
    int m_lastLatency = 0;  // Track latency for Device Monitor

    // NEW: Pointer to OpenOCD manager
    OpenOCDManager* m_openocd = nullptr;  // Will be set from MainWindow

    // NEW: Methods to handle GPIO toggle and selection
    void handleSwitchToggle(UIComponent* switchComponent);
    void selectComponent(UIComponent* component);

    /**
     * @brief Get the board HAL from OpenOCDManager.
     * Used to validate GPIO pins and get board-specific settings.
     */
    BoardHAL* getBoardHAL() const {
        return m_openocd ? m_openocd->getBoardHAL() : nullptr;
    }
};
