#include "ResourceAnalyzerPanel.h"
#include "project/Project.h"
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QListWidget>

ResourceAnalyzerPanel::ResourceAnalyzerPanel(Project* project, QWidget* parent)
    : QWidget(parent)
    , m_project(project)
{
    setupUi();
    setProject(project);
}

void ResourceAnalyzerPanel::setProject(Project* project) {
    if (m_project) {
        disconnect(m_project, nullptr, this, nullptr);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::projectModified, this, &ResourceAnalyzerPanel::refreshAnalysis);
        connect(m_project, &Project::projectLoaded, this, &ResourceAnalyzerPanel::refreshAnalysis);
        connect(m_project, &Project::hardwareConfigChanged, this, &ResourceAnalyzerPanel::refreshAnalysis);
        connect(m_project, &Project::screenListChanged, this, &ResourceAnalyzerPanel::refreshAnalysis);
        connect(m_project, &Project::dataSourcesChanged, this, &ResourceAnalyzerPanel::refreshAnalysis);
        connect(m_project, &Project::dataBindingsChanged, this, &ResourceAnalyzerPanel::refreshAnalysis);
    }
    refreshAnalysis();
}

void ResourceAnalyzerPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // Header Toolbar
    auto* headerLayout = new QHBoxLayout();
    m_targetLabel = new QLabel("Target: <b>None (Generic Estimation)</b>", this);
    m_resolutionLabel = new QLabel("Resolution: 320x240", this);
    m_resolutionLabel->setStyleSheet("color: #888;");

    m_refreshBtn = new QPushButton("Refresh", this);
    m_refreshBtn->setFixedWidth(70);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ResourceAnalyzerPanel::refreshAnalysis);

    headerLayout->addWidget(m_targetLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_resolutionLabel);
    headerLayout->addWidget(m_refreshBtn);
    mainLayout->addLayout(headerLayout);

    // 1. Memory Gauges Group
    auto* gaugesGroup = new QGroupBox("Memory Footprint & Hardware Utilization", this);
    auto* gaugesLayout = new QVBoxLayout(gaugesGroup);

    // Flash Gauge
    auto* flashTitleLayout = new QHBoxLayout();
    flashTitleLayout->addWidget(new QLabel("<b>Flash Memory (ROM)</b>", this));
    m_flashDetailsLabel = new QLabel("0 KB / Unknown", this);
    flashTitleLayout->addStretch();
    flashTitleLayout->addWidget(m_flashDetailsLabel);
    gaugesLayout->addLayout(flashTitleLayout);

    m_flashBar = new QProgressBar(this);
    m_flashBar->setRange(0, 100);
    m_flashBar->setValue(0);
    m_flashBar->setTextVisible(true);
    m_flashBar->setFixedHeight(18);
    gaugesLayout->addWidget(m_flashBar);

    // RAM Gauge
    auto* ramTitleLayout = new QHBoxLayout();
    ramTitleLayout->addWidget(new QLabel("<b>RAM (SRAM)</b>", this));
    m_ramDetailsLabel = new QLabel("0 KB / Unknown", this);
    ramTitleLayout->addStretch();
    ramTitleLayout->addWidget(m_ramDetailsLabel);
    gaugesLayout->addLayout(ramTitleLayout);

    m_ramBar = new QProgressBar(this);
    m_ramBar->setRange(0, 100);
    m_ramBar->setValue(0);
    m_ramBar->setTextVisible(true);
    m_ramBar->setFixedHeight(18);
    gaugesLayout->addWidget(m_ramBar);

    // Framebuffer Details
    m_framebufferDetailsLabel = new QLabel("Framebuffer: 0 KB (RGB565, 1 buffer, 16-byte DMA aligned)", this);
    m_framebufferDetailsLabel->setStyleSheet("color: #aaa; font-size: 11px;");
    gaugesLayout->addWidget(m_framebufferDetailsLabel);

    mainLayout->addWidget(gaugesGroup);

    // 2. Resource Breakdown Tree
    m_breakdownTree = new QTreeWidget(this);
    m_breakdownTree->setHeaderLabels(QStringList() << "Subsystem / Component" << "Flash Footprint" << "RAM Footprint" << "Details");
    m_breakdownTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_breakdownTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_breakdownTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_breakdownTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_breakdownTree->setAlternatingRowColors(true);
    m_breakdownTree->setRootIsDecorated(true);
    mainLayout->addWidget(m_breakdownTree, 2);

    // 3. Diagnostics & Warnings List
    auto* warnGroup = new QGroupBox("Analyzer Diagnostics & Advisory", this);
    auto* warnLayout = new QVBoxLayout(warnGroup);
    m_warningsList = new QListWidget(this);
    m_warningsList->setAlternatingRowColors(true);
    m_warningsList->setFixedHeight(75);
    warnLayout->addWidget(m_warningsList);
    mainLayout->addWidget(warnGroup);
}

