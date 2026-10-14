#include "DesignWorkspace.h"
#include "CanvasView.h"
#include "ContextualTopToolbar.h"
#include "Project.h"
#include <QMainWindow>
#include <QScrollBar>

DesignWorkspace::DesignWorkspace(QMainWindow* parent, Project* project)
    : QWidget(parent)
    , m_project(project)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
}

DesignWorkspace::DesignWorkspace(QWidget* parent)
    : QWidget(parent)
    , m_project(nullptr)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
}

void DesignWorkspace::setCanvasView(CanvasView* view) {
    if (m_canvasView) {
        m_layout->removeWidget(m_canvasView);
    }
    m_canvasView = view;
    if (m_canvasView) {
        m_layout->addWidget(m_canvasView, 1);
    }
}

void DesignWorkspace::setTopToolbar(ContextualTopToolbar* toolbar) {
    if (m_topToolbar) {
        m_layout->removeWidget(m_topToolbar);
    }
    m_topToolbar = toolbar;
    if (m_topToolbar) {
        m_layout->insertWidget(0, m_topToolbar);
    }
}

void DesignWorkspace::syncWithProject(Project* project) {
    m_project = project;
}

qreal DesignWorkspace::currentZoom() const {
    if (m_canvasView) {
        return m_canvasView->transform().m11();
    }
    return 1.0;
}

void DesignWorkspace::setZoom(qreal factor) {
    if (m_canvasView && factor > 0.05) {
        QTransform t;
        t.scale(factor, factor);
        m_canvasView->setTransform(t);
        emit zoomChanged(factor);
    }
}

void DesignWorkspace::saveState(double& zoomLevel, int& scrollX, int& scrollY) {
    zoomLevel = currentZoom();
    if (m_canvasView) {
        if (m_canvasView->horizontalScrollBar()) {
            scrollX = m_canvasView->horizontalScrollBar()->value();
        }
        if (m_canvasView->verticalScrollBar()) {
            scrollY = m_canvasView->verticalScrollBar()->value();
        }
    }
}

void DesignWorkspace::restoreState(double zoomLevel, int scrollX, int scrollY) {
    setZoom(zoomLevel);
    if (m_canvasView) {
        if (m_canvasView->horizontalScrollBar()) {
            m_canvasView->horizontalScrollBar()->setValue(scrollX);
        }
        if (m_canvasView->verticalScrollBar()) {
            m_canvasView->verticalScrollBar()->setValue(scrollY);
        }
    }
}
