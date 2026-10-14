#include "ExportValidationDialog.h"
#include "theme/Theme.h"
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <algorithm>

ExportValidationDialog::ExportValidationDialog(const ResourceAnalysis& analysis, QWidget* parent)
    : QDialog(parent)
    , m_analysis(analysis)
{
    setupUi();
}

void ExportValidationDialog::setupUi() {
    setWindowTitle("Pre-Export Resource Validation");
    resize(480, 360);

    setStyleSheet(QString(
        "QDialog { background-color: %1; color: %2; }"
        "QLabel { color: %2; font-size: 11px; }"
        "QProgressBar { background-color: %3; border: 1px solid %4; border-radius: %5px; text-align: center; color: %2; font-weight: bold; font-size: 10px; height: 16px; }"
        "QProgressBar::chunk { border-radius: 3px; }"
        "QListWidget { background-color: %3; border: 1px solid %4; border-radius: %5px; font-size: 11px; padding: 4px; }"
        "QListWidget::item { padding: 4px; border-bottom: 1px solid %4; }"
        "QPushButton { background: %6; color: %2; border: 1px solid %4; border-radius: %5px; padding: 6px 14px; font-weight: bold; font-size: 11px; }"
        "QPushButton:hover { background: %7; border-color: %8; }"
        "#exportAnywayBtn { background: %9; border-color: %10; color: %10; }"
        "#exportAnywayBtn:hover { background: %10; color: %2; }"
        "#optimizeBtn { background: %8; border-color: %11; color: %3; }"
        "#optimizeBtn:hover { background: %11; }"
    ).arg(Theme::BgSurface, Theme::TextPrimary, Theme::BgBase, Theme::BorderSubtle, QString::number(Theme::RadiusSm),
          Theme::BgRaised, Theme::BgOverlay, Theme::Accent, Theme::DangerBg, Theme::Danger,
          Theme::AccentHover));

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setSpacing(10);
    root->setContentsMargins(14, 14, 14, 14);

    m_headerLabel = new QLabel(this);
    if (m_analysis.status == ResourceAnalysis::CRITICAL) {
        m_headerLabel->setText("<h2>⚠ Resource Limit Exceeded</h2>");
        m_headerLabel->setStyleSheet(QString("color: %1;").arg(Theme::Danger));
    } else {
        m_headerLabel->setText("<h2>⚠ Resource Warning</h2>");
        m_headerLabel->setStyleSheet(QString("color: %1;").arg(Theme::Warning));
    }
    root->addWidget(m_headerLabel);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setText(QString("Target: <b>%1</b> | Framework: <b>%2</b><br>%3")
                                .arg(m_analysis.device_name, m_analysis.target_framework, m_analysis.summary));
    root->addWidget(m_summaryLabel);

    // Flash and RAM Bars
    QLabel* fLbl = new QLabel(QString("Flash Memory: %1 / %2 KB (%3%)")
                                  .arg(m_analysis.flash_used / 1024)
                                  .arg(m_analysis.flash_total / 1024)
                                  .arg(m_analysis.flash_percent, 0, 'f', 1), this);
    root->addWidget(fLbl);

    m_flashBar = new QProgressBar(this);
    int flashPct = std::clamp(static_cast<int>(m_analysis.flash_percent), 0, 100);
    m_flashBar->setValue(flashPct);
    QString flashColor = flashPct >= 95 ? Theme::Danger : (flashPct >= 80 ? Theme::Warning : Theme::Success);
    m_flashBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(flashColor));
    root->addWidget(m_flashBar);

    QLabel* rLbl = new QLabel(QString("RAM Memory: %1 / %2 KB (%3%)")
                                  .arg(m_analysis.ram_used / 1024)
                                  .arg(m_analysis.ram_total / 1024)
                                  .arg(m_analysis.ram_percent, 0, 'f', 1), this);
    root->addWidget(rLbl);

    m_ramBar = new QProgressBar(this);
    int ramPct = std::clamp(static_cast<int>(m_analysis.ram_percent), 0, 100);
    m_ramBar->setValue(ramPct);
    QString ramColor = ramPct >= 95 ? Theme::Danger : (ramPct >= 80 ? Theme::Warning : Theme::Info);
    m_ramBar->setStyleSheet(QString("QProgressBar::chunk { background-color: %1; }").arg(ramColor));
    root->addWidget(m_ramBar);

    QLabel* warnTitle = new QLabel("<b>Identified Bottlenecks & Warnings:</b>", this);
    root->addWidget(warnTitle);

    m_warningsList = new QListWidget(this);
    for (const QString& w : m_analysis.warnings) {
        QListWidgetItem* item = new QListWidgetItem(w, m_warningsList);
        item->setForeground(w.startsWith("CRITICAL") ? Theme::cDanger() : Theme::cWarning());
    }
    root->addWidget(m_warningsList);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    QPushButton* optimizeBtn = new QPushButton("Optimize Design", this);
    optimizeBtn->setObjectName("optimizeBtn");
    connect(optimizeBtn, &QPushButton::clicked, this, &ExportValidationDialog::onOptimizeClicked);
    btnLayout->addWidget(optimizeBtn);

    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    connect(cancelBtn, &QPushButton::clicked, this, &ExportValidationDialog::onCancelClicked);
    btnLayout->addWidget(cancelBtn);

    btnLayout->addStretch();

    QPushButton* exportAnywayBtn = new QPushButton("Export Anyway", this);
    exportAnywayBtn->setObjectName("exportAnywayBtn");
    connect(exportAnywayBtn, &QPushButton::clicked, this, &ExportValidationDialog::onExportClicked);
    btnLayout->addWidget(exportAnywayBtn);

    root->addLayout(btnLayout);
}

void ExportValidationDialog::onExportClicked() {
    m_action = EXPORT;
    accept();
}

void ExportValidationDialog::onCancelClicked() {
    m_action = CANCEL;
    reject();
}

void ExportValidationDialog::onOptimizeClicked() {
    m_action = OPTIMIZE;
    reject();
}
