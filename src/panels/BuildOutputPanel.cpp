#include "BuildOutputPanel.h"
#include "project/Project.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFileInfo>
#include <QDir>
#include <QColor>

BuildOutputPanel::BuildOutputPanel(Project* project, QWidget* parent)
    : QWidget(parent)
    , m_project(project)
{
    setupUi();
    setProject(project);

    connect(&m_runner, &BuildRunner::buildStarted, this, &BuildOutputPanel::onBuildStarted);
    connect(&m_runner, &BuildRunner::outputReceived, this, &BuildOutputPanel::onOutputReceived);
    connect(&m_runner, &BuildRunner::diagnosticFound, this, &BuildOutputPanel::onDiagnosticFound);
    connect(&m_runner, &BuildRunner::buildFinished, this, &BuildOutputPanel::onBuildFinished);
}

BuildOutputPanel::~BuildOutputPanel() {
    m_runner.cancelBuild();
}

void BuildOutputPanel::setProject(Project* project) {
    m_project = project;
}

void BuildOutputPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // Toolbar
    auto* toolBarLayout = new QHBoxLayout();

    toolBarLayout->addWidget(new QLabel("Toolchain:", this));
    m_toolchainCombo = new QComboBox(this);
    m_toolchainCombo->addItem("ARM GNU (arm-none-eabi-gcc)", static_cast<int>(ToolchainType::ArmGnu));
    m_toolchainCombo->addItem("ESP-IDF (idf.py)", static_cast<int>(ToolchainType::EspIdf));
    m_toolchainCombo->addItem("Native GCC / Clang", static_cast<int>(ToolchainType::NativeGcc));
    m_toolchainCombo->addItem("MSVC (Windows)", static_cast<int>(ToolchainType::Msvc));
    toolBarLayout->addWidget(m_toolchainCombo);

    toolBarLayout->addWidget(new QLabel("Config:", this));
    m_buildTypeCombo = new QComboBox(this);
    m_buildTypeCombo->addItem("Release", static_cast<int>(BuildType::Release));
    m_buildTypeCombo->addItem("Debug", static_cast<int>(BuildType::Debug));
    m_buildTypeCombo->addItem("MinSizeRel", static_cast<int>(BuildType::MinSizeRel));
    m_buildTypeCombo->addItem("RelWithDebInfo", static_cast<int>(BuildType::RelWithDebInfo));
    toolBarLayout->addWidget(m_buildTypeCombo);

    toolBarLayout->addWidget(new QLabel("Opt:", this));
    m_optLevelCombo = new QComboBox(this);
    m_optLevelCombo->addItem("-Os (Size for MCU)", static_cast<int>(OptimizationLevel::Os));
    m_optLevelCombo->addItem("-O2 (Speed)", static_cast<int>(OptimizationLevel::O2));
    m_optLevelCombo->addItem("-O0 (Debug)", static_cast<int>(OptimizationLevel::O0));
    m_optLevelCombo->addItem("-Oz (Extreme Size)", static_cast<int>(OptimizationLevel::Oz));
    toolBarLayout->addWidget(m_optLevelCombo);

    m_configureBtn = new QPushButton("Configure CMake", this);
    m_buildBtn = new QPushButton("Build Target", this);
    m_buildBtn->setStyleSheet("background-color: #27ae60; color: white; font-weight: bold; padding: 4px 10px;");
    m_cleanBtn = new QPushButton("Clean", this);
    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setEnabled(false);

    connect(m_configureBtn, &QPushButton::clicked, this, &BuildOutputPanel::onConfigureClicked);
    connect(m_buildBtn, &QPushButton::clicked, this, &BuildOutputPanel::onBuildClicked);
    connect(m_cleanBtn, &QPushButton::clicked, this, &BuildOutputPanel::onCleanClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &BuildOutputPanel::onCancelClicked);

    toolBarLayout->addWidget(m_configureBtn);
    toolBarLayout->addWidget(m_buildBtn);
    toolBarLayout->addWidget(m_cleanBtn);
    toolBarLayout->addWidget(m_cancelBtn);
    toolBarLayout->addStretch();

    m_statusLabel = new QLabel("Ready", this);
    m_statusLabel->setStyleSheet("color: #888; font-weight: bold;");
    toolBarLayout->addWidget(m_statusLabel);

    m_busyIndicator = new QProgressBar(this);
    m_busyIndicator->setRange(0, 0); // Indeterminate marquee
    m_busyIndicator->setFixedWidth(80);
    m_busyIndicator->setFixedHeight(14);
    m_busyIndicator->setVisible(false);
    toolBarLayout->addWidget(m_busyIndicator);

    mainLayout->addLayout(toolBarLayout);

    // Tabbed Console & Diagnostics
    m_tabWidget = new QTabWidget(this);

    // 1. Raw Output Console
    m_consoleOutput = new QPlainTextEdit(this);
    m_consoleOutput->setReadOnly(true);
    m_consoleOutput->setStyleSheet("background-color: #1a1a1a; color: #f0f0f0; font-family: monospace, 'Courier New'; font-size: 12px;");
    m_tabWidget->addTab(m_consoleOutput, "Build Console");

    // 2. Parsed Diagnostics Table
    m_diagnosticsTable = new QTableWidget(this);
    m_diagnosticsTable->setColumnCount(5);
    m_diagnosticsTable->setHorizontalHeaderLabels(QStringList() << "Severity" << "File" << "Line" << "Col" << "Message");
    m_diagnosticsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_diagnosticsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_diagnosticsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_diagnosticsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_diagnosticsTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_diagnosticsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_diagnosticsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_diagnosticsTable->setAlternatingRowColors(true);
    connect(m_diagnosticsTable, &QTableWidget::cellDoubleClicked, this, &BuildOutputPanel::onDiagnosticDoubleClicked);

    m_tabWidget->addTab(m_diagnosticsTable, "Issues (0)");

    mainLayout->addWidget(m_tabWidget);
}

