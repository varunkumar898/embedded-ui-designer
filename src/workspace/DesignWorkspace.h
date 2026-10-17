#pragma once

#include <QWidget>
#include <QVBoxLayout>

class QMainWindow;
class CanvasView;
class CanvasScene;
class ContextualTopToolbar;
class Project;

/**
 * @brief DesignWorkspace wraps the visual design canvas, scene, contextual toolbars,
 * and view controls for the embedded UI designer.
 */
class DesignWorkspace : public QWidget {
    Q_OBJECT

public:
    explicit DesignWorkspace(QMainWindow* parent = nullptr, Project* project = nullptr);
    explicit DesignWorkspace(QWidget* parent);
    ~DesignWorkspace() override = default;

    QWidget* getWidget() const { return const_cast<DesignWorkspace*>(this); }

    void setCanvasView(CanvasView* view);
    CanvasView* canvasView() const { return m_canvasView; }

    void setTopToolbar(ContextualTopToolbar* toolbar);
    ContextualTopToolbar* topToolbar() const { return m_topToolbar; }

    void syncWithProject(Project* project);

    void setBoardBanner(QWidget* banner);
    QWidget* boardBanner() const { return m_boardBanner; }

    // Zoom and view state preservation
    qreal currentZoom() const;
    void setZoom(qreal factor);

    void saveState(double& zoomLevel, int& scrollX, int& scrollY);
    void restoreState(double zoomLevel, int scrollX, int scrollY);

signals:
    void zoomChanged(qreal factor);

private:
    QVBoxLayout* m_layout = nullptr;
    CanvasView* m_canvasView = nullptr;
    ContextualTopToolbar* m_topToolbar = nullptr;
    QWidget* m_boardBanner = nullptr;
    Project* m_project = nullptr;
};
