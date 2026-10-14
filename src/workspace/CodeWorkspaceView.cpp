#include "CodeWorkspaceView.h"
#include "ProjectExplorerWidget.h"
#include "TabbedCodeEditor.h"
#include "ProblemsWidget.h"
#include "TerminalWidget.h"
#include "SerialMonitorWidget.h"
#include "panels/BuildOutputPanel.h"
#include "project/Project.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

CodeWorkspaceView::CodeWorkspaceView(Project* project, DeviceManager* deviceManager, QWidget *parent)
    : QWidget(parent)
    , m_project(project)
    , m_deviceManager(deviceManager)
{
    setupUi();
    syncWithProject(project);
}

CodeWorkspaceView::~CodeWorkspaceView() {
}

void CodeWorkspaceView::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Main Horizontal Splitter (Left: Project Explorer, Right: Editor + Bottom Panel)
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setHandleWidth(2);
    m_mainSplitter->setStyleSheet(QString(
        "QSplitter::handle { background-color: %1; } "
    ).arg(Theme::BorderSubtle));

    // Left Panel: Project Explorer
    m_explorer = new ProjectExplorerWidget(m_mainSplitter);
    m_mainSplitter->addWidget(m_explorer);

    // Right Vertical Splitter (Top: Tabbed Editor, Bottom: Bottom Panels)
    m_rightSplitter = new QSplitter(Qt::Vertical, m_mainSplitter);
    m_rightSplitter->setObjectName("rightVerticalSplitter");
    m_rightSplitter->setHandleWidth(5);
    m_rightSplitter->setStyleSheet(QString(
        "QSplitter::handle:vertical { background-color: %1; height: 5px; } "
        "QSplitter::handle:vertical:hover { background-color: %2; } "
    ).arg(Theme::BorderSubtle, Theme::Accent));

    // Top: Tabbed Code Editor
    m_editor = new TabbedCodeEditor(m_rightSplitter);
    m_rightSplitter->addWidget(m_editor);

    // Bottom: Tab Widget (Problems | Build Output | Terminal | Serial Monitor)
    m_bottomTabs = new QTabWidget(m_rightSplitter);
    m_bottomTabs->setDocumentMode(true);
    m_bottomTabs->setStyleSheet(QString(
        "QTabWidget::pane { border: 1px solid %1; background: %2; } "
        "QTabBar::tab { "
        "  background: %3; "
        "  color: %4; "
        "  border: 1px solid %1; "
        "  border-bottom: none; "
        "  padding: 5px 12px; "
        "  font-weight: 500; "
        "  font-size: 11px; "
        "} "
        "QTabBar::tab:selected { "
        "  background: %2; "
        "  color: %5; "
        "  border-top: 2px solid %6; "
        "  font-weight: bold; "
        "} "
    ).arg(Theme::BorderSubtle, Theme::BgBase, Theme::BgRaised, Theme::TextMuted,
          Theme::TextPrimary, Theme::Accent));

    // Tab 0: Problems
    m_problems = new ProblemsWidget(m_bottomTabs);
    m_bottomTabs->addTab(m_problems, "Problems");

    // Tab 1: Build Output
    m_buildOutput = new BuildOutputPanel(m_project, m_bottomTabs);
    m_bottomTabs->addTab(m_buildOutput, "Build Output");

    // Tab 2: Terminal
    m_terminal = new TerminalWidget(m_bottomTabs);
    m_bottomTabs->addTab(m_terminal, "Terminal");

    // Tab 3: Serial Monitor
    m_serialMonitor = new SerialMonitorWidget(m_bottomTabs);
    m_bottomTabs->addTab(m_serialMonitor, "Serial Monitor");

    m_rightSplitter->addWidget(m_bottomTabs);
    m_rightSplitter->setStretchFactor(0, 1);
    m_rightSplitter->setStretchFactor(1, 0);
    m_rightSplitter->setCollapsible(0, false);
    m_rightSplitter->setCollapsible(1, true);

    m_mainSplitter->addWidget(m_rightSplitter);

    mainLayout->addWidget(m_mainSplitter);

    // Set initial splitter proportions:
    // Left: ~240px, Right: ~860px
    m_mainSplitter->setSizes({240, 860});
    // Top editor: ~500px, Bottom panel: ~250px
    m_rightSplitter->setSizes({500, 250});

    // Wire Signals
    connect(m_explorer, &ProjectExplorerWidget::fileActivated, this, &CodeWorkspaceView::openFile);
    connect(m_explorer, &ProjectExplorerWidget::syncRequested, this, &CodeWorkspaceView::syncUiCode);
    connect(m_explorer, &ProjectExplorerWidget::firmwareDirectorySelected, this, [this](const QString& dir) {
        if (m_project) {
            m_project->setFirmwareDirectory(dir);
        }
        m_terminal->setWorkingDirectory(dir);
    });
    connect(m_editor, &TabbedCodeEditor::fileModified, m_explorer, &ProjectExplorerWidget::onFileDirtyChanged);
    connect(m_editor, &TabbedCodeEditor::currentFileChanged, m_terminal, &TerminalWidget::setActiveFile);
    connect(m_problems, &ProblemsWidget::problemDoubleClicked, m_editor, &TabbedCodeEditor::gotoLocation);
    connect(m_problems, &ProblemsWidget::countsChanged, this, &CodeWorkspaceView::onProblemsCountChanged);

    if (m_buildOutput) {
        connect(m_buildOutput, &BuildOutputPanel::fileNavigationRequested, m_editor, &TabbedCodeEditor::gotoLocation);

        BuildRunner* runner = m_buildOutput->runner();
        if (runner) {
            connect(runner, &BuildRunner::buildStarted, m_problems, &ProblemsWidget::clearDiagnostics);
            connect(runner, &BuildRunner::diagnosticFound, m_problems, &ProblemsWidget::addDiagnostic);
        }
    }
}