BuildConfiguration BuildOutputPanel::currentConfiguration() const {
    BuildConfiguration cfg;
    cfg.toolchain = static_cast<ToolchainType>(m_toolchainCombo->currentData().toInt());
    cfg.buildType = static_cast<BuildType>(m_buildTypeCombo->currentData().toInt());
    cfg.optLevel = static_cast<OptimizationLevel>(m_optLevelCombo->currentData().toInt());
    cfg.enableLto = true;
    cfg.parallelJobs = 4;

    if (m_project && !m_project->projectFilePath().isEmpty()) {
        cfg.workingDirectory = QFileInfo(m_project->projectFilePath()).absolutePath();
    } else {
        cfg.workingDirectory = QDir::currentPath();
    }

    return cfg;
}

void BuildOutputPanel::onConfigureClicked() {
    m_consoleOutput->clear();
    m_diagnosticsTable->setRowCount(0);
    m_tabWidget->setTabText(1, "Issues (0)");

    BuildConfiguration cfg = currentConfiguration();
    if (m_runner.startConfigure(cfg)) {
        updateButtonStates();
    }
}

void BuildOutputPanel::onBuildClicked() {
    m_consoleOutput->clear();
    m_diagnosticsTable->setRowCount(0);
    m_tabWidget->setTabText(1, "Issues (0)");

    BuildConfiguration cfg = currentConfiguration();
    if (m_runner.startBuild(cfg)) {
        updateButtonStates();
    }
}

void BuildOutputPanel::onCleanClicked() {
    m_consoleOutput->clear();
    m_diagnosticsTable->setRowCount(0);
    m_tabWidget->setTabText(1, "Issues (0)");

    BuildConfiguration cfg = currentConfiguration();
    if (m_runner.startClean(cfg.workingDirectory)) {
        updateButtonStates();
    }
}

void BuildOutputPanel::onCancelClicked() {
    m_runner.cancelBuild();
    updateButtonStates();
}

