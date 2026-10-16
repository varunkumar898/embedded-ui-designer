#include "BoardDisplayPanel.h"

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QFrame>
#include <QVBoxLayout>

#include "Project.h"
#include "analyzer/CapabilityDatabase.h"
#include "hardware/BoardRegistry.h"
#include "theme/Theme.h"

namespace {

// One shared capability database for every Board & Display dock instance.
CapabilityDatabase& deviceDb() {
    static CapabilityDatabase db;
    static bool loaded = false;
    if (!loaded) {
        db.loadBuiltinDatabase();
        loaded = true;
    }
    return db;
}

QString yesNoUnknown(int value) {
    if (value == 1) return QStringLiteral("Yes");
    if (value == 0) return QStringLiteral("No");
    return QStringLiteral("Unknown");
}

} // namespace

BoardDisplayPanel::BoardDisplayPanel(QWidget* parent)
    : QWidget(parent) {
    setupUi();
    refreshFromProject();
}

void BoardDisplayPanel::setupUi() {
    setObjectName("BoardDisplayPanel");
    setStyleSheet(QString(
        "#BoardDisplayPanel { background-color: %1; color: %2; }"
        "QGroupBox { font-size: 11px; font-weight: 600; color: %3; border: 1px solid %4; border-radius: %5px; margin-top: 8px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
        "QLabel { color: %6; font-size: 11px; }"
        "QComboBox, QSpinBox { background: %7; color: %2; border: 1px solid %4; border-radius: %8px; padding: 3px 6px; font-size: 11px; }"
        "QComboBox:disabled, QSpinBox:disabled { color: %9; }"
        "QTableWidget { background-color: %7; color: %2; border: 1px solid %4; border-radius: %8px; font-size: 11px; gridline-color: %4; }"
        "QHeaderView::section { background: %10; color: %6; border: none; border-bottom: 1px solid %4; padding: 3px 6px; font-size: 10px; font-weight: 600; }"
    ).arg(Theme::BgSurface, Theme::TextPrimary, Theme::TextSecondary, Theme::BorderSubtle)
     .arg(Theme::RadiusMd)
     .arg(Theme::TextSecondary, Theme::BgBase)
     .arg(Theme::RadiusSm)
     .arg(Theme::TextDisabled)
     .arg(Theme::BgRaised));

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(6, 6, 6, 6);

    QScrollArea* sa = new QScrollArea(this);
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);

    QWidget* container = new QWidget(sa);
    QVBoxLayout* l = new QVBoxLayout(container);
    l->setContentsMargins(4, 4, 4, 4);
    l->setSpacing(8);

    // ── 1. Target ────────────────────────────────────────────────────────────
    QGroupBox* targetGroup = new QGroupBox("Target", container);
    QVBoxLayout* tgl = new QVBoxLayout(targetGroup);

    tgl->addWidget(new QLabel("Microcontroller (MCU):", targetGroup));
    m_mcuCombo = new QComboBox(targetGroup);
    m_mcuCombo->setObjectName("bdMcuCombo");
    populateMcuCombo();
    connect(m_mcuCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BoardDisplayPanel::onMcuChanged);
    tgl->addWidget(m_mcuCombo);

    tgl->addWidget(new QLabel("Board:", targetGroup));
    m_boardCombo = new QComboBox(targetGroup);
    m_boardCombo->setObjectName("bdBoardCombo");
    rebuildBoardCombo();
    connect(m_boardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BoardDisplayPanel::onBoardChanged);
    tgl->addWidget(m_boardCombo);

    m_targetSummary = new QLabel(targetGroup);
    m_targetSummary->setObjectName("bdTargetSummary");
    m_targetSummary->setWordWrap(true);
    m_targetSummary->setTextFormat(Qt::RichText);
    tgl->addWidget(m_targetSummary);

    tgl->addWidget(new QLabel("Memory regions (framebuffer suitability):", targetGroup));
    m_regionsTable = new QTableWidget(0, 3, targetGroup);
    m_regionsTable->setObjectName("bdRegionsTable");
    m_regionsTable->setHorizontalHeaderLabels({"Region", "Size", "Framebuffer"});
    m_regionsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_regionsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_regionsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_regionsTable->verticalHeader()->setVisible(false);
    m_regionsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_regionsTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_regionsTable->setMinimumHeight(90);
    tgl->addWidget(m_regionsTable);

    l->addWidget(targetGroup);

    // ── 2. Display ───────────────────────────────────────────────────────────
    QGroupBox* displayGroup = new QGroupBox("Display", container);
    QGridLayout* dgl = new QGridLayout(displayGroup);

    dgl->addWidget(new QLabel("Width:", displayGroup), 0, 0);
    m_widthSpin = new QSpinBox(displayGroup);
    m_widthSpin->setObjectName("bdWidthSpin");
    m_widthSpin->setRange(1, 4096);
    m_widthSpin->setSuffix(" px");
    dgl->addWidget(m_widthSpin, 0, 1);

    dgl->addWidget(new QLabel("Height:", displayGroup), 0, 2);
    m_heightSpin = new QSpinBox(displayGroup);
    m_heightSpin->setObjectName("bdHeightSpin");
    m_heightSpin->setRange(1, 4096);
    m_heightSpin->setSuffix(" px");
    dgl->addWidget(m_heightSpin, 0, 3);

    dgl->addWidget(new QLabel("Pixel format:", displayGroup), 1, 0);
    m_pixelFormatCombo = new QComboBox(displayGroup);
    m_pixelFormatCombo->setObjectName("bdPixelFormatCombo");
    dgl->addWidget(m_pixelFormatCombo, 1, 1, 1, 3);

    dgl->addWidget(new QLabel("Interface:", displayGroup), 2, 0);
    m_interfaceCombo = new QComboBox(displayGroup);
    m_interfaceCombo->setObjectName("bdInterfaceCombo");
    dgl->addWidget(m_interfaceCombo, 2, 1, 1, 3);

    dgl->addWidget(new QLabel("Controller:", displayGroup), 3, 0);
    m_controllerCombo = new QComboBox(displayGroup);
    m_controllerCombo->setObjectName("bdControllerCombo");
    dgl->addWidget(m_controllerCombo, 3, 1, 1, 3);

    dgl->addWidget(new QLabel("Touch driver:", displayGroup), 4, 0);
    m_touchCombo = new QComboBox(displayGroup);
    m_touchCombo->setObjectName("bdTouchCombo");
    dgl->addWidget(m_touchCombo, 4, 1, 1, 3);

    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &BoardDisplayPanel::onResolutionChanged);
    connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &BoardDisplayPanel::onResolutionChanged);
    connect(m_pixelFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BoardDisplayPanel::onPixelFormatChanged);
    connect(m_interfaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BoardDisplayPanel::onInterfaceChanged);
    connect(m_controllerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BoardDisplayPanel::onControllerChanged);
    connect(m_touchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BoardDisplayPanel::onTouchChanged);

    l->addWidget(displayGroup);

    // ── 3. Buffering ─────────────────────────────────────────────────────────
    QGroupBox* bufferGroup = new QGroupBox("Buffering", container);
    QGridLayout* bgl = new QGridLayout(bufferGroup);

    bgl->addWidget(new QLabel("Mode:", bufferGroup), 0, 0);
    m_modeCombo = new QComboBox(bufferGroup);
    m_modeCombo->setObjectName("bdModeCombo");
    m_modeCombo->addItem("Full frame (all lines in RAM)",
                         static_cast<int>(BoardDisplayConfig::FramebufferMode::Full));
    m_modeCombo->addItem("Partial single buffer",
                         static_cast<int>(BoardDisplayConfig::FramebufferMode::PartialSingle));
    m_modeCombo->addItem("Partial double buffer",
                         static_cast<int>(BoardDisplayConfig::FramebufferMode::PartialDouble));
    bgl->addWidget(m_modeCombo, 0, 1, 1, 3);

    bgl->addWidget(new QLabel("Buffer height:", bufferGroup), 1, 0);
    m_bufferLinesSpin = new QSpinBox(bufferGroup);
    m_bufferLinesSpin->setObjectName("bdBufferLinesSpin");
    m_bufferLinesSpin->setRange(1, 4096);
    m_bufferLinesSpin->setSuffix(" lines");
    m_bufferLinesSpin->setValue(BoardDisplayConfig().partialBufferLines);
    bgl->addWidget(m_bufferLinesSpin, 1, 1, 1, 3);

    m_sizeLabel = new QLabel(bufferGroup);
    m_sizeLabel->setObjectName("bdSizeLabel");
    m_sizeLabel->setWordWrap(true);
    bgl->addWidget(m_sizeLabel, 2, 0, 1, 4);

    m_formulaLabel = new QLabel(bufferGroup);
    m_formulaLabel->setObjectName("bdFormulaLabel");
    m_formulaLabel->setWordWrap(true);
    bgl->addWidget(m_formulaLabel, 3, 0, 1, 4);

    m_fitLabel = new QLabel(bufferGroup);
    m_fitLabel->setObjectName("bdFitLabel");
    m_fitLabel->setWordWrap(true);
    m_fitLabel->setAlignment(Qt::AlignCenter);
    bgl->addWidget(m_fitLabel, 4, 0, 1, 4);

    m_fitHintLabel = new QLabel(bufferGroup);
    m_fitHintLabel->setObjectName("bdFitHintLabel");
    m_fitHintLabel->setWordWrap(true);
    bgl->addWidget(m_fitHintLabel, 5, 0, 1, 4);

    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BoardDisplayPanel::onModeChanged);
    connect(m_bufferLinesSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &BoardDisplayPanel::onBufferLinesChanged);

    l->addWidget(bufferGroup);
    l->addStretch();

    sa->setWidget(container);
    rootLayout->addWidget(sa);
}

