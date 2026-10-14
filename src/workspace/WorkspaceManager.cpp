#include "WorkspaceManager.h"
#include "DesignWorkspace.h"
#include "CodeWorkspace.h"
#include "Project.h"
#include <QMainWindow>
#include <QDockWidget>

WorkspaceManager::WorkspaceManager(QMainWindow* parent, Project* project)
    : QObject(parent)
    , m_mainWindow(parent)
    , m_project(project)
    , m_currentWorkspace(WorkspaceType::DESIGN)
{
    m_container = new QStackedWidget(parent);
}

WorkspaceManager::WorkspaceManager(QMainWindow* mainWindow,
                                   QStackedWidget* centralStack,
                                   DesignWorkspace* designWorkspace,
                                   CodeWorkspaceView* codeWorkspace,
                                   QObject* parent)
    : QObject(parent ? parent : mainWindow)
    , m_mainWindow(mainWindow)
    , m_container(centralStack)
    , m_designWorkspace(designWorkspace)
    , m_codeWorkspace(codeWorkspace)
    , m_currentWorkspace(WorkspaceType::DESIGN)
{
}

WorkspaceManager::~WorkspaceManager() = default;

void WorkspaceManager::initialize() {
    if (!m_container) {
        m_container = new QStackedWidget(m_mainWindow);
    }

    if (!m_designWorkspace) {
        m_ownedDesignWorkspace = std::make_unique<DesignWorkspace>(m_mainWindow, m_project);
        m_designWorkspace = m_ownedDesignWorkspace.get();
        m_container->addWidget(m_designWorkspace->getWidget());
    }

    if (!m_codeWorkspace) {
        m_ownedCodeWorkspace = std::make_unique<CodeWorkspace>(m_mainWindow, m_project);
        m_codeWorkspace = m_ownedCodeWorkspace.get();
        m_container->addWidget(m_ownedCodeWorkspace->getWidget());
    }

    m_container->setCurrentIndex(0);
    m_currentWorkspace = WorkspaceType::DESIGN;
    emit designWorkspaceLoaded();
}

QStackedWidget* WorkspaceManager::getContainerWidget() const {
    return m_container;
}

CodeWorkspace* WorkspaceManager::codeWorkspace() const {
    if (m_ownedCodeWorkspace) {
        return m_ownedCodeWorkspace.get();
    }
    return dynamic_cast<CodeWorkspace*>(m_codeWorkspace);
}

void WorkspaceManager::setProject(Project* project) {
    m_project = project;
    if (m_designWorkspace) {
        m_designWorkspace->syncWithProject(project);
    }
    if (m_codeWorkspace) {
        m_codeWorkspace->syncWithProject(project);
    }
}

void WorkspaceManager::registerDesignDock(const QString& name, QDockWidget* dock) {
    if (dock) {
        m_designDocks[name] = dock;
    }
}

void WorkspaceManager::switchToDesign() {
    setMode(WorkspaceMode::Design);
}

void WorkspaceManager::switchToCode() {
    setMode(WorkspaceMode::Code);
}

void WorkspaceManager::toggle() {
    if (m_currentWorkspace == WorkspaceType::DESIGN) {
        switchToCode();
    } else {
        switchToDesign();
    }
}

void WorkspaceManager::saveState() {
    if (m_designWorkspace) {
        m_designWorkspace->saveState(m_state.designZoomLevel, m_state.designScrollX, m_state.designScrollY);
    }
    if (codeWorkspace()) {
        codeWorkspace()->saveState(m_state.openCodeFiles, m_state.activeCodeFile);
    }
}

void WorkspaceManager::restoreState() {
    if (m_currentWorkspace == WorkspaceType::DESIGN) {
        if (m_designWorkspace) {
            m_designWorkspace->restoreState(m_state.designZoomLevel, m_state.designScrollX, m_state.designScrollY);
        }
    } else {
        if (codeWorkspace()) {
            codeWorkspace()->restoreState(m_state.openCodeFiles, m_state.activeCodeFile);
        }
    }
}

void WorkspaceManager::setMode(WorkspaceMode mode) {
    WorkspaceType newType = (mode == WorkspaceMode::Design) ? WorkspaceType::DESIGN : WorkspaceType::CODE;
    if (m_currentWorkspace == newType && m_container &&
        m_container->currentIndex() == (newType == WorkspaceType::DESIGN ? 0 : 1)) {
        return;
    }

    if (newType == WorkspaceType::CODE) {
        // 1. Save design state
        if (m_designWorkspace) {
            m_designWorkspace->saveState(m_state.designZoomLevel, m_state.designScrollX, m_state.designScrollY);
        }

        // 2. Save and hide design docks
        for (auto it = m_designDocks.begin(); it != m_designDocks.end(); ++it) {
            if (it.value()) {
                m_savedDockVisibility[it.key()] = it.value()->isVisible();
                it.value()->hide();
            }
        }

        // 3. Switch container to Code
        if (m_container) {
            if (m_codeWorkspace) {
                m_container->setCurrentWidget(m_codeWorkspace);
            } else if (m_container->count() > 1) {
                m_container->setCurrentIndex(1);
            }
        }

        m_currentWorkspace = WorkspaceType::CODE;

        // 4. Restore code state
        if (codeWorkspace()) {
            codeWorkspace()->restoreState(m_state.openCodeFiles, m_state.activeCodeFile);
            codeWorkspace()->syncWithProject(m_project);
        }

        emit codeWorkspaceLoaded();
    } else {
        // 1. Save code state
        if (codeWorkspace()) {
            codeWorkspace()->saveState(m_state.openCodeFiles, m_state.activeCodeFile);
        }

        // 2. Switch container to Design
        if (m_container) {
            if (m_designWorkspace) {
                m_container->setCurrentWidget(m_designWorkspace);
            } else if (m_container->count() > 0) {
                m_container->setCurrentIndex(0);
            }
        }

        // 3. Restore design docks
        for (auto it = m_savedDockVisibility.begin(); it != m_savedDockVisibility.end(); ++it) {
            if (m_designDocks.contains(it.key()) && m_designDocks[it.key()]) {
                m_designDocks[it.key()]->setVisible(it.value());
            }
        }

        m_currentWorkspace = WorkspaceType::DESIGN;

        // 4. Restore design state
        if (m_designWorkspace) {
            m_designWorkspace->restoreState(m_state.designZoomLevel, m_state.designScrollX, m_state.designScrollY);
        }

        emit designWorkspaceLoaded();
    }

    emit workspaceSwitched(m_currentWorkspace);
    emit modeChanged(mode);
}
