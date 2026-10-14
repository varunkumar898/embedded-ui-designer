#include "CICDDialog.h"
#include "Project.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QDir>

namespace CICD {

CICDDialog::CICDDialog(Project* project, QWidget* parent)
    : QDialog(parent)
    , m_project(project)
{
    setWindowTitle("CI/CD Pipeline Generator");
    resize(780, 580);
    setupUI();
}

void CICDDialog::setupUI() {
    auto mainLayout = new QVBoxLayout(this);

    auto tabWidget = new QTabWidget(this);

    // Tab 1: Configuration & Generation
    auto configWidget = new QWidget(this);
    auto configLayout = new QVBoxLayout(configWidget);

    auto settingsBox = new QGroupBox("Pipeline Settings", configWidget);
    auto grid = new QGridLayout(settingsBox);

    grid->addWidget(new QLabel("CI/CD Provider:", settingsBox), 0, 0);
    m_providerCombo = new QComboBox(settingsBox);
    m_providerCombo->addItems(CICDGenerator::supportedProviders());
    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CICDDialog::onProviderChanged);
    grid->addWidget(m_providerCombo, 0, 1);

    grid->addWidget(new QLabel("Target MCU:", settingsBox), 1, 0);
    m_targetMcuEdit = new QLineEdit(settingsBox);
    if (m_project && !m_project->hardwareConfig().deviceId.isEmpty()) {
        m_targetMcuEdit->setText(m_project->hardwareConfig().deviceId);
    } else {
        m_targetMcuEdit->setText("STM32F030R8");
    }
    grid->addWidget(m_targetMcuEdit, 1, 1);

    grid->addWidget(new QLabel("UI Generator:", settingsBox), 2, 0);
    m_generatorCombo = new QComboBox(settingsBox);
    m_generatorCombo->addItems(QStringList() << "LVGL" << "Qt/QUL" << "µGFX");
    grid->addWidget(m_generatorCombo, 2, 1);

    grid->addWidget(new QLabel("Toolchain:", settingsBox), 3, 0);
    m_toolchainEdit = new QLineEdit("arm-none-eabi-gcc", settingsBox);
    grid->addWidget(m_toolchainEdit, 3, 1);

    grid->addWidget(new QLabel("Artifact Path:", settingsBox), 4, 0);
    m_artifactPathEdit = new QLineEdit("build/firmware.bin", settingsBox);
    grid->addWidget(m_artifactPathEdit, 4, 1);

    auto flagsRow = new QHBoxLayout();
    m_runBuildCheck = new QCheckBox("Run Build", settingsBox);
    m_runBuildCheck->setChecked(true);
    m_runTestsCheck = new QCheckBox("Run Unit & Simulator Tests", settingsBox);
    m_runTestsCheck->setChecked(true);
    m_archiveCheck = new QCheckBox("Archive Artifacts", settingsBox);
    m_archiveCheck->setChecked(true);
    flagsRow->addWidget(m_runBuildCheck);
    flagsRow->addWidget(m_runTestsCheck);
    flagsRow->addWidget(m_archiveCheck);
    grid->addLayout(flagsRow, 5, 0, 1, 2);

    configLayout->addWidget(settingsBox);

    auto btnRow = new QHBoxLayout();
    auto validateBtn = new QPushButton("Validate Pipeline", configWidget);
    connect(validateBtn, &QPushButton::clicked, this, &CICDDialog::onValidate);
    auto generateBtn = new QPushButton("Generate Pipeline", configWidget);
    generateBtn->setStyleSheet(QString("QPushButton { font-weight: bold; background-color: %1; color: %2; padding: 6px; border-radius: %3px; }").arg(Theme::Accent, Theme::BgBase, QString::number(Theme::RadiusSm)));
    connect(generateBtn, &QPushButton::clicked, this, &CICDDialog::onGenerate);
    btnRow->addWidget(validateBtn);
    btnRow->addWidget(generateBtn);
    configLayout->addLayout(btnRow);

    configLayout->addWidget(new QLabel("Validation Results:", configWidget));
    m_validationOutput = new QTextEdit(configWidget);
    m_validationOutput->setReadOnly(true);
    m_validationOutput->setMaximumHeight(120);
    configLayout->addWidget(m_validationOutput);

    tabWidget->addTab(configWidget, "Settings & Validation");