void BoardDisplayPanel::setProject(Project* project) {
    if (m_project == project) return;
    if (m_project) {
        disconnect(m_project, nullptr, this, nullptr);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::projectLoaded, this, &BoardDisplayPanel::refreshFromProject);
        connect(m_project, &Project::hardwareConfigChanged, this, &BoardDisplayPanel::refreshFromProject);
        connect(m_project, &Project::boardDisplayConfigChanged, this, &BoardDisplayPanel::refreshFromProject);
    }
    refreshFromProject();
}

DeviceProfile BoardDisplayPanel::currentDevice() const {
    return m_project ? m_project->getDeviceProfile() : DeviceProfile();
}

QString BoardDisplayPanel::capabilityDeviceFor(const QString& rawMcu) const {
    const QString mcu = rawMcu.trimmed();
    if (mcu.isEmpty()) return QString();
    const QStringList devices = deviceDb().listAllDevices();
    for (const QString& d : devices) {
        if (d.trimmed().compare(mcu, Qt::CaseInsensitive) == 0 ||
            mcu.startsWith(d.trimmed(), Qt::CaseInsensitive)) {
            return d;
        }
    }
    return QString();
}

bool BoardDisplayPanel::boardMatchesDevice(const QString& boardMcu, const QString& deviceId) {
    if (boardMcu.isEmpty() || deviceId.isEmpty()) return false;
    return boardMcu.compare(deviceId, Qt::CaseInsensitive) == 0 ||
           boardMcu.startsWith(deviceId, Qt::CaseInsensitive) ||
           deviceId.startsWith(boardMcu, Qt::CaseInsensitive);
}

