#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QTabWidget>
#include <QProgressBar>
#include "build/BuildRunner.h"
#include "build/BuildConfiguration.h"

class Project;

class BuildOutputPanel : public QWidget {
    Q_OBJECT

public:
    explicit BuildOutputPanel(Project* project, QWidget* parent = nullptr);
    ~BuildOutputPanel() override;

    void setProject(Project* project);
    BuildRunner* runner() { return &m_runner; }

public slots:
    void onBuildClicked();
    void onConfigureClicked();
    void onCleanClicked();
    void onCancelClicked();

signals:
    void fileNavigationRequested(const QString& filePath, int line, int column);

private slots:
    void onBuildStarted(const QString& description);
    void onOutputReceived(const QString& text, bool isStderr);
    void onDiagnosticFound(const CompilerDiagnostic& diag);
    void onBuildFinished(bool success, int exitCode, qint64 durationMs);
    void onDiagnosticDoubleClicked(int row, int column);

private:
    void setupUi();
    void updateButtonStates();
    BuildConfiguration currentConfiguration() const;

    Project* m_project = nullptr;
    BuildRunner m_runner;

    // Controls
    QComboBox* m_toolchainCombo = nullptr;
    QComboBox* m_buildTypeCombo = nullptr;
    QComboBox* m_optLevelCombo = nullptr;
    QPushButton* m_configureBtn = nullptr;
    QPushButton* m_buildBtn = nullptr;
    QPushButton* m_cleanBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QProgressBar* m_busyIndicator = nullptr;

    // Tabs
    QTabWidget* m_tabWidget = nullptr;
    QPlainTextEdit* m_consoleOutput = nullptr;
    QTableWidget* m_diagnosticsTable = nullptr;
};