void ResourceAnalyzerPanel::formatProgressBar(QProgressBar* bar, double percent) {
    if (!bar) return;
    int intVal = std::min(100, static_cast<int>(percent));
    bar->setValue(intVal);
    bar->setFormat(QString("%1%").arg(QString::number(percent, 'f', 1)));

    QString chunkColor;
    if (percent > 100.0) {
        chunkColor = "#d9534f"; // Exceeded (Red)
    } else if (percent >= 95.0) {
        chunkColor = "#e67e22"; // Critical (Orange/Amber)
    } else if (percent >= 80.0) {
        chunkColor = "#f1c40f"; // Warning (Yellow)
    } else {
        chunkColor = "#2ecc71"; // Normal (Green)
    }

    bar->setStyleSheet(QString(
        "QProgressBar { border: 1px solid #444; border-radius: 3px; text-align: center; background-color: #222; color: white; }"
        "QProgressBar::chunk { background-color: %1; border-radius: 2px; }"
    ).arg(chunkColor));
}

void ResourceAnalyzerPanel::refreshAnalysis() {
    if (!m_project) return;
    ResourceReport report = m_analyzer.analyzeProject(m_project);
    updateUi(report);
}

void ResourceAnalyzerPanel::updateUi(const ResourceReport& report) {
    // Target & Resolution
    if (!report.targetDeviceName.isEmpty()) {
        m_targetLabel->setText(QString("Target: <b>%1</b>").arg(report.targetDeviceName));
    } else {
        m_targetLabel->setText("Target: <b>Generic Embedded MCU</b>");
    }

    m_resolutionLabel->setText(QString("%1x%2 (%3 bpp, %4 buf)")
        .arg(report.framebufferWidth).arg(report.framebufferHeight).arg(report.bitsPerPixel).arg(report.bufferCount));

    // Flash Bar
    if (report.hasTargetLimits && report.targetFlashCapacityBytes > 0) {
        m_flashDetailsLabel->setText(QString("%1 KB / %2 KB (%3%)")
            .arg(report.flashTotalEstimatedBytes / 1024)
            .arg(report.targetFlashCapacityBytes / 1024)
            .arg(QString::number(report.flashUtilizationPercent, 'f', 1)));
        formatProgressBar(m_flashBar, report.flashUtilizationPercent);
    } else {
        m_flashDetailsLabel->setText(QString("%1 KB (Target Flash Unknown)").arg(report.flashTotalEstimatedBytes / 1024));
        m_flashBar->setValue(0);
        m_flashBar->setFormat("Est: %v KB");
        m_flashBar->setStyleSheet("QProgressBar { border: 1px solid #444; border-radius: 3px; text-align: center; background-color: #222; color: white; }");
    }

    // RAM Bar
    if (report.hasTargetLimits && report.targetRamCapacityBytes > 0) {
        m_ramDetailsLabel->setText(QString("%1 KB / %2 KB (%3%)")
            .arg(report.ramTotalEstimatedBytes / 1024)
            .arg(report.targetRamCapacityBytes / 1024)
            .arg(QString::number(report.ramUtilizationPercent, 'f', 1)));
        formatProgressBar(m_ramBar, report.ramUtilizationPercent);
    } else {
        m_ramDetailsLabel->setText(QString("%1 KB (Target RAM Unknown)").arg(report.ramTotalEstimatedBytes / 1024));
        m_ramBar->setValue(0);
        m_ramBar->setFormat("Est: %v KB");
        m_ramBar->setStyleSheet("QProgressBar { border: 1px solid #444; border-radius: 3px; text-align: center; background-color: #222; color: white; }");
    }

    // Framebuffer label
    m_framebufferDetailsLabel->setText(QString("Display Framebuffer: %1 KB (DMA Row Stride: %2 B, Format: %3 bpp, %4 buffer%5)")
        .arg(report.framebufferBytes / 1024)
        .arg(report.rowStrideBytes)
        .arg(report.bitsPerPixel)
        .arg(report.bufferCount)
        .arg(report.bufferCount > 1 ? "s" : ""));

    // Breakdown Tree
    m_breakdownTree->clear();

    auto addTreeRow = [this](const QString& name, size_t flash, size_t ram, const QString& details, QTreeWidgetItem* parent = nullptr) {
        auto* item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(m_breakdownTree);
        item->setText(0, name);
        item->setText(1, flash > 0 ? QString("%1 KB").arg(QString::number(flash / 1024.0, 'f', 1)) : "-");
        item->setText(2, ram > 0 ? QString("%1 KB").arg(QString::number(ram / 1024.0, 'f', 1)) : "-");
        item->setText(3, details);
        return item;
    };

    // Framework
    addTreeRow("GUI Framework Engine", report.flashFrameworkBaseBytes, report.ramFrameworkWorkBufferBytes, "Core GUI library runtime & drawing buffers");

    // Framebuffer
    addTreeRow("Display Framebuffer", 0, report.ramFramebufferBytes, QString("%1x%2 @ %3 bpp (DMA aligned)").arg(report.framebufferWidth).arg(report.framebufferHeight).arg(report.bitsPerPixel));

    // UI Components
    auto* compItem = addTreeRow(QString("UI Components (%1)").arg(report.totalComponents), report.flashComponentCodeBytes, report.ramComponentStateBytes, QString("Across %1 screen(s)").arg(report.totalScreens));
    compItem->setExpanded(true);

    // Assets
    if (report.flashAssetsBytes > 0) {
        addTreeRow("Compiled Image Assets", report.flashAssetsBytes, 0, "Bitmap assets compiled to C arrays in Flash");
    }

    // Fonts
    addTreeRow("Embedded Font Tables", report.flashFontsBytes, 0, "Pre-rendered font glyph tables");

    // DataSources & Bindings
    if (report.totalDataSources > 0 || report.totalDataBindings > 0) {
        addTreeRow(QString("Data Binding Subsystem (%1 sources, %2 bindings)").arg(report.totalDataSources).arg(report.totalDataBindings),
            report.flashBindingsBytes, report.ramDataSourcesBytes, "Hardware pin/bus binding structs & buffers");
    }

    // Diagnostics / Warnings
    m_warningsList->clear();
    for (const ResourceWarning& w : report.warnings) {
        auto* item = new QListWidgetItem(m_warningsList);
        QString prefix = (w.severity == ResourceWarningSeverity::Critical) ? "[CRITICAL]" :
                         (w.severity == ResourceWarningSeverity::Warning) ? "[WARNING]" : "[INFO]";
        item->setText(QString("%1 [%2] %3").arg(prefix, w.category, w.message));
        if (w.severity == ResourceWarningSeverity::Critical) {
            item->setForeground(QColor("#ff6b6b"));
        } else if (w.severity == ResourceWarningSeverity::Warning) {
            item->setForeground(QColor("#feca57"));
        } else {
            item->setForeground(QColor("#48dbfb"));
        }
    }
}