void BoardDisplayPanel::populateMcuCombo() {
    if (!m_mcuCombo) return;
    QSignalBlocker blocker(m_mcuCombo);
    m_mcuCombo->clear();
    m_mcuCombo->addItem("Select target MCU...", QString());
    const QStringList devices = deviceDb().listAllDevices();
    for (const QString& d : devices) {
        m_mcuCombo->addItem(d, d);
    }

    const QString current = m_project ? m_project->hardwareConfig().deviceId : QString();
    int index = current.isEmpty() ? 0 : m_mcuCombo->findData(current);
    if (index < 0 && !current.isEmpty()) {
        // A device that is not in the capability database (e.g. a raw part
        // number from an imported board): never silently replace it.
        m_mcuCombo->addItem(QString("Keep current (%1)").arg(current), current);
        index = m_mcuCombo->count() - 1;
    }
    m_mcuCombo->setCurrentIndex(index >= 0 ? index : 0);
}

void BoardDisplayPanel::rebuildBoardCombo() {
    if (!m_boardCombo) return;
    const QString deviceId = m_project ? m_project->hardwareConfig().deviceId : QString();
    const QString deviceName = currentDevice().name;

    QSignalBlocker blocker(m_boardCombo);
    m_boardCombo->clear();
    m_boardCombo->addItem("Custom / none", QString());

    const QList<BoardDef> boards = BoardRegistry::instance().boards();
    for (const BoardDef& b : boards) {
        bool include = true;
        if (!deviceId.isEmpty() || !deviceName.isEmpty()) {
            include = b.mcu.isEmpty() ||
                      boardMatchesDevice(b.mcu, deviceId) ||
                      boardMatchesDevice(b.mcu, deviceName);
        }
        if (include) {
            const QString label = b.displayName.isEmpty() ? b.id : b.displayName;
            m_boardCombo->addItem(QString("%1 (%2)").arg(label, b.id), b.id);
        }
    }

    const QString current = m_project ? m_project->hardwareConfig().boardId : QString();
    int index = current.isEmpty() ? 0 : m_boardCombo->findData(current);
    if (index < 0 && !current.isEmpty()) {
        m_boardCombo->addItem(QString("Keep current (%1)").arg(current), current);
        index = m_boardCombo->count() - 1;
    }
    m_boardCombo->setCurrentIndex(index >= 0 ? index : 0);
}

