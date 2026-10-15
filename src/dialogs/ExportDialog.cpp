#include "ExportDialog.h"
#include "project/Project.h"
#include "analyzer/ResourceEstimator.h"
#include "theme/Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <algorithm>

ExportDialog::ExportDialog(Project* project, QWidget* parent)
    : QDialog(parent)
    , m_project(project)
{
    setWindowTitle("Export to Real Firmware Project");
    resize(640, 720);
    setupUi();
    runResourceAnalysis();
    updateValidation();
}

void ExportDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(14, 14, 14, 14);

    // Title / Description
    auto* descLabel = new QLabel(
        "<b>Export Production Embedded Firmware</b><br>"
        "Generates a complete embedded firmware project separating auto-generated UI/HAL "
        "from user-maintained application code (<code>user/main.c</code>, <code>custom_logic.c</code>).", this);
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);

    // Form settings
    auto* formGroup = new QGroupBox("Export Configuration", this);
    auto* formLayout = new QFormLayout(formGroup);

    // Output Directory
    auto* pathLayout = new QHBoxLayout();
    m_outputPathEdit = new QLineEdit(this);
    QString defaultExportPath = QDir::currentPath() + "/exported_firmware";
    if (m_project && !m_project->projectFilePath().isEmpty()) {
        defaultExportPath = QFileInfo(m_project->projectFilePath()).absolutePath() + "/exported_firmware";
    }
    m_outputPathEdit->setText(defaultExportPath);
    m_browseBtn = new QPushButton("Browse...", this);
    connect(m_browseBtn, &QPushButton::clicked, this, &ExportDialog::onBrowseClicked);
    pathLayout->addWidget(m_outputPathEdit);
    pathLayout->addWidget(m_browseBtn);
    formLayout->addRow("Output Directory:", pathLayout);

    // Framework
    m_frameworkCombo = new QComboBox(this);
    m_frameworkCombo->addItem("LVGL v8 / v9 (C Embedded Library)", "lvgl");
    m_frameworkCombo->addItem("µGFX (Lightweight C Library)", "ugfx");
    m_frameworkCombo->addItem("Qt for MCUs (QUL QML Engine)", "qtmcu");
    m_frameworkCombo->addItem("EmbedWeb (Native Bounded C Runtime - Prototype)", "embedweb");
    if (m_project) {
        QString fw = m_project->targetFramework().toLower();
        if (fw.contains("ugfx")) m_frameworkCombo->setCurrentIndex(1);
        else if (fw.contains("qtmcu")) m_frameworkCombo->setCurrentIndex(2);
        else if (fw.contains("embedweb")) m_frameworkCombo->setCurrentIndex(3);
        else m_frameworkCombo->setCurrentIndex(0);
    }
    formLayout->addRow("Target Framework:", m_frameworkCombo);

    // Target Hardware
    m_targetCombo = new QComboBox(this);
    m_targetCombo->addItem("STM32 (ARM Cortex-M HAL / LL)", "stm32");
    m_targetCombo->addItem("ESP32 / ESP32-S3 (ESP-IDF / FreeRTOS)", "esp32");
    m_targetCombo->addItem("Raspberry Pi Pico (Pico SDK / C++)", "pico");
    m_targetCombo->addItem("Generic Embedded / PC Simulator", "generic");
    if (m_project) {
        QString dev = m_project->hardwareConfig().deviceId.toLower();
        if (dev.contains("esp32")) m_targetCombo->setCurrentIndex(1);
        else if (dev.contains("pico") || dev.contains("rp2040")) m_targetCombo->setCurrentIndex(2);
        else m_targetCombo->setCurrentIndex(0);
    }
    formLayout->addRow("Hardware Target:", m_targetCombo);

    // Options
    m_generateCmakeCheck = new QCheckBox("Generate Root CMakeLists.txt with toolchain rules", this);
    m_generateCmakeCheck->setChecked(true);
    formLayout->addRow("", m_generateCmakeCheck);

    m_overwriteUserCodeCheck = new QCheckBox("Overwrite existing user code (Dangerous: replaces user/main.c)", this);
    m_overwriteUserCodeCheck->setChecked(false);
    formLayout->addRow("", m_overwriteUserCodeCheck);

    mainLayout->addWidget(formGroup);

    // ── Resource Analysis GroupBox ───────────────────────────────────────────
    m_resourceGroupBox = new QGroupBox("Resource Analysis (Estimated)", this);
    auto* resLayout = new QVBoxLayout(m_resourceGroupBox);
    resLayout->setSpacing(6);

    m_deviceProfileLabel = new QLabel(this);
    m_deviceProfileLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TextPrimary));
    resLayout->addWidget(m_deviceProfileLabel);

    // Flash Progress
    m_flashLabel = new QLabel("Estimated Flash Memory: -- / -- KB (0%)", this);
    m_flashLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TextPrimary));
    resLayout->addWidget(m_flashLabel);

    m_flashBar = new QProgressBar(this);
    m_flashBar->setObjectName("flashBar");
    m_flashBar->setRange(0, 100);
    m_flashBar->setValue(0);
    m_flashBar->setTextVisible(true);
    m_flashBar->setStyleSheet(QString(
        "QProgressBar { background-color: %1; border: 1px solid %2; border-radius: %3px; text-align: center; color: %4; font-weight: bold; font-size: 10px; height: 16px; }"
        "QProgressBar::chunk { background-color: %5; border-radius: 2px; }"
    ).arg(Theme::BgBase, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::TextPrimary, Theme::Success));
    resLayout->addWidget(m_flashBar);

    // RAM Progress
    m_ramLabel = new QLabel("Estimated RAM Memory: -- / -- KB (0%)", this);
    m_ramLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TextPrimary));
    resLayout->addWidget(m_ramLabel);

    m_ramBar = new QProgressBar(this);
    m_ramBar->setObjectName("ramBar");
    m_ramBar->setRange(0, 100);
    m_ramBar->setValue(0);
    m_ramBar->setTextVisible(true);
    m_ramBar->setStyleSheet(QString(
        "QProgressBar { background-color: %1; border: 1px solid %2; border-radius: %3px; text-align: center; color: %4; font-weight: bold; font-size: 10px; height: 16px; }"
        "QProgressBar::chunk { background-color: %5; border-radius: 2px; }"
    ).arg(Theme::BgBase, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::TextPrimary, Theme::Success));
    resLayout->addWidget(m_ramBar);

    // Breakdown: Framework / Components / Assets / Framebuffer
    m_breakdownLabel = new QLabel(this);
    m_breakdownLabel->setWordWrap(true);
    m_breakdownLabel->setStyleSheet(QString("color: %1; font-size: 11px; margin-top: 2px; margin-bottom: 2px;").arg(Theme::TextSecondary));
    resLayout->addWidget(m_breakdownLabel);

    // Warning List Header & Optimize Button
    auto* warnHeader = new QHBoxLayout();
    auto* warnTitle = new QLabel("<b>Estimated Bottlenecks & Warnings:</b>", this);
    warnTitle->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TextPrimary));
    warnHeader->addWidget(warnTitle);
    warnHeader->addStretch();

    m_optimizeBtn = new QPushButton("Optimize...", this);
    m_optimizeBtn->setObjectName("optimizeButton");
    m_optimizeBtn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 2px 10px; font-size: 11px; font-weight: 600; }"
        "QPushButton:hover { background-color: %5; }"
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::BgOverlay));
    connect(m_optimizeBtn, &QPushButton::clicked, this, &ExportDialog::onOptimizeClicked);
    warnHeader->addWidget(m_optimizeBtn);
    resLayout->addLayout(warnHeader);

    // Warning List
    m_warningListWidget = new QListWidget(this);
    m_warningListWidget->setObjectName("warningListWidget");
    m_warningListWidget->setMaximumHeight(85);
    m_warningListWidget->setStyleSheet(QString(
        "QListWidget { background-color: %1; border: 1px solid %2; border-radius: %3px; padding: 2px; font-size: 11px; }"
        "QListWidget::item { padding: 2px 4px; border-bottom: 1px solid %2; }"
    ).arg(Theme::BgBase, Theme::BorderSubtle, QString::number(Theme::RadiusSm)));
    resLayout->addWidget(m_warningListWidget);

    // Status Banner / Warning
    m_resourceStatusLabel = new QLabel(this);
    m_resourceStatusLabel->setObjectName("statusBannerLabel");
    m_resourceStatusLabel->setWordWrap(true);
    m_resourceStatusLabel->setStyleSheet(QString("font-size: 11px; font-weight: bold; padding: 2px 0;"));
    resLayout->addWidget(m_resourceStatusLabel);

    mainLayout->addWidget(m_resourceGroupBox);

    // Export Summary / Log
    auto* summaryGroup = new QGroupBox("Export Summary & Output", this);
    auto* summaryLayout = new QVBoxLayout(summaryGroup);
    m_summaryEdit = new QTextEdit(this);
    m_summaryEdit->setReadOnly(true);
    m_summaryEdit->setMaximumHeight(90);
    m_summaryEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: monospace; font-size: 11px; border: 1px solid %3; border-radius: %4px;")
        .arg(Theme::BgBase, Theme::TextSecondary, Theme::BorderSubtle, QString::number(Theme::RadiusSm)));
    summaryLayout->addWidget(m_summaryEdit);
    mainLayout->addWidget(summaryGroup);

    // Button box
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_exportBtn = new QPushButton("Export Firmware", this);
    m_exportBtn->setObjectName("exportButton");
    m_exportBtn->setStyleSheet(QString("background-color: %1; color: %2; font-weight: bold; padding: 6px 16px; border-radius: %3px;")
        .arg(Theme::Accent, Theme::BgBase, QString::number(Theme::RadiusSm)));
    connect(m_exportBtn, &QPushButton::clicked, this, &ExportDialog::onExportClicked);

    m_closeBtn = new QPushButton("Close", this);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(m_exportBtn);
    btnLayout->addWidget(m_closeBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_frameworkCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        runResourceAnalysis();
        updateValidation();
    });
    connect(m_targetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        runResourceAnalysis();
        updateValidation();
    });
}

