#include "ResourceAnalyzerPanel.h"
#include "Project.h"
#include "theme/Theme.h"
#include <QScrollArea>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <algorithm>

ResourceAnalyzerPanel::ResourceAnalyzerPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    m_device = m_estimator.capabilityDatabase().getDeviceProfile("STM32H745");
    refresh();
}

void ResourceAnalyzerPanel::setupUi() {
    setObjectName("ResourceAnalyzerPanel");
    setStyleSheet(QString(
        "#ResourceAnalyzerPanel { background-color: %1; color: %2; }"
        "QGroupBox { font-size: 11px; font-weight: 600; color: %3; border: 1px solid %4; border-radius: %5px; margin-top: 8px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
        "QLabel { color: %6; font-size: 11px; }"
        "QComboBox { background: %7; color: %2; border: 1px solid %4; border-radius: %8px; padding: 4px 8px; font-size: 11px; font-weight: 600; }"
        "QProgressBar { background-color: %7; border: 1px solid %4; border-radius: %8px; text-align: center; color: %2; font-weight: 600; font-size: 10px; height: 16px; }"
        "QProgressBar::chunk { border-radius: 3px; }"
        "QPushButton { background: %9; color: %2; border: none; border-radius: %8px; padding: 6px 12px; font-size: 11px; font-weight: 600; }"
        "QPushButton:hover { background: %10; }"
        "QListWidget { background-color: %7; border: 1px solid %4; border-radius: %8px; font-size: 11px; }"
        "QListWidget::item { padding: 4px 6px; border-bottom: 1px solid %4; }"
    ).arg(Theme::BgSurface, Theme::TextPrimary, Theme::TextSecondary, Theme::BorderSubtle)
     .arg(Theme::RadiusMd)
     .arg(Theme::TextSecondary, Theme::BgBase)
     .arg(Theme::RadiusSm)
     .arg(Theme::BgRaised, Theme::BgOverlay));

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(6, 6, 6, 6);

    QScrollArea* sa = new QScrollArea(this);
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);

    QWidget* container = new QWidget(sa);
    QVBoxLayout* l = new QVBoxLayout(container);
    l->setContentsMargins(4, 4, 4, 4);
    l->setSpacing(8);

    // 1. Target Device Selector & Info
    QGroupBox* devGroup = new QGroupBox("Target Microcontroller", container);
    QVBoxLayout* dgl = new QVBoxLayout(devGroup);

    m_deviceCombo = new QComboBox(devGroup);
    m_deviceCombo->setObjectName("deviceCombo");
    const QStringList devices = m_estimator.capabilityDatabase().listAllDevices();
    for (const QString& d : devices) {
        m_deviceCombo->addItem(d);
    }
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ResourceAnalyzerPanel::onDeviceComboChanged);
    dgl->addWidget(m_deviceCombo);

    m_deviceInfoLabel = new QLabel(devGroup);
    m_deviceInfoLabel->setObjectName("deviceInfoLabel");
    m_deviceInfoLabel->setWordWrap(true);
    dgl->addWidget(m_deviceInfoLabel);

    m_statusBadge = new QLabel(devGroup);
    m_statusBadge->setObjectName("statusBadge");
    m_statusBadge->setAlignment(Qt::AlignCenter);
    dgl->addWidget(m_statusBadge);

    l->addWidget(devGroup);

    // 2. Resource Gauges Group
    QGroupBox* gaugeGroup = new QGroupBox("Estimated Hardware Footprint", container);
    QVBoxLayout* ggl = new QVBoxLayout(gaugeGroup);

    // Flash Gauge
    QHBoxLayout* flashHeader = new QHBoxLayout();
    flashHeader->addWidget(new QLabel("Flash Usage (ROM):", gaugeGroup));
    m_flashDetailsLabel = new QLabel(gaugeGroup);
    m_flashDetailsLabel->setObjectName("flashDetailsLabel");
    m_flashDetailsLabel->setAlignment(Qt::AlignRight);
    flashHeader->addWidget(m_flashDetailsLabel);
    ggl->addLayout(flashHeader);

    m_flashBar = new QProgressBar(gaugeGroup);
    m_flashBar->setObjectName("flashBar");
    m_flashBar->setRange(0, 100);
    ggl->addWidget(m_flashBar);

    // RAM Gauge
    QHBoxLayout* ramHeader = new QHBoxLayout();
    ramHeader->addWidget(new QLabel("RAM Usage (SRAM):", gaugeGroup));
    m_ramDetailsLabel = new QLabel(gaugeGroup);
    m_ramDetailsLabel->setObjectName("ramDetailsLabel");
    ramHeader->addWidget(m_ramDetailsLabel);
    ggl->addLayout(ramHeader);

    m_ramBar = new QProgressBar(gaugeGroup);
    m_ramBar->setObjectName("ramBar");
    m_ramBar->setRange(0, 100);
    ggl->addWidget(m_ramBar);

    // CPU Overhead Gauge
    QHBoxLayout* cpuHeader = new QHBoxLayout();
    cpuHeader->addWidget(new QLabel("CPU Rendering Load:", gaugeGroup));
    m_cpuDetailsLabel = new QLabel(gaugeGroup);
    m_cpuDetailsLabel->setObjectName("cpuDetailsLabel");
    cpuHeader->addWidget(m_cpuDetailsLabel);
    ggl->addLayout(cpuHeader);

    m_cpuBar = new QProgressBar(gaugeGroup);
    m_cpuBar->setObjectName("cpuBar");
    m_cpuBar->setRange(0, 100);
    ggl->addWidget(m_cpuBar);

    l->addWidget(gaugeGroup);

    // 3. Breakdown Summary
    QGroupBox* breakdownGroup = new QGroupBox("Resource Breakdown", container);
    QVBoxLayout* bgl = new QVBoxLayout(breakdownGroup);

    m_breakdownLabel = new QLabel(breakdownGroup);
    m_breakdownLabel->setObjectName("breakdownLabel");
    m_breakdownLabel->setTextFormat(Qt::RichText);
    m_breakdownLabel->setWordWrap(true);
    bgl->addWidget(m_breakdownLabel);

    l->addWidget(breakdownGroup);

    // 4. Warnings & Recommendations
    QGroupBox* warnGroup = new QGroupBox("Diagnostics & Warnings", container);
    QVBoxLayout* wgl = new QVBoxLayout(warnGroup);

    m_warningsList = new QListWidget(warnGroup);
    m_warningsList->setObjectName("warningsList");
    m_warningsList->setMinimumHeight(100);
    wgl->addWidget(m_warningsList);

    l->addWidget(warnGroup);

    // 5. Actions / Refresh
    m_refreshBtn = new QPushButton("Recalculate Estimates", container);
    m_refreshBtn->setObjectName("refreshBtn");
    connect(m_refreshBtn, &QPushButton::clicked, this, &ResourceAnalyzerPanel::refresh);
    l->addWidget(m_refreshBtn);

    l->addStretch();
    sa->setWidget(container);
    rootLayout->addWidget(sa);
}