void BoardDisplayPanel::updateTargetSummary() {
    if (!m_targetSummary) return;
    const DeviceProfile dev = currentDevice();
    if (dev.name.isEmpty()) {
        m_targetSummary->setText("No target selected. Pick an MCU to see its capabilities.");
        return;
    }

    const QString core = !dev.cores.isEmpty()
        ? dev.cores.join(", ")
        : (!dev.architecture.isEmpty() ? dev.architecture : QStringLiteral("Unknown"));

    QString flash;
    if (dev.flash_is_external) {
        flash = QStringLiteral("external, depends on board");
        if (m_project && m_project->hardwareConfig().flashBytes > 0) {
            flash += QString(" (%1 KB configured)").arg(m_project->hardwareConfig().flashBytes / 1024);
        } else if (dev.external_flash_kb > 0) {
            flash += QString(" (typical module: %1 KB)").arg(dev.external_flash_kb);
        }
    } else {
        flash = QString("%1 KB on-chip").arg(dev.flash_kb);
    }

    const QString verified = dev.verified
        ? QStringLiteral("Verified against vendor sources")
        : QStringLiteral("Not vendor-verified");

    m_targetSummary->setText(QString(
        "<b>MCU:</b> %1 (%2)<br>"
        "<b>Core:</b> %3 @ %4 MHz<br>"
        "<b>RAM:</b> %5 KB total<br>"
        "<b>Flash:</b> %6<br>"
        "<b>LCD controller:</b> %7<br>"
        "<b>Data:</b> %8")
        .arg(dev.name.toHtmlEscaped(), dev.family.toHtmlEscaped(), core.toHtmlEscaped())
        .arg(dev.cpu_speed_mhz)
        .arg(dev.ram_kb)
        .arg(flash.toHtmlEscaped())
        .arg(yesNoUnknown(dev.has_lcd_controller))
        .arg(verified));
}