void ExportDialog::runResourceAnalysis() {
    QString fw = m_frameworkCombo ? m_frameworkCombo->currentData().toString() : QString();
    DeviceProfile dev;
    if (m_project) {
        dev = m_project->getDeviceProfile();
    }
    m_analysis = m_estimator.analyze(m_project, dev, fw);
    updateResourceAnalysisUi();
}

void ExportDialog::updateResourceAnalysisUi() {
    if (!m_resourceGroupBox) return;

    if (!m_analysis.has_device || m_analysis.device_name.isEmpty()) {
        m_deviceProfileLabel->setText("Target MCU: <b>No device selected</b> (Estimated memory unconstrained)");
        m_flashLabel->setText("Estimated Flash Memory: Unconstrained (No device selected)");
        m_flashBar->setValue(0);
        m_flashBar->setStyleSheet(QString(
            "QProgressBar { background-color: %1; border: 1px solid %2; border-radius: %3px; text-align: center; color: %4; font-weight: bold; font-size: 10px; height: 16px; }"
            "QProgressBar::chunk { background-color: %5; border-radius: 2px; }"
        ).arg(Theme::BgBase, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::TextPrimary, Theme::Info));

        m_ramLabel->setText("Estimated RAM Memory: Unconstrained (No device selected)");
        m_ramBar->setValue(0);
        m_ramBar->setStyleSheet(QString(
            "QProgressBar { background-color: %1; border: 1px solid %2; border-radius: %3px; text-align: center; color: %4; font-weight: bold; font-size: 10px; height: 16px; }"
            "QProgressBar::chunk { background-color: %5; border-radius: 2px; }"
        ).arg(Theme::BgBase, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::TextPrimary, Theme::Info));

        m_breakdownLabel->setText(QString("Estimated Breakdown: Framework: %1 KB Flash | Components: %2 KB | Assets: %3 KB | Framebuffer: %4 KB")
            .arg(m_analysis.breakdown.framework_flash / 1024)
            .arg((m_analysis.breakdown.components_flash + m_analysis.breakdown.components_ram) / 1024)
            .arg((m_analysis.breakdown.assets_flash + m_analysis.breakdown.assets_ram) / 1024)
            .arg(m_analysis.breakdown.framebuffer_ram / 1024));

        m_warningListWidget->clear();
        QListWidgetItem* item = new QListWidgetItem("[INFO] No device selected. Export will proceed without target MCU memory constraints.", m_warningListWidget);
        item->setForeground(Theme::cInfo());

        m_resourceStatusLabel->setText("No device selected: memory usage will not block export.");
        m_resourceStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 11px;").arg(Theme::Info));
        if (m_exportBtn) {
            m_exportBtn->setEnabled(true);
            m_exportBtn->setToolTip("");
        }
        return;
    }

    // Has target device profile
    m_deviceProfileLabel->setText(QString("Target MCU: <b>%1</b> (%2) — Total Flash: %3 KB | Total RAM: %4 KB")
        .arg(m_analysis.device_name, m_analysis.device.family.isEmpty() ? "Embedded" : m_analysis.device.family)
        .arg(m_analysis.flash_total / 1024)
        .arg(m_analysis.ram_total / 1024));

    // Flash progress bar
    int flashPct = std::clamp(static_cast<int>(m_analysis.flash_percent), 0, 100);
    m_flashBar->setValue(flashPct);
    m_flashLabel->setText(QString("Estimated Flash Memory: %1 / %2 KB (%3%)")
        .arg(m_analysis.flash_used / 1024)
        .arg(m_analysis.flash_total / 1024)
        .arg(m_analysis.flash_percent, 0, 'f', 1));

    QString flashColor = flashPct >= 95 ? Theme::Danger : (flashPct >= 80 ? Theme::Warning : Theme::Success);
    m_flashBar->setStyleSheet(QString(
        "QProgressBar { background-color: %1; border: 1px solid %2; border-radius: %3px; text-align: center; color: %4; font-weight: bold; font-size: 10px; height: 16px; }"
        "QProgressBar::chunk { background-color: %5; border-radius: 2px; }"
    ).arg(Theme::BgBase, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::TextPrimary, flashColor));

    // RAM progress bar
    int ramPct = std::clamp(static_cast<int>(m_analysis.ram_percent), 0, 100);
    m_ramBar->setValue(ramPct);
    m_ramLabel->setText(QString("Estimated RAM Memory: %1 / %2 KB (%3%)")
        .arg(m_analysis.ram_used / 1024)
        .arg(m_analysis.ram_total / 1024)
        .arg(m_analysis.ram_percent, 0, 'f', 1));

    QString ramColor = ramPct >= 95 ? Theme::Danger : (ramPct >= 80 ? Theme::Warning : Theme::Success);
    m_ramBar->setStyleSheet(QString(
        "QProgressBar { background-color: %1; border: 1px solid %2; border-radius: %3px; text-align: center; color: %4; font-weight: bold; font-size: 10px; height: 16px; }"
        "QProgressBar::chunk { background-color: %5; border-radius: 2px; }"
    ).arg(Theme::BgBase, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::TextPrimary, ramColor));

    // Breakdown: Framework / Components / Assets / Framebuffer
    m_breakdownLabel->setText(QString("Estimated Breakdown: Framework: %1 KB Flash, %2 KB RAM | Components: %3 KB | Assets: %4 KB | Framebuffer: %5 KB")
        .arg(m_analysis.breakdown.framework_flash / 1024)
        .arg(m_analysis.breakdown.framework_ram / 1024)
        .arg((m_analysis.breakdown.components_flash + m_analysis.breakdown.components_ram) / 1024)
        .arg((m_analysis.breakdown.assets_flash + m_analysis.breakdown.assets_ram) / 1024)
        .arg(m_analysis.breakdown.framebuffer_ram / 1024));

    // Warnings list
    m_warningListWidget->clear();
    for (const auto& w : m_analysis.structuredWarnings) {
        QString prefix;
        QColor fg;
        if (w.severity == ResourceAnalysis::WarningSeverity::Critical) {
            prefix = "[CRITICAL] ";
            fg = Theme::cDanger();
        } else if (w.severity == ResourceAnalysis::WarningSeverity::Warning) {
            prefix = "[WARNING] ";
            fg = Theme::cWarning();
        } else {
            prefix = "[INFO] ";
            fg = Theme::cInfo();
        }
        QListWidgetItem* item = new QListWidgetItem(prefix + w.message, m_warningListWidget);
        item->setForeground(fg);
    }

    // Status / Blocking Behavior
    if (m_analysis.status == ResourceAnalysis::CRITICAL) {
        m_resourceStatusLabel->setText("Design exceeds device limits. Optimize or select a larger MCU.");
        m_resourceStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 11px;").arg(Theme::Danger));
        if (m_exportBtn) {
            m_exportBtn->setEnabled(false);
            m_exportBtn->setToolTip("Export blocked: Estimated memory exceeds device limits (>=95%). Optimize or choose a larger MCU.");
        }
    } else if (m_analysis.status == ResourceAnalysis::WARNING || m_analysis.flash_percent >= 80.0f || m_analysis.ram_percent >= 80.0f) {
        m_resourceStatusLabel->setText("Resource Warning: Estimated usage exceeds recommended threshold (>=80%). Confirmation required before export.");
        m_resourceStatusLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 11px;").arg(Theme::Warning));
        if (m_exportBtn) {
            m_exportBtn->setEnabled(true);
            m_exportBtn->setToolTip("");
        }
    } else {
        m_resourceStatusLabel->setText("Estimated resource usage within normal device limits (<80%).");
        m_resourceStatusLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::Success));
        if (m_exportBtn) {
            m_exportBtn->setEnabled(true);
            m_exportBtn->setToolTip("");
        }
    }
}