void ResourceAnalyzerPanel::setProject(Project* project) {
    if (m_project == project) return;
    if (m_project) {
        disconnect(m_project, nullptr, this, nullptr);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::projectModified, this, &ResourceAnalyzerPanel::onProjectModified);
        connect(m_project, &Project::screenListChanged, this, &ResourceAnalyzerPanel::onProjectModified);
        connect(m_project, &Project::activeScreenChanged, this, &ResourceAnalyzerPanel::onProjectModified);
        connect(m_project, &Project::hardwareConfigChanged, this, &ResourceAnalyzerPanel::onProjectModified);
    }
    refresh();
}

void ResourceAnalyzerPanel::setDevice(const DeviceProfile& device) {
    m_device = device;
    m_updatingCombo = true;
    if (m_deviceCombo) {
        int idx = m_deviceCombo->findText(device.name);
        if (idx >= 0) {
            m_deviceCombo->setCurrentIndex(idx);
        }
    }
    m_updatingCombo = false;
    refresh();
}

void ResourceAnalyzerPanel::setDevice(const QString& deviceName) {
    DeviceProfile dev = m_estimator.capabilityDatabase().getDeviceProfile(deviceName);
    if (!dev.name.isEmpty()) {
        setDevice(dev);
    }
}

void ResourceAnalyzerPanel::onDeviceComboChanged(int index) {
    if (m_updatingCombo || index < 0) return;
    QString devName = m_deviceCombo->currentText();
    m_device = m_estimator.capabilityDatabase().getDeviceProfile(devName);
    refresh();
}