void BoardDisplayPanel::rebuildRegionsTable() {
    if (!m_regionsTable) return;
    const DeviceProfile dev = currentDevice();
    m_regionsTable->setRowCount(0);

    auto addRow = [this](const QString& name, const QString& size, const QString& usable, int usableFlag) {
        const int row = m_regionsTable->rowCount();
        m_regionsTable->insertRow(row);
        auto* nameItem = new QTableWidgetItem(name);
        auto* sizeItem = new QTableWidgetItem(size);
        auto* usableItem = new QTableWidgetItem(usable);
        nameItem->setToolTip(name);
        if (usableFlag == 1) usableItem->setForeground(Theme::cSuccess());
        else if (usableFlag == 0) usableItem->setForeground(Theme::cTextMuted());
        else usableItem->setForeground(Theme::cTextSecondary());
        m_regionsTable->setItem(row, 0, nameItem);
        m_regionsTable->setItem(row, 1, sizeItem);
        m_regionsTable->setItem(row, 2, usableItem);
    };

    if (dev.ram_regions.isEmpty()) {
        addRow(dev.name.isEmpty() ? QStringLiteral("No target selected")
                                  : QStringLiteral("No memory region data for this device"),
               QStringLiteral("—"), QStringLiteral("Unknown"), -1);
    } else {
        for (const DeviceProfile::RamRegion& region : dev.ram_regions) {
            addRow(region.name,
                   QString("%1 KB").arg(region.kb),
                   yesNoUnknown(region.usable_for_framebuffer),
                   region.usable_for_framebuffer);
        }
    }

    // Keep the table compact: exactly as tall as its rows, so the Buffering
    // section (fit verdict) stays visible without scrolling. Row heights come
    // from the actual rendered sections (theme font/padding aware).
    m_regionsTable->resizeRowsToContents();
    int rowsHeight = 0;
    for (int r = 0; r < m_regionsTable->rowCount(); ++r) {
        rowsHeight += m_regionsTable->rowHeight(r);
    }
    const int headerHeight = qMax(24, m_regionsTable->horizontalHeader()->sizeHint().height());
    m_regionsTable->setFixedHeight(headerHeight + rowsHeight + 10);
}

void BoardDisplayPanel::syncInterfaceCombo(QComboBox* combo, const QStringList& options,
                                           const QString& current, const QString& placeholder) {
    if (!combo) return;
    QSignalBlocker blocker(combo);
    combo->clear();
    combo->addItem(placeholder, QString());
    for (const QString& option : options) {
        combo->addItem(option, option);
    }
    int index = 0;
    if (!current.isEmpty()) {
        index = combo->findData(current);
        if (index < 0) {
            combo->addItem(QString("Keep current (%1)").arg(current), current);
            index = combo->count() - 1;
        }
    }
    combo->setCurrentIndex(index);
}

void BoardDisplayPanel::updateDisplayControls() {
    if (!m_project) return;
    const DisplayConfig dc = m_project->displayConfig();

    {
        QSignalBlocker bw(m_widthSpin);
        QSignalBlocker bh(m_heightSpin);
        m_widthSpin->setValue(dc.width > 0 ? dc.width : 320);
        m_heightSpin->setValue(dc.height > 0 ? dc.height : 240);
    }

    {
        QSignalBlocker blocker(m_pixelFormatCombo);
        m_pixelFormatCombo->clear();
        m_pixelFormatCombo->addItem("RGB565 (16-bit)", QStringLiteral("RGB565"));
        m_pixelFormatCombo->addItem("RGB888 (24-bit)", QStringLiteral("RGB888"));
        int index = -1;
        if (dc.colorDepth == 16 && dc.colorFormat.compare("RGB565", Qt::CaseInsensitive) == 0) {
            index = 0;
        } else if (dc.colorDepth == 24 && dc.colorFormat.compare("RGB888", Qt::CaseInsensitive) == 0) {
            index = 1;
        }
        if (index < 0) {
            m_pixelFormatCombo->addItem(
                QString("Keep current (%1, %2-bit)").arg(dc.colorFormat).arg(dc.colorDepth),
                QString());
            index = 2;
        }
        m_pixelFormatCombo->setCurrentIndex(index);
    }

    syncInterfaceCombo(m_interfaceCombo, BoardDisplayConfig::interfaceOptions(),
                       m_config.interfaceId, QStringLiteral("Custom / not specified"));
    syncInterfaceCombo(m_controllerCombo, BoardDisplayConfig::controllerOptions(),
                       m_config.controllerId, QStringLiteral("Custom / not specified"));
    syncInterfaceCombo(m_touchCombo, BoardDisplayConfig::touchDriverOptions(),
                       m_config.touchDriverId, QStringLiteral("None"));
}

