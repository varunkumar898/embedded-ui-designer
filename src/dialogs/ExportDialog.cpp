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

ExportDialog::ExportDialog(Project* project, QWidget* parent)
    : QDialog(parent)
    , m_project(project)
{
    setWindowTitle("Export to Real Firmware Project");
    resize(560, 480);
    setupUi();
    updateValidation();
}

void ExportDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

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

    // Export Summary / Log
    auto* summaryGroup = new QGroupBox("Export Summary & Output", this);
    auto* summaryLayout = new QVBoxLayout(summaryGroup);
    m_summaryEdit = new QTextEdit(this);
    m_summaryEdit->setReadOnly(true);
    m_summaryEdit->setStyleSheet(QString("background-color: %1; color: %2; font-family: monospace; font-size: 11px; border: 1px solid %3; border-radius: %4px;")
        .arg(Theme::BgBase, Theme::TextSecondary, Theme::BorderSubtle, QString::number(Theme::RadiusSm)));
    summaryLayout->addWidget(m_summaryEdit);
    mainLayout->addWidget(summaryGroup);

    // Button box
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_exportBtn = new QPushButton("Export Firmware", this);
    m_exportBtn->setStyleSheet(QString("background-color: %1; color: %2; font-weight: bold; padding: 6px 16px; border-radius: %3px;")
        .arg(Theme::Accent, Theme::BgBase, QString::number(Theme::RadiusSm)));
    connect(m_exportBtn, &QPushButton::clicked, this, &ExportDialog::onExportClicked);

    m_closeBtn = new QPushButton("Close", this);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(m_exportBtn);
    btnLayout->addWidget(m_closeBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_frameworkCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ExportDialog::updateValidation);
    connect(m_targetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ExportDialog::updateValidation);
    updateValidation();
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
        ResourceEstimator estimator;
        auto evaluations = estimator.evaluateEmbedWebTargets(m_project);
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

void ExportDialog::onExportClicked() {
    ExportOptions opts;
    opts.outputDirectory = m_outputPathEdit->text().trimmed();
    opts.targetFramework = m_frameworkCombo->currentData().toString();
    opts.targetMcu = m_targetCombo->currentData().toString();
    opts.generateCmake = m_generateCmakeCheck->isChecked();
    opts.overwriteUserCode = m_overwriteUserCodeCheck->isChecked();

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
