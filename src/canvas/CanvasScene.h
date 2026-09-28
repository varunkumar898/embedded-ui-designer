#pragma once

#include <QGraphicsScene>
#include <QColor>
#include <QList>
#include "DisplayConfig.h"
#include "UIComponent.h"

class CanvasScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit CanvasScene(QObject* parent = nullptr);

    void setDisplayConfig(const DisplayConfig& config);
    DisplayConfig displayConfig() const { return m_displayConfig; }

    QRectF displayRect() const;

    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }

    void setSnapToGrid(bool snap);
    bool isSnapToGrid() const { return m_snapToGrid; }

    void setGridSize(int size);
    int gridSize() const { return m_gridSize; }

    void setScreenBackgroundColor(const QColor& color);
    QColor screenBackgroundColor() const { return m_screenBackgroundColor; }

    void addUIComponent(UIComponent* comp);
    void removeUIComponent(UIComponent* comp);
    QList<UIComponent*> uiComponents() const;
    void clearComponents();

    QPointF snapPoint(const QPointF& pt) const;

signals:
    void componentSelected(UIComponent* comp);
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
};