void BoardDisplayPanel::updateBufferingControls() {
    if (m_modeCombo) {
        QSignalBlocker blocker(m_modeCombo);
        const int index = m_modeCombo->findData(static_cast<int>(m_config.framebufferMode));
        m_modeCombo->setCurrentIndex(index >= 0 ? index : 0);
    }
    if (m_bufferLinesSpin) {
        QSignalBlocker blocker(m_bufferLinesSpin);
        const int displayHeight = m_project ? qMax(1, m_project->displayConfig().height) : 240;
        m_bufferLinesSpin->setMaximum(displayHeight);
        int lines = m_config.partialBufferLines;
        lines = qBound(1, lines, displayHeight);
        m_bufferLinesSpin->setValue(lines);
        m_bufferLinesSpin->setEnabled(m_config.isPartial());
    }
}

void BoardDisplayPanel::updateComputedPreview() {
    const DeviceProfile dev = currentDevice();
    const DisplayConfig dc = m_project ? m_project->displayConfig() : DisplayConfig();

    m_lastFit = FramebufferFit::evaluate(dev, dc.width, dc.height, dc.colorDepth, m_config);

    if (m_sizeLabel) {
        m_sizeLabel->setText(QString("<b>Framebuffer size:</b> %1")
                                 .arg(FramebufferFit::formatBytes(m_lastFit.bytes)));
    }
    if (m_formulaLabel) {
        m_formulaLabel->setText(m_lastFit.formula);
    }

    if (m_fitLabel) {
        QString text;
        QString fg;
        QString bg;
        if (m_lastFit.status == FramebufferFit::Result::Status::Fits) {
            text = QStringLiteral("✓ Fits (estimated)");
            fg = Theme::Success;
            bg = Theme::SuccessBg;
        } else if (m_lastFit.status == FramebufferFit::Result::Status::DoesNotFit) {
            text = QStringLiteral("✕ Does not fit");
            fg = Theme::Danger;
            bg = Theme::DangerBg;
        } else {
            text = QStringLiteral("ℹ Region suitability not documented; cannot confirm");
            fg = Theme::Info;
            bg = Theme::InfoBg;
        }
        m_fitLabel->setText(text);
        m_fitLabel->setStyleSheet(QString(
            "background: %1; color: %2; padding: 5px 8px; border-radius: %3px; font-weight: 600;")
            .arg(bg, fg)
            .arg(Theme::RadiusSm));
    }

    if (m_fitHintLabel) {
        m_fitHintLabel->setText(m_lastFit.hint);
        const QString hintColor = m_lastFit.status == FramebufferFit::Result::Status::DoesNotFit
            ? Theme::Danger
            : (m_lastFit.regionSuitabilityUnknown ? Theme::Info : Theme::TextSecondary);
        m_fitHintLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 10px; }").arg(hintColor));
        m_fitHintLabel->setVisible(!m_lastFit.hint.isEmpty());
    }
}

void BoardDisplayPanel::refreshFromProject() {
    if (m_updating) return;
    m_updating = true;
    if (m_project) {
        m_config = m_project->boardDisplayConfig();
    }
    populateMcuCombo();
    rebuildBoardCombo();
    updateTargetSummary();
    rebuildRegionsTable();
    updateDisplayControls();
    updateBufferingControls();
    m_updating = false;
    updateComputedPreview();
}

void BoardDisplayPanel::applyConfig(const BoardDisplayConfig& config) {
    m_config = config;
    if (m_project) {
        const BoardDisplayConfig before = m_project->boardDisplayConfig();
        m_project->setBoardDisplayConfig(config);
        if (m_project->boardDisplayConfig() == before) {
            return; // Nothing changed; do not report a spurious edit.
        }
    }
    emit settingsChanged();
}

// ── Slots ────────────────────────────────────────────────────────────────────

void BoardDisplayPanel::onMcuChanged(int index) {
    Q_UNUSED(index);
    if (m_updating || !m_project) return;
    const QString name = m_mcuCombo->currentData().toString();
    if (name.isEmpty()) return; // placeholder row

    Hardware::HardwareConfig hw = m_project->hardwareConfig();
    if (hw.deviceId == name) return;
    hw.deviceId = name;

    // The board combo is filtered to this MCU: keep the board only when it
    // matches, otherwise fall back to "Custom / none" (never silently).
    if (!hw.boardId.isEmpty()) {
        const BoardDef board = BoardRegistry::instance().board(hw.boardId);
        const QString deviceName = currentDevice().name;
        const bool matches = board.mcu.isEmpty() ||
                             boardMatchesDevice(board.mcu, name) ||
                             boardMatchesDevice(board.mcu, deviceName);
        if (!matches) {
            hw.boardId.clear();
        }
    }
    hw.targetType = hw.boardId.isEmpty() ? QStringLiteral("device") : QStringLiteral("board");

    // setHardwareConfig triggers hardwareConfigChanged -> refreshFromProject().
    m_project->setHardwareConfig(hw);
}

