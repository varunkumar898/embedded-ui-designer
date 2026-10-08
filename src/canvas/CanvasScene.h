#pragma once

#include <QGraphicsScene>
#include <QColor>
#include <QList>
#include <QLineF>
#include "DisplayConfig.h"
#include "UIComponent.h"

class CanvasScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit CanvasScene(QObject* parent = nullptr);
    ~CanvasScene() override;

    void setDisplayConfig(const DisplayConfig& config);
    DisplayConfig displayConfig() const { return m_displayConfig; }

    QRectF displayRect() const;

    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }

    void setSnapToGrid(bool snap);
    bool isSnapToGrid() const { return m_snapToGrid; }

    void setGridSize(int size);
    int gridSize() const { return m_gridSize; }

    // ── Ruler guides ──────────────────────────────────────────────────────
    void addHGuide(qreal y);          ///< Add a horizontal guide at scene y
    void addVGuide(qreal x);          ///< Add a vertical guide at scene x
    void removeHGuide(qreal y);       ///< Remove the nearest horizontal guide
    void removeVGuide(qreal x);       ///< Remove the nearest vertical guide
    void clearGuides();
    const QList<qreal>& hGuides() const { return m_hGuides; }
    const QList<qreal>& vGuides() const { return m_vGuides; }
    void setGuides(const QList<qreal>& hg, const QList<qreal>& vg);

    // ── Snap-highlight (one-frame feedback line drawn in drawForeground) ──
    void showSnapHighlight(const QLineF& line); ///< Set and arm the highlight
    void clearSnapHighlight();

    void setScreenBackgroundColor(const QColor& color);
    QColor screenBackgroundColor() const { return m_screenBackgroundColor; }

    void addUIComponent(UIComponent* comp);
    void removeUIComponent(UIComponent* comp);
    QList<UIComponent*> uiComponents() const;
    void clearComponents();
    void detachAllComponents();


    QPointF snapPoint(const QPointF& pt, UIComponent* ignore = nullptr) const;
    void setPathPreview(const QList<QPointF>& points, const QPointF& cursor, bool visible);

    void setUndoStack(class QUndoStack* stack) { m_undoStack = stack; }
    class QUndoStack* undoStack() const { return m_undoStack; }

signals:
    void componentSelected(UIComponent* comp);
    void selectionListChanged(const QList<UIComponent*>& selected);
    void componentAdded(UIComponent* comp);
    void componentRemoved(UIComponent* comp);
    void componentChanged(UIComponent* comp);

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;

private slots:
    void onSelectionChanged();
    void onComponentGeometryChanged(UIComponent* comp);

private:
    DisplayConfig m_displayConfig;
    QColor m_screenBackgroundColor = Qt::white;
    bool m_gridVisible = true;
    bool m_snapToGrid = true;
    int m_gridSize = 10;
    QList<QPointF> m_pathPreviewPoints;
    QPointF m_pathPreviewCursor;
    bool m_pathPreviewVisible = false;
    class QUndoStack* m_undoStack = nullptr;

    // Ruler guides
    QList<qreal> m_hGuides;   ///< Horizontal guide y-positions (scene coords)
    QList<qreal> m_vGuides;   ///< Vertical guide x-positions (scene coords)

    // Snap-highlight (one-shot: cleared after first drawForeground call)
    QLineF m_snapHighlightLine;
    bool m_snapHighlightActive = false;
};