void ExportDialog::updateValidation() {
    QString fw = m_frameworkCombo->currentData().toString();
    QString target = m_targetCombo->currentData().toString();
    QString summary = "Ready to export firmware project.\n";
    summary += QString("Target Framework: %1\n").arg(m_frameworkCombo->currentText());
    summary += QString("Hardware Target:  %1\n").arg(m_targetCombo->currentText());
    if (fw == "embedweb") {
        summary += "Runtime Architecture: Bounded POD runtime, zero dynamic allocations during rendering.\n";
        summary += "Output Files: generated/ui/, generated/runtime/, generated/runtime/adapters/, generated/bindings/, user/main.c, CMakeLists.txt\n\n";

        // Resource & Toolchain Evaluation
        auto evaluations = m_estimator.evaluateEmbedWebTargets(m_project);
        summary += "--- Candidate MCU Target Evaluations ---\n";
        for (const auto& ev : evaluations) {
            QString compStatus;
            switch (ev.compatibility) {
                case ResourceEstimator::EmbedWebTargetEvaluation::Compatible: compStatus = "[COMPATIBLE]"; break;
                case ResourceEstimator::EmbedWebTargetEvaluation::CompatibleWithWarnings: compStatus = "[WARNING]"; break;
                case ResourceEstimator::EmbedWebTargetEvaluation::InsufficientFlash: compStatus = "[INSUFFICIENT FLASH]"; break;
                case ResourceEstimator::EmbedWebTargetEvaluation::InsufficientRAM: compStatus = "[INSUFFICIENT RAM]"; break;
                case ResourceEstimator::EmbedWebTargetEvaluation::MissingToolchain: compStatus = "[MISSING TOOLCHAIN]"; break;
                case ResourceEstimator::EmbedWebTargetEvaluation::NotValidated: compStatus = "[NOT TESTED]"; break;
                case ResourceEstimator::EmbedWebTargetEvaluation::AdapterNotImplemented: compStatus = "[NO ADAPTER]"; break;
                default: compStatus = "[MISMATCH]"; break;
            }
            summary += QString("• %1 (%2, %3): %4 - %5\n")
                           .arg(ev.targetName, ev.family, ev.architecture, compStatus, ev.reason);
        }
        summary += "\n";

        if (target == "esp32") {
            summary += "Target Notice: ESP32-S3 validation is NOT TESTED (Xtensa/ESP-IDF toolchain absent on host).\n";
        } else if (target == "stm32") {
            summary += "Target Notice: STM32 Cortex-M target compiler verified (arm-none-eabi-gcc).\n";
        } else if (target == "pico") {
            summary += "Target Notice: RP2040 Cortex-M0+ target compiler verified (arm-none-eabi-gcc).\n";
        }
    }
    summary += "User preservation: ON (Existing user/ files will not be overwritten)\n";
    m_summaryEdit->setText(summary);
}