void BoardDisplayPanel::onBoardChanged(int index) {
    Q_UNUSED(index);
    if (m_updating || !m_project) return;
    const QString boardId = m_boardCombo->currentData().toString();

    Hardware::HardwareConfig hw = m_project->hardwareConfig();
    if (hw.boardId == boardId) return;
    hw.boardId = boardId;

    if (!boardId.isEmpty()) {
        hw.targetType = QStringLiteral("board");
        const BoardDef board = BoardRegistry::instance().board(boardId);
        const QString device = capabilityDeviceFor(board.mcu);
        if (!device.isEmpty()) {
            hw.deviceId = device;
        }
    } else if (hw.deviceId.isEmpty()) {
        hw.targetType = QStringLiteral("custom");
    }

    m_project->setHardwareConfig(hw);
}

void BoardDisplayPanel::onResolutionChanged() {
    if (m_updating || !m_project) return;
    DisplayConfig dc = m_project->displayConfig();
    const int width = m_widthSpin->value();
    const int height = m_heightSpin->value();
    if (dc.width == width && dc.height == height) return;

    dc.width = width;
    dc.height = height;

    m_updating = true;
    m_project->setDisplayConfig(dc); // keeps canvas + active screen in sync
    if (m_config.partialBufferLines > height) {
        BoardDisplayConfig cfg = m_project->boardDisplayConfig();
        cfg.partialBufferLines = height;
        m_config = cfg;
        m_project->setBoardDisplayConfig(cfg);
    }
    m_updating = false;

    updateBufferingControls();
    updateComputedPreview();
    emit settingsChanged();
}

void BoardDisplayPanel::onPixelFormatChanged(int index) {
    Q_UNUSED(index);
    if (m_updating || !m_project) return;
    const QString format = m_pixelFormatCombo->currentData().toString();
    if (format.isEmpty()) return; // "Keep current" entry

    DisplayConfig dc = m_project->displayConfig();
    const int depth = format == QLatin1String("RGB888") ? 24 : 16;
    if (dc.colorFormat == format && dc.colorDepth == depth) return;

    dc.colorFormat = format;
    dc.colorDepth = depth;
    m_updating = true;
    m_project->setDisplayConfig(dc);
    m_updating = false;

    updateComputedPreview();
    emit settingsChanged();
}

void BoardDisplayPanel::onInterfaceChanged(int index) {
    Q_UNUSED(index);
    if (m_updating || !m_project) return;
    BoardDisplayConfig cfg = m_config;
    cfg.interfaceId = m_interfaceCombo->currentData().toString();
    if (cfg.interfaceId == m_config.interfaceId) return;
    applyConfig(cfg);
    updateComputedPreview();
}

void BoardDisplayPanel::onControllerChanged(int index) {
    Q_UNUSED(index);
    if (m_updating || !m_project) return;
    BoardDisplayConfig cfg = m_config;
    cfg.controllerId = m_controllerCombo->currentData().toString();
    if (cfg.controllerId == m_config.controllerId) return;
    applyConfig(cfg);
    updateComputedPreview();
}

void BoardDisplayPanel::onTouchChanged(int index) {
    Q_UNUSED(index);
    if (m_updating || !m_project) return;
    BoardDisplayConfig cfg = m_config;
    cfg.touchDriverId = m_touchCombo->currentData().toString();
    if (cfg.touchDriverId == m_config.touchDriverId) return;
    applyConfig(cfg);
    updateComputedPreview();
}