void CodeWorkspaceView::openFile(const QString& filePath) {
    if (m_editor->openFile(filePath)) {
        m_terminal->setActiveFile(filePath);
        emit fileOpened(filePath);
    }
}

void CodeWorkspaceView::openDirectory(const QString& dirPath) {
    if (m_explorer) {
        m_explorer->setRootDirectory(dirPath);
    }
    if (m_terminal) {
        m_terminal->setWorkingDirectory(dirPath);
        m_terminal->setProjectRoot(dirPath);
    }
}

void CodeWorkspaceView::openProject(const QString& dirPath) {
    openDirectory(dirPath);
}

void CodeWorkspaceView::saveActiveFile() {
    if (m_editor) {
        m_editor->saveCurrentFile();
    }
}

void CodeWorkspaceView::syncUiCode() {
    if (!m_project) return;

    QString errorMsg;
    // Export/sync without overwriting user files
    bool ok = m_project->syncFirmwareFiles(&errorMsg);
    if (ok) {
        m_explorer->refresh();
        QMessageBox::information(this, "UI Synchronization Complete",
            "Firmware UI sources and hardware configuration have been synchronized in 'generated/'.\n"
            "All user code in 'main.c' and 'user/' was safely preserved.");
    } else {
        QMessageBox::warning(this, "UI Synchronization Failed",
            QString("Could not synchronize UI code:\n%1").arg(errorMsg));
    }
}

void CodeWorkspaceView::syncWithProject(Project* project) {
    m_project = project;
    if (!project) return;

    QString fwDir = project->firmwareDirectory();
    m_explorer->setRootDirectory(fwDir);
    m_terminal->setProjectName(project->projectName());
    m_terminal->setProjectRoot(project->projectDirectory());
    m_terminal->setWorkingDirectory(fwDir);
    if (m_buildOutput) {
        m_buildOutput->setProject(project);
    }

    connect(project, &Project::firmwareDirectoryChanged, this, [this](const QString& dir) {
        m_explorer->setRootDirectory(dir);
        m_terminal->setWorkingDirectory(dir);
    }, Qt::UniqueConnection);

    // If no file is open, open the primary firmware source
    if (m_editor && m_editor->openFileCount() == 0) {
        QStringList candidates = {
            QDir(fwDir).filePath("scr/Main.c"),
            QDir(fwDir).filePath("src/Main.c"),
            QDir(fwDir).filePath("src/main.c"),
            QDir(fwDir).filePath("Main.c"),
            QDir(fwDir).filePath("main.c"),
            QDir(fwDir).filePath("user/main.c"),
            QDir(fwDir).filePath("startup/startup.s"),
            QDir(fwDir).filePath("startup.s")
        };
        for (const QString& candidate : candidates) {
            if (QFile::exists(candidate)) {
                openFile(candidate);
                break;
            }
        }
    }
}

void CodeWorkspaceView::toggleBottomPanel() {
    if (!m_rightSplitter) return;
    QList<int> sizes = m_rightSplitter->sizes();
    if (sizes.size() >= 2) {
        if (sizes[1] <= 10) {
            // Restore previous height or default 250px
            int editorH = qMax(100, sizes[0] - 250);
            m_rightSplitter->setSizes({editorH, 250});
        } else {
            // Collapse
            m_rightSplitter->setSizes({sizes[0] + sizes[1], 0});
        }
    }
}

bool CodeWorkspaceView::isBottomPanelVisible() const {
    if (!m_rightSplitter) return false;
    QList<int> sizes = m_rightSplitter->sizes();
    return (sizes.size() >= 2 && sizes[1] > 10);
}

void CodeWorkspaceView::onProblemsCountChanged(int total, int errors, int warnings) {
    if (total == 0) {
        m_bottomTabs->setTabText(0, "Problems");
    } else {
        m_bottomTabs->setTabText(0, QString("Problems (%1)").arg(total));
    }
}