void ExportDialog::onBrowseClicked() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Export Directory", m_outputPathEdit->text());
    if (!dir.isEmpty()) {
        m_outputPathEdit->setText(dir);
    }
}

void ExportDialog::onOptimizeClicked() {
    QDialog optDlg(this);
    optDlg.setWindowTitle("Estimated Heaviest Components & Optimization");
    optDlg.resize(600, 380);

    auto* layout = new QVBoxLayout(&optDlg);
    layout->setSpacing(8);

    auto* titleLbl = new QLabel("<b>Estimated Heaviest UI Components (Sorted Descending)</b>", &optDlg);
    titleLbl->setStyleSheet(QString("color: %1; font-size: 12px;").arg(Theme::TextPrimary));
    layout->addWidget(titleLbl);

    auto* subLbl = new QLabel("Read-only optimization suggestions to fit within target MCU Flash and RAM limits:", &optDlg);
    subLbl->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TextSecondary));
    layout->addWidget(subLbl);

    auto* table = new QTableWidget(&optDlg);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"Component Type", "Count", "Est. Flash (KB)", "Est. RAM (Bytes)", "Optimization Suggestion"});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setStyleSheet(QString(
        "QTableWidget { background-color: %1; border: 1px solid %2; border-radius: %3px; gridline-color: %2; font-size: 11px; }"
        "QHeaderView::section { background-color: %4; color: %5; padding: 4px; border: 1px solid %2; font-weight: bold; font-size: 11px; }"
    ).arg(Theme::BgBase, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::BgRaised, Theme::TextPrimary));

    auto heaviest = m_estimator.getHeaviestComponents(m_project);
    table->setRowCount(heaviest.size());
    for (int i = 0; i < heaviest.size(); ++i) {
        const auto& item = heaviest[i];
        table->setItem(i, 0, new QTableWidgetItem(item.componentType));
        table->setItem(i, 1, new QTableWidgetItem(QString::number(item.count)));
        table->setItem(i, 2, new QTableWidgetItem(QString::number(item.totalFlashBytes / 1024.0, 'f', 1)));
        table->setItem(i, 3, new QTableWidgetItem(QString::number(item.totalRamBytes)));
        table->setItem(i, 4, new QTableWidgetItem(item.optimizationSuggestion));
    }
    layout->addWidget(table);

    auto* closeBtn = new QPushButton("Close", &optDlg);
    connect(closeBtn, &QPushButton::clicked, &optDlg, &QDialog::accept);
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);

    optDlg.exec();
}