void ResourceAnalyzerPanel::onProjectModified() {
    refresh();
}

void ResourceAnalyzerPanel::refresh() {
    if (m_device.name.isEmpty()) {
        m_device = m_estimator.capabilityDatabase().getDeviceProfile("STM32H745");
    }

    // Determine target framework
    QString framework = "lvgl";
    if (m_project) {
        framework = m_project->targetFramework().toLower();
        if (framework.isEmpty()) framework = "lvgl";
    }

    m_last_analysis = m_estimator.analyzeProject(m_project, m_device, framework);
    updateUI(m_last_analysis);
}

void ResourceAnalyzerPanel::updateUI(const ResourceAnalysis& analysis) {
    // Device info text
    QString devFamily = !m_device.family.isEmpty() ? m_device.family : (!analysis.device.family.isEmpty() ? analysis.device.family : m_device.name);
    uint32_t speed = m_device.cpu_speed_mhz > 0 ? m_device.cpu_speed_mhz : analysis.device.cpu_speed_mhz;
    uint32_t flashKb = m_device.flash_kb > 0 ? m_device.flash_kb : (analysis.flash_total / 1024);
    uint32_t ramKb = m_device.ram_kb > 0 ? m_device.ram_kb : (analysis.ram_total / 1024);

    uint32_t internalSramKb = m_device.internal_sram_kb > 0 ? m_device.internal_sram_kb : ramKb;
    QString memLine = QString("Flash: %1 KB | Internal SRAM: %2 KB").arg(flashKb).arg(internalSramKb);
    if (m_device.psram_kb > 0) {
        memLine += QString(" | PSRAM: %1 KB").arg(m_device.psram_kb);
    }
    QString targetStatus = !m_device.embedweb_adapter_status.isEmpty() ?
        QString("\nStatus: %1 (Toolchain: %2)").arg(m_device.embedweb_adapter_status, m_device.required_toolchain) : "";

    m_deviceInfoLabel->setText(QString("Target: %1 (%2) @ %3 MHz\n%4%5")
                                   .arg(m_device.name.isEmpty() ? analysis.device_name : m_device.name)
                                   .arg(devFamily)
                                   .arg(speed)
                                   .arg(memLine)
                                   .arg(targetStatus));

    // Status Badge
    if (analysis.status == ResourceAnalysis::OK) {
        m_statusBadge->setText("✓ Fits on Device");
        m_statusBadge->setStyleSheet(QString("background: %1; color: %2; padding: 4px 8px; border-radius: %3px; font-weight: 600;").arg(Theme::SuccessBg, Theme::Success).arg(Theme::RadiusSm));
    } else if (analysis.status == ResourceAnalysis::WARNING) {
        m_statusBadge->setText("⚠ Resource Warning (>80% used)");
        m_statusBadge->setStyleSheet(QString("background: %1; color: %2; padding: 4px 8px; border-radius: %3px; font-weight: 600;").arg(Theme::WarningBg, Theme::Warning).arg(Theme::RadiusSm));
    } else if (analysis.status == ResourceAnalysis::CRITICAL) {
        m_statusBadge->setText("✕ CRITICAL: Out of Memory!");
        m_statusBadge->setStyleSheet(QString("background: %1; color: %2; padding: 4px 8px; border-radius: %3px; font-weight: 600;").arg(Theme::DangerBg, Theme::Danger).arg(Theme::RadiusSm));
    } else {
        m_statusBadge->setText("⚠ Framework Mismatch");
        m_statusBadge->setStyleSheet(QString("background: %1; color: %2; padding: 4px 8px; border-radius: %3px; font-weight: 600;").arg(Theme::InfoBg, Theme::Info).arg(Theme::RadiusSm));
    }

    // 2. Flash Progress Bar
    int flashPct = std::clamp(static_cast<int>(analysis.flash_percent), 0, 100);
    m_flashBar->setValue(flashPct);
    m_flashDetailsLabel->setText(QString("%1 / %2 KB (%3%)")
                                     .arg(analysis.flash_used / 1024)
                                     .arg(analysis.flash_total / 1024)
                                     .arg(analysis.flash_percent, 0, 'f', 1));
    QString flashColor = flashPct >= 95 ? Theme::Danger : (flashPct >= 80 ? Theme::Warning : Theme::Success);
    m_flashBar->setStyleSheet(QString("QProgressBar { background-color: %1; border: 1px solid %2; border-radius: %3px; text-align: center; color: %4; font-weight: 600; font-size: 10px; height: 16px; }"
                                      "QProgressBar::chunk { background-color: %5; border-radius: 3px; }")
                                      .arg(Theme::BgBase, Theme::BorderSubtle).arg(Theme::RadiusSm).arg(Theme::TextPrimary, flashColor));

    // 3. RAM Progress Bar
    int ramPct = std::clamp(static_cast<int>(analysis.ram_percent), 0, 100);
    m_ramBar->setValue(ramPct);
    m_ramDetailsLabel->setText(QString("%1 / %2 KB (%3%)")
                                   .arg(analysis.ram_used / 1024)
                                   .arg(analysis.ram_total / 1024)
                                   .arg(analysis.ram_percent, 0, 'f', 1));
    QString ramColor = ramPct >= 95 ? Theme::Danger : (ramPct >= 80 ? Theme::Warning : Theme::Info);
    m_ramBar->setStyleSheet(QString("QProgressBar { background-color: %1; border: 1px solid %2; border-radius: %3px; text-align: center; color: %4; font-weight: 600; font-size: 10px; height: 16px; }"
                                    "QProgressBar::chunk { background-color: %5; border-radius: 3px; }")
                                    .arg(Theme::BgBase, Theme::BorderSubtle).arg(Theme::RadiusSm).arg(Theme::TextPrimary, ramColor));

    // 4. CPU Overhead Bar
    int cpuPct = std::clamp(static_cast<int>(analysis.cpu_overhead_percent), 0, 100);
    m_cpuBar->setValue(cpuPct);
    m_cpuDetailsLabel->setText(QString("~%1% frame budget").arg(analysis.cpu_overhead_percent, 0, 'f', 1));
    QString cpuColor = cpuPct >= 70 ? Theme::Danger : (cpuPct >= 40 ? Theme::Warning : Theme::Accent);
    m_cpuBar->setStyleSheet(QString("QProgressBar { background-color: %1; border: 1px solid %2; border-radius: %3px; text-align: center; color: %4; font-weight: 600; font-size: 10px; height: 16px; }"
                                    "QProgressBar::chunk { background-color: %5; border-radius: 3px; }")
                                    .arg(Theme::BgBase, Theme::BorderSubtle).arg(Theme::RadiusSm).arg(Theme::TextPrimary, cpuColor));

    // 5. Breakdown Text
    QString breakdownHtml = QString(
        "• <b>Framework Baseline:</b> %1 KB Flash, %2 KB RAM<br>"
        "• <b>UI Components:</b> %3 KB Flash, %4 KB RAM<br>"
        "• <b>Assets & Images:</b> %5 KB Flash, %6 KB RAM<br>"
        "• <b>User Headroom:</b> <font color='%7'><b>%8 KB Flash, %9 KB RAM</b></font>"
    ).arg(analysis.breakdown.framework_flash / 1024)
     .arg(analysis.breakdown.framework_ram / 1024)
     .arg(analysis.breakdown.components_flash / 1024)
     .arg(analysis.breakdown.components_ram / 1024)
     .arg(analysis.breakdown.assets_flash / 1024)
     .arg(analysis.breakdown.assets_ram / 1024)
     .arg(Theme::Success)
     .arg(analysis.breakdown.user_space_flash / 1024)
     .arg(analysis.breakdown.user_space_ram / 1024);
    m_breakdownLabel->setText(breakdownHtml);

    // 6. Warnings List
    m_warningsList->clear();
    if (analysis.warnings.isEmpty()) {
        QListWidgetItem* item = new QListWidgetItem("✓ No warnings. Resource budget is healthy.", m_warningsList);
        item->setForeground(Theme::cSuccess());
    } else {
        for (const QString& w : analysis.warnings) {
            QListWidgetItem* item = new QListWidgetItem(w, m_warningsList);
            if (w.startsWith("CRITICAL")) {
                item->setForeground(Theme::cDanger());
            } else {
                item->setForeground(Theme::cWarning());
            }
        }
    }
}
