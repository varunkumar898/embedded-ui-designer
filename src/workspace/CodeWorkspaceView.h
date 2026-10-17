#pragma once

#include <QWidget>
#include <QSplitter>
#include <QTabWidget>

class Project;
class DeviceManager;
class ProjectExplorerWidget;
class TabbedCodeEditor;
class ProblemsWidget;
class BuildOutputPanel;
class TerminalWidget;
class SerialMonitorWidget;

class CodeWorkspaceView : public QWidget {
    Q_OBJECT

public:
    explicit CodeWorkspaceView(Project* project = nullptr, DeviceManager* deviceManager = nullptr, QWidget *parent = nullptr);
    ~CodeWorkspaceView() override;

    void syncWithProject(Project* project);

    ProjectExplorerWidget* projectExplorer() const { return m_explorer; }
    TabbedCodeEditor* tabbedEditor() const { return m_editor; }
    ProblemsWidget* problemsWidget() const { return m_problems; }
    BuildOutputPanel* buildOutputPanel() const { return m_buildOutput; }
    TerminalWidget* terminalWidget() const { return m_terminal; }
    SerialMonitorWidget* serialMonitorWidget() const { return m_serialMonitor; }
    QTabWidget* bottomTabs() const { return m_bottomTabs; }

public slots:
    void openFile(const QString& filePath);
    void openDirectory(const QString& dirPath);
    void openProject(const QString& dirPath);
    void saveActiveFile();
    void syncUiCode();
    void toggleBottomPanel();
    bool isBottomPanelVisible() const;

signals:
    void fileOpened(const QString& filePath);

private slots:
    void onProblemsCountChanged(int total, int errors, int warnings);

private:
    void setupUi();

    Project* m_project = nullptr;
    DeviceManager* m_deviceManager = nullptr;

    QSplitter* m_mainSplitter = nullptr;
    QSplitter* m_rightSplitter = nullptr;

    ProjectExplorerWidget* m_explorer = nullptr;
    TabbedCodeEditor* m_editor = nullptr;
    QTabWidget* m_bottomTabs = nullptr;

    ProblemsWidget* m_problems = nullptr;
    BuildOutputPanel* m_buildOutput = nullptr;
    TerminalWidget* m_terminal = nullptr;
    SerialMonitorWidget* m_serialMonitor = nullptr;
    QMetaObject::Connection m_dirConn;
};