    // Tab 2: Generated Pipeline Preview
    auto previewWidget = new QWidget(this);
    auto prevLayout = new QVBoxLayout(previewWidget);

    prevLayout->addWidget(new QLabel("Generated Pipeline Configuration:", previewWidget));
    m_pipelinePreview = new QTextEdit(previewWidget);
    QFont font("Monospace", 9);
    font.setStyleHint(QFont::TypeWriter);
    m_pipelinePreview->setFont(font);
    prevLayout->addWidget(m_pipelinePreview);

    auto saveRow = new QHBoxLayout();
    m_statusLabel = new QLabel("", previewWidget);
    saveRow->addWidget(m_statusLabel);
    saveRow->addStretch();
    m_saveButton = new QPushButton("Save Pipeline File...", previewWidget);
    m_saveButton->setEnabled(false);
    connect(m_saveButton, &QPushButton::clicked, this, &CICDDialog::onSaveToFile);
    saveRow->addWidget(m_saveButton);
    prevLayout->addLayout(saveRow);

    tabWidget->addTab(previewWidget, "Pipeline Preview");

    mainLayout->addWidget(tabWidget);

    auto closeRow = new QHBoxLayout();
    closeRow->addStretch();
    auto closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeRow->addWidget(closeBtn);
    mainLayout->addLayout(closeRow);
}

CICDConfig CICDDialog::currentConfig() const {
    CICDConfig cfg;
    cfg.provider = stringToProvider(m_providerCombo->currentText());
    cfg.targetMcu = m_targetMcuEdit->text().trimmed();
    cfg.generator = m_generatorCombo->currentText();
    cfg.toolchain = m_toolchainEdit->text().trimmed();
    cfg.artifactPath = m_artifactPathEdit->text().trimmed();
    cfg.runBuild = m_runBuildCheck->isChecked();
    cfg.runTests = m_runTestsCheck->isChecked();
    cfg.archiveArtifacts = m_archiveCheck->isChecked();
    return cfg;
}

void CICDDialog::onProviderChanged(int index) {
    Q_UNUSED(index);
    m_pipelinePreview->clear();
    m_saveButton->setEnabled(false);
}

void CICDDialog::onValidate() {
    CICDConfig cfg = currentConfig();
    auto items = CICDGenerator::validate(cfg, m_project);

    QString text;
    for (const auto& it : items) {
        QString prefix = it.severity == Diagnostics::Severity::Error ? "🚨 [ERROR]" :
                         (it.severity == Diagnostics::Severity::Warning ? "⚠️ [WARNING]" : "✅ [INFO]");
        text += QString("%1 %2: %3\n").arg(prefix, it.title, it.message);
        if (!it.fixSuggestion.isEmpty()) {
            text += QString("   Suggestion: %1\n").arg(it.fixSuggestion);
        }
    }
    m_validationOutput->setPlainText(text);
}

void CICDDialog::onGenerate() {
    onValidate();

    CICDConfig cfg = currentConfig();
    QString pipelineContent = CICDGenerator::generatePipeline(cfg, m_project);
    m_pipelinePreview->setPlainText(pipelineContent);
    m_saveButton->setEnabled(true);
    m_statusLabel->setText("Pipeline generated successfully. Ready to save.");
}

void CICDDialog::onSaveToFile() {
    CICDConfig cfg = currentConfig();
    QString defaultRelPath = CICDGenerator::defaultPipelineFilePath(cfg.provider);

    QString initialDir = m_project ? QFileInfo(m_project->projectFilePath()).dir().absolutePath() : QDir::currentPath();
    QString defaultFullPath = initialDir + "/" + defaultRelPath;

    QString chosenPath = QFileDialog::getSaveFileName(this, "Save Pipeline File", defaultFullPath, "All Files (*.*)");
    if (chosenPath.isEmpty()) return;

    QFileInfo fi(chosenPath);
    fi.dir().mkpath(".");

    QFile file(chosenPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(m_pipelinePreview->toPlainText().toUtf8());
        file.close();
        QMessageBox::information(this, "Saved", "Pipeline saved successfully to:\n" + chosenPath);
    } else {
        QMessageBox::critical(this, "Save Failed", "Could not write to file: " + file.errorString());
    }
}

} // namespace CICD
