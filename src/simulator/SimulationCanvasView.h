#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPointer>
#include "SimulationRuntime.h"
#include "Screen.h"

namespace Simulator {

/**
 * @brief SimulationCanvasView renders the active simulation screen without designer selection handles
 * and translates desktop mouse & touch events directly into interactive widget controls.
 */
class SimulationCanvasView : public QGraphicsView {
    Q_OBJECT

public:
    explicit SimulationCanvasView(SimulationRuntime* runtime, QWidget* parent = nullptr);
    ~SimulationCanvasView() override;

    void setZoomFactor(qreal factor);
    qreal zoomFactor() const { return m_zoomFactor; }
    void fitInViewport();

public slots:
    void refreshScreen();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    void setupScene();
    UIComponent* componentAtPoint(const QPointF& scenePos) const;

    SimulationRuntime* m_runtime = nullptr;
    QGraphicsScene* m_simScene = nullptr;
    qreal m_zoomFactor = 1.0;
    bool m_fitToWindow = false;

    QPointer<UIComponent> m_activePressedComponent;
};

} // namespace Simulator