void BoardDisplayPanel::onModeChanged(int index) {
    Q_UNUSED(index);
    if (m_updating || !m_project) return;
    BoardDisplayConfig cfg = m_config;
    cfg.framebufferMode = static_cast<BoardDisplayConfig::FramebufferMode>(
        m_modeCombo->currentData().toInt());
    if (cfg.framebufferMode == m_config.framebufferMode) return;
    applyConfig(cfg);
    updateBufferingControls();
    updateComputedPreview();
}

void BoardDisplayPanel::onBufferLinesChanged(int lines) {
    if (m_updating || !m_project) return;
    BoardDisplayConfig cfg = m_config;
    cfg.partialBufferLines = lines;
    if (cfg.partialBufferLines == m_config.partialBufferLines) return;
    applyConfig(cfg);
    updateComputedPreview();
}

// ── Programmatic API ─────────────────────────────────────────────────────────

void BoardDisplayPanel::setFramebufferMode(BoardDisplayConfig::FramebufferMode mode) {
    BoardDisplayConfig cfg = m_config;
    if (cfg.framebufferMode == mode) return;
    cfg.framebufferMode = mode;
    applyConfig(cfg);
    updateBufferingControls();
    updateComputedPreview();
}

void BoardDisplayPanel::setPartialBufferLines(int lines) {
    BoardDisplayConfig cfg = m_config;
    if (cfg.partialBufferLines == lines) return;
    cfg.partialBufferLines = lines;
    applyConfig(cfg);
    updateBufferingControls();
    updateComputedPreview();
}

void BoardDisplayPanel::setInterfaceId(const QString& id) {
    if (m_config.interfaceId == id) return;
    BoardDisplayConfig cfg = m_config;
    cfg.interfaceId = id;
    applyConfig(cfg);
    updateDisplayControls();
}

void BoardDisplayPanel::setControllerId(const QString& id) {
    if (m_config.controllerId == id) return;
    BoardDisplayConfig cfg = m_config;
    cfg.controllerId = id;
    applyConfig(cfg);
    updateDisplayControls();
}

void BoardDisplayPanel::setTouchDriverId(const QString& id) {
    if (m_config.touchDriverId == id) return;
    BoardDisplayConfig cfg = m_config;
    cfg.touchDriverId = id;
    applyConfig(cfg);
    updateDisplayControls();
}

void BoardDisplayPanel::setDisplaySize(int width, int height) {
    if (m_widthSpin) m_widthSpin->setValue(width);
    if (m_heightSpin) m_heightSpin->setValue(height);
}

void BoardDisplayPanel::setPixelFormat(const QString& colorFormat, int colorDepth) {
    if (!m_project) return;
    DisplayConfig dc = m_project->displayConfig();
    if (dc.colorFormat == colorFormat && dc.colorDepth == colorDepth) return;
    dc.colorFormat = colorFormat;
    dc.colorDepth = colorDepth;
    m_updating = true;
    m_project->setDisplayConfig(dc);
    m_updating = false;
    updateDisplayControls();
    updateComputedPreview();
    emit settingsChanged();
}

void BoardDisplayPanel::setTargetDevice(const QString& deviceName) {
    if (!m_project) return;
    Hardware::HardwareConfig hw = m_project->hardwareConfig();
    if (hw.deviceId == deviceName) return;
    hw.deviceId = deviceName;
    if (!hw.boardId.isEmpty()) {
        const BoardDef board = BoardRegistry::instance().board(hw.boardId);
        if (!board.mcu.isEmpty() && !boardMatchesDevice(board.mcu, deviceName)) {
            hw.boardId.clear();
            hw.targetType = QStringLiteral("device");
        }
    } else if (hw.targetType.isEmpty()) {
        hw.targetType = QStringLiteral("device");
    }
    m_project->setHardwareConfig(hw);
}

// ── Read-only mirrors ────────────────────────────────────────────────────────

QString BoardDisplayPanel::fitStatusText() const {
    return m_fitLabel ? m_fitLabel->text() : QString();
}

QString BoardDisplayPanel::fitHintText() const {
    return m_fitHintLabel ? m_fitHintLabel->text() : QString();
}

QString BoardDisplayPanel::formulaText() const {
    return m_formulaLabel ? m_formulaLabel->text() : QString();
}

QString BoardDisplayPanel::sizeText() const {
    return m_sizeLabel ? m_sizeLabel->text() : QString();
}
