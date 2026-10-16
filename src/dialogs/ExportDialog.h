#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QListWidget>
#include <QGroupBox>
#include <QLabel>
#include "codegen/BackendRegistry.h"
#include "codegen/FirmwareExportManager.h"
#include "analyzer/ResourceEstimator.h"

class Project;

class ExportDialog : public QDialog {
    Q_OBJECT

public:
    explicit ExportDialog(Project* project, QWidget* parent = nullptr);
    ~ExportDialog() override = default;

    ResourceAnalysis currentAnalysis() const { return m_analysis; }
    ResourceEstimator& estimator() { return m_estimator; }
    QPushButton* exportButton() const { return m_exportBtn; }
    QPushButton* optimizeButton() const { return m_optimizeBtn; }
    QProgressBar* flashBar() const { return m_flashBar; }
    QProgressBar* ramBar() const { return m_ramBar; }
    QListWidget* warningListWidget() const { return m_warningListWidget; }
    QLabel* resourceStatusLabel() const { return m_resourceStatusLabel; }
    // Task 6: the backend selector (same source of truth as the dock).
    QComboBox* frameworkCombo() const { return m_frameworkCombo; }

private slots:
    void onBrowseClicked();
    void onExportClicked();
    void onOptimizeClicked();

private:
    void setupUi();
    void updateValidation();
    void runResourceAnalysis();
    void updateResourceAnalysisUi();

    // Task 6: effective backend selected in the combo (never the planned one).
    BackendDescriptor selectedBackend() const;
    QString selectedBackendFramework() const;

    Project* m_project = nullptr;
    FirmwareExportManager m_exportManager;
    ResourceEstimator m_estimator;
    ResourceAnalysis m_analysis;

    // Form settings
    QLineEdit* m_outputPathEdit = nullptr;
    QPushButton* m_browseBtn = nullptr;
    QComboBox* m_frameworkCombo = nullptr;
    QComboBox* m_targetCombo = nullptr;
    QCheckBox* m_generateCmakeCheck = nullptr;
    QCheckBox* m_overwriteUserCodeCheck = nullptr;

    // Resource Analysis UI
    QGroupBox* m_resourceGroupBox = nullptr;
    QLabel* m_deviceProfileLabel = nullptr;
    QLabel* m_resourceStatusLabel = nullptr;
    QProgressBar* m_flashBar = nullptr;
    QLabel* m_flashLabel = nullptr;
    QProgressBar* m_ramBar = nullptr;
    QLabel* m_ramLabel = nullptr;
    QLabel* m_breakdownLabel = nullptr;
    QListWidget* m_warningListWidget = nullptr;
    QPushButton* m_optimizeBtn = nullptr;

    // Summary & Action buttons
    QTextEdit* m_summaryEdit = nullptr;
    QPushButton* m_exportBtn = nullptr;
    QPushButton* m_closeBtn = nullptr;
};
