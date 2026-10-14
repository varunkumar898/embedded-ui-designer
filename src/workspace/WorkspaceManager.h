#pragma once

#include <QObject>
#include <QStackedWidget>
#include <QMainWindow>
#include <QStringList>
#include <QMap>
#include <memory>
#include "WorkspaceMode.h"

class DesignWorkspace;
class CodeWorkspace;
class CodeWorkspaceView;
class Project;
class QDockWidget;

enum class WorkspaceType {
    DESIGN,
    CODE
};

/**
 * @brief WorkspaceManager controls switching between the Design Workspace (Visual UI designer)
 * and Code Workspace (Firmware IDE), preserving view and editor state across switches.
 */
class WorkspaceManager : public QObject {
    Q_OBJECT

public:
    explicit WorkspaceManager(QMainWindow* parent, Project* project);
    explicit WorkspaceManager(QMainWindow* mainWindow,
                              QStackedWidget* centralStack,
                              DesignWorkspace* designWorkspace,
                              CodeWorkspaceView* codeWorkspace,
                              QObject* parent = nullptr);
    ~WorkspaceManager() override;

    // Initialize workspaces (call after construction, before show())
    void initialize();

    // Get the stacked widget to add to main window
    QStackedWidget* getContainerWidget() const;

    // Switch between workspaces
    void switchToDesign();
    void switchToCode();
    void toggle();
    void setMode(WorkspaceMode mode);

    // Get current workspace
    WorkspaceType currentWorkspace() const { return m_currentWorkspace; }
    WorkspaceMode currentMode() const {
        return m_currentWorkspace == WorkspaceType::DESIGN ? WorkspaceMode::Design : WorkspaceMode::Code;
    }

    DesignWorkspace* designWorkspace() const { return m_designWorkspace; }
    CodeWorkspace* codeWorkspace() const;

    void setProject(Project* project);
    Project* project() const { return m_project; }

    void registerDesignDock(const QString& name, QDockWidget* dock);

    // State preservation
    void saveState();
    void restoreState();

signals:
    void workspaceSwitched(WorkspaceType newWorkspace);
    void modeChanged(WorkspaceMode newMode);
    void designWorkspaceLoaded();
    void codeWorkspaceLoaded();

private:
    QMainWindow* m_mainWindow = nullptr;
    Project* m_project = nullptr;
    QStackedWidget* m_container = nullptr;

    DesignWorkspace* m_designWorkspace = nullptr;
    CodeWorkspaceView* m_codeWorkspace = nullptr;
    std::unique_ptr<DesignWorkspace> m_ownedDesignWorkspace;
    std::unique_ptr<CodeWorkspace> m_ownedCodeWorkspace;

    WorkspaceType m_currentWorkspace = WorkspaceType::DESIGN;

    struct WorkspaceState {
        double designZoomLevel = 1.0;
        int designScrollX = 0, designScrollY = 0;
        QStringList openCodeFiles;
        int activeCodeFile = 0;
    } m_state;

    QMap<QString, QDockWidget*> m_designDocks;
    QMap<QString, bool> m_savedDockVisibility;
};