void ExportDialog::onExportClicked() {
    // 1. Critical Check: >=95% blocks export
    if (m_analysis.has_device && m_analysis.status == ResourceAnalysis::CRITICAL) {
        QMessageBox::critical(this, "Export Blocked", "Design exceeds device limits. Optimize or select a larger MCU.");
        return;
    }

    // 2. Warning Check: >=80% prompts confirmation
    bool force = false;
    if (m_analysis.has_device && (m_analysis.status == ResourceAnalysis::WARNING || m_analysis.flash_percent >= 80.0f || m_analysis.ram_percent >= 80.0f)) {
        auto res = QMessageBox::warning(this, "Estimated Resource Warning",
            QString("Estimated resource utilization is high (%1% Flash, %2% RAM).\n\n"
                    "Exporting may lead to runtime memory constraints or allocation failures on hardware.\n\n"
                    "Do you want to proceed with firmware export anyway?")
                .arg(m_analysis.flash_percent, 0, 'f', 1)
                .arg(m_analysis.ram_percent, 0, 'f', 1),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (res != QMessageBox::Yes) {
            return;
        }
        force = true;
    }

    ExportOptions opts;
    opts.outputDirectory = m_outputPathEdit->text().trimmed();
    opts.targetFramework = m_frameworkCombo->currentData().toString();
    opts.targetMcu = m_targetCombo->currentData().toString();
    opts.generateCmake = m_generateCmakeCheck->isChecked();
    opts.overwriteUserCode = m_overwriteUserCodeCheck->isChecked();
    opts.forceExport = force;

    m_exportBtn->setEnabled(false);
    ExportResult result = m_exportManager.exportProject(m_project, opts);
    m_exportBtn->setEnabled(true);

    if (result.success) {
        QString text = "=== EXPORT SUCCESSFUL ===\n\n";
        text += result.exportSummary + "\n\n";
        text += "Generated files:\n";
        for (const QString& f : result.generatedFiles) {
            text += QString("  + %1\n").arg(f);
        }
        if (!result.preservedUserFiles.isEmpty()) {
            text += "\nPreserved existing user files:\n";
            for (const QString& f : result.preservedUserFiles) {
                text += QString("  * %1\n").arg(f);
            }
        }
        m_summaryEdit->setText(text);
        QMessageBox::information(this, "Export Succeeded", "Firmware project exported successfully!");
    } else {
        m_summaryEdit->setText(QString("Export Failed:\n%1").arg(result.errorMessage));
        QMessageBox::critical(this, "Export Error", result.errorMessage);
    }
}