void BuildOutputPanel::updateButtonStates() {
    bool running = m_runner.isRunning();
    m_configureBtn->setEnabled(!running);
    m_buildBtn->setEnabled(!running);
    m_cleanBtn->setEnabled(!running);
    m_cancelBtn->setEnabled(running);
    m_busyIndicator->setVisible(running);
}

void BuildOutputPanel::onBuildStarted(const QString& description) {
    m_statusLabel->setText(description);
    m_statusLabel->setStyleSheet("color: #3498db; font-weight: bold;");
    updateButtonStates();
}

void BuildOutputPanel::onOutputReceived(const QString& text, bool isStderr) {
    m_consoleOutput->moveCursor(QTextCursor::End);
    if (isStderr) {
        m_consoleOutput->insertPlainText(text);
    } else {
        m_consoleOutput->insertPlainText(text);
    }
    m_consoleOutput->moveCursor(QTextCursor::End);
}

void BuildOutputPanel::onDiagnosticFound(const CompilerDiagnostic& diag) {
    int row = m_diagnosticsTable->rowCount();
    m_diagnosticsTable->insertRow(row);

    QString sevStr = diagnosticSeverityToString(diag.severity);
    auto* sevItem = new QTableWidgetItem(sevStr);
    if (diag.severity == DiagnosticSeverity::Error) {
        sevItem->setForeground(QColor("#e74c3c"));
        sevItem->setText("❌ " + sevStr);
    } else if (diag.severity == DiagnosticSeverity::Warning) {
        sevItem->setForeground(QColor("#f39c12"));
        sevItem->setText("⚠️ " + sevStr);
    } else {
        sevItem->setForeground(QColor("#3498db"));
        sevItem->setText("ℹ️ " + sevStr);
    }

    m_diagnosticsTable->setItem(row, 0, sevItem);
    m_diagnosticsTable->setItem(row, 1, new QTableWidgetItem(QFileInfo(diag.filePath).fileName()));
    m_diagnosticsTable->setItem(row, 2, new QTableWidgetItem(diag.line > 0 ? QString::number(diag.line) : "-"));
    m_diagnosticsTable->setItem(row, 3, new QTableWidgetItem(diag.column > 0 ? QString::number(diag.column) : "-"));

    auto* msgItem = new QTableWidgetItem(diag.message);
    msgItem->setData(Qt::UserRole, diag.filePath); // Store full path
    m_diagnosticsTable->setItem(row, 4, msgItem);

    m_tabWidget->setTabText(1, QString("Issues (%1)").arg(m_diagnosticsTable->rowCount()));
}

void BuildOutputPanel::onBuildFinished(bool success, int exitCode, qint64 durationMs) {
    updateButtonStates();
    if (success) {
        m_statusLabel->setText(QString("Build Succeeded (%1 ms)").arg(durationMs));
        m_statusLabel->setStyleSheet("color: #2ecc71; font-weight: bold;");
    } else {
        m_statusLabel->setText(QString("Build Failed (Exit %1, %2 ms)").arg(exitCode).arg(durationMs));
        m_statusLabel->setStyleSheet("color: #e74c3c; font-weight: bold;");
        // Automatically switch to Issues tab if diagnostics found
        if (m_diagnosticsTable->rowCount() > 0) {
            m_tabWidget->setCurrentIndex(1);
        }
    }
}

void BuildOutputPanel::onDiagnosticDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    auto* fileItem = m_diagnosticsTable->item(row, 4);
    auto* lineItem = m_diagnosticsTable->item(row, 2);
    auto* colItem = m_diagnosticsTable->item(row, 3);
    if (!fileItem) return;

    QString fullPath = fileItem->data(Qt::UserRole).toString();
    int line = lineItem ? lineItem->text().toInt() : 1;
    int col = colItem ? colItem->text().toInt() : 1;

    emit fileNavigationRequested(fullPath, line, col);
}
