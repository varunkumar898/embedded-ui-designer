#include "NewProjectDialog.h"
#include "HardwareBridge.h"
#include "HardwareManager.h"
#include "theme/Theme.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QGroupBox>
#include <QFrame>
#include <QScrollArea>
#include <QMessageBox>

QList<TargetBoardInfo> NewProjectDialog::allBoardProfiles() {
    static QList<TargetBoardInfo> s_profiles = {
        // STM32
        {
            "stm32f030r8",
            "STM32F030R8 — NUCLEO-F030R8",
            "STM32",
            "STM32F030R8",
            "NUCLEO-F030R8",
            "ARM Cortex-M0 (48 MHz)",
            64 * 1024,
            8 * 1024,
            23,
            "ST-Link / supported OpenOCD configuration",
            {"lvgl", "ugfx"},
            "320x240 @ 16-bit RGB565 (SPI / 8-bit parallel)",
            320, 240, false, "lvgl", "stm32", "openocd"
        },
        {
            "stm32f407g_disc1",
            "STM32F407VG — STM32F4 Discovery",
            "STM32",
            "STM32F407VG",
            "STM32F407G-DISC1",
            "ARM Cortex-M4F with FPU (168 MHz)",
            1024 * 1024,
            192 * 1024,
            80,
            "ST-Link / supported OpenOCD configuration",
            {"lvgl", "ugfx", "qt-for-mcus"},
            "480x320 @ 16-bit RGB565 (FSMC / SPI)",
            480, 320, false, "lvgl", "stm32", "openocd"
        },
        {
            "nucleo_f401re",
            "STM32F401RE — NUCLEO-F401RE",
            "STM32",
            "STM32F401RE",
            "NUCLEO-F401RE",
            "ARM Cortex-M4F with FPU (84 MHz)",
            512 * 1024,
            96 * 1024,
            50,
            "ST-Link / supported OpenOCD configuration",
            {"lvgl", "ugfx", "qt-for-mcus"},
            "320x240 @ 16-bit RGB565 (SPI / I8080)",
            320, 240, false, "lvgl", "stm32", "openocd"
        },
        {
            "stm32f469i",
            "STM32F469I — STM32F469I-DISCO",
            "STM32",
            "STM32F469NIH6",
            "STM32F469I-DISCO",
            "ARM Cortex-M4F with Chrom-ART (180 MHz)",
            2048 * 1024,
            324 * 1024,
            114,
            "ST-Link / supported OpenOCD configuration",
            {"lvgl", "ugfx", "qt-for-mcus"},
            "800x480 Capacitive Touch (MIPI-DSI / Chrom-ART)",
            800, 480, false, "lvgl", "stm32", "openocd"
        },
        {
            "stm32h747i",
            "STM32H747I — STM32H747I-DISCO",
            "STM32",
            "STM32H747XI",
            "STM32H747I-DISCO",
            "Dual ARM Cortex-M7 (480 MHz) & Cortex-M4 (240 MHz)",
            2048 * 1024,
            1024 * 1024,
            132,
            "ST-Link / supported OpenOCD configuration",
            {"lvgl", "ugfx", "qt-for-mcus"},
            "800x480 @ 24-bit RGB (LTDC / MIPI-DSI)",
            800, 480, false, "lvgl", "stm32", "openocd"
        },

        // ESP32
        {
            "esp32s3_devkitc1",
            "ESP32-S3-DevKitC-1 (ESP32-S3)",
            "ESP32",
            "ESP32-S3",
            "ESP32-S3-DevKitC-1",
            "Xtensa Dual-Core 32-bit LX7 (240 MHz)",
            8192 * 1024,
            512 * 1024,
            45,
            "Native USB-JTAG / Serial esptool",
            {"lvgl", "ugfx", "qt-for-mcus"},
            "Up to 800x480 @ 16-bit (QSPI / 8-bit Parallel / RGB)",
            320, 240, false, "lvgl", "esp32", "serial"
        },
        {
            "esp32_devkitc",
            "ESP32 DevKitC (ESP32-WROOM-32)",
            "ESP32",
            "ESP32",
            "ESP32-WROOM-32",
            "Xtensa Dual-Core 32-bit LX6 (240 MHz)",
            4096 * 1024,
            520 * 1024,
            34,
            "Serial esptool / OpenOCD JTAG",
            {"lvgl", "ugfx"},
            "Up to 320x240 @ 16-bit (SPI / I8080)",
            320, 240, false, "lvgl", "esp32", "serial"
        },
        {
            "esp32_c6",
            "ESP32-C6-DevKitC-1 (ESP32-C6 RISC-V)",
            "ESP32",
            "ESP32-C6",
            "ESP32-C6-DevKitC-1",
            "32-bit RISC-V Single-Core (160 MHz)",
            4096 * 1024,
            512 * 1024,
            30,
            "USB-JTAG / Serial esptool",
            {"lvgl", "ugfx"},
            "Up to 320x240 @ 16-bit (SPI / I2C)",
            320, 240, false, "lvgl", "esp32", "serial"
        },
        {
            "esp32s3",
            "ESP32-S3-BOX (Bare / Flashed via esptool)",
            "ESP32",
            "ESP32-S3",
            "ESP32-S3-BOX",
            "Xtensa Dual-Core 32-bit LX7 (240 MHz)",
            16384 * 1024,
            512 * 1024,
            45,
            "Serial esptool / USB-JTAG",
            {"lvgl", "ugfx", "qt-for-mcus"},
            "320x240 Capacitive Touch (SPI LCD + Touch)",
            320, 240, false, "lvgl", "esp32", "serial"
        },

        // Raspberry Pi Microcontrollers (RP2040 / RP2350)
        {
            "rp2040",
            "Raspberry Pi Pico — RP2040",
            "RP2040",
            "RP2040",
            "Raspberry Pi Pico",
            "Dual ARM Cortex-M0+ (133 MHz)",
            2048 * 1024,
            264 * 1024,
            26,
            "CMSIS-DAP / Picoprobe / OpenOCD (SWD)",
            {"lvgl", "ugfx"},
            "Up to 320x240 @ 16-bit (SPI / PIO 8-bit)",
            320, 240, false, "lvgl", "raspberrypi", "openocd"
        },
        {
            "rp2350",
            "Raspberry Pi Pico 2 — RP2350",
            "RP2040",
            "RP2350",
            "Raspberry Pi Pico 2",
            "Dual ARM Cortex-M33 / Hazard3 RISC-V (150 MHz)",
            4096 * 1024,
            520 * 1024,
            26,
            "CMSIS-DAP / Picoprobe / OpenOCD (SWD)",
            {"lvgl", "ugfx", "qt-for-mcus"},
            "Up to 480x320 @ 16-bit (SPI / PIO / HSTX)",
            320, 240, false, "lvgl", "raspberrypi", "openocd"
        },

        // Raspberry Pi Linux SBC
        {
            "raspberrypi_4b",
            "Raspberry Pi 4 Model B (BCM2711 / Linux SBC)",
            "Raspberry Pi",
            "BCM2711",
            "Raspberry Pi 4B",
            "Quad ARM Cortex-A72 (64-bit ARMv8 @ 1.5 GHz)",
            8192ULL * 1024 * 1024, // 8GB Flash / MicroSD
            4096ULL * 1024 * 1024, // 4GB RAM
            28,
            "Linux Direct / Remote SSH GDB / DRM-KMS",
            {"lvgl", "qt-for-mcus", "ugfx"},
            "Up to 1920x1080 Full HD (HDMI / DSI / Linux FB)",
            800, 480, false, "lvgl", "raspberrypi", "linux"
        },

        // NXP Semiconductors
        {
            "nxp_lpc55s69",
            "NXP LPC55S69-EVK (LPC55S69 Dual Cortex-M33)",
            "NXP",
            "LPC55S69",
            "LPC55S69-EVK",
            "Dual ARM Cortex-M33 (150 MHz) with TrustZone",
            640 * 1024,
            320 * 1024,
            64,
            "LinkServer / CMSIS-DAP / OpenOCD",
            {"lvgl", "ugfx", "qt-for-mcus"},
            "Up to 480x320 @ 16-bit (SPI / Flexcomm)",
            320, 240, false, "lvgl", "stm32", "linkserver"
        },

        // STC Microcontrollers (8051)
        {
            "stc89c52",
            "STC89C52RC Microcontroller Board (8051)",
            "STC",
            "STC89C52RC",
            "STC89C52RC Board",
            "8-bit 8051 CISC (11.0592 MHz / 35 MHz)",
            8 * 1024,
            512, // 512 bytes
            32,
            "STC-ISP UART Bootloader",
            {"ugfx"},
            "128x64 Monochrome (SSD1306/ST7920) or 160x128 (ST7735)",
            128, 64, false, "ugfx", "mock", "stcisp"
        },

        // Custom Target
        {
            "custom",
            "Custom MCU or Board Profile",
            "Custom",
            "Custom MCU",
            "Custom Board",
            "Configurable Architecture",
            512 * 1024,
            64 * 1024,
            16,
            "Configurable Backend / Debug Probe",
            {"lvgl", "ugfx", "qt-for-mcus"},
            "Configurable display parameters",
            320, 240, false, "lvgl", "mock", "mock"
        },

        // Generic Starter
        {
            "generic",
            "No Target Selected (Generic Embedded Target)",
            "Generic",
            "Generic MCU",
            "Generic Target",
            "Generic Architecture",
            0,
            0,
            0,
            "Not configured",
            {"lvgl", "ugfx", "qt-for-mcus"},
            "320x240 (Standard)",
            320, 240, false, "lvgl", "mock", "mock"
        }
    };
    return s_profiles;
}

TargetBoardInfo NewProjectDialog::findBoardInfo(const QString& boardId) {
    const auto list = allBoardProfiles();
    for (const auto& info : list) {
        if (info.id.compare(boardId, Qt::CaseInsensitive) == 0) {
            return info;
        }
    }
    // Fallback generic
    return list.last();
}

NewProjectDialog::NewProjectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("New Project");
    resize(760, 680);
    setMinimumSize(680, 560);
    setStyleSheet(QString(
        "QDialog { background-color: %1; color: %2; } "
        "QLabel { color: %3; font-size: 12px; } "
        "QLineEdit, QSpinBox, QComboBox { background-color: %4; color: %2; border: 1px solid %5; border-radius: %6px; padding: 5px 8px; font-size: 12px; } "
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: %7; } "
        "QComboBox QAbstractItemView { background-color: %4; color: %2; selection-background-color: %8; selection-color: %9; border: 1px solid %10; } "
        "QTabWidget::pane { border: 1px solid %5; background-color: %11; border-radius: %12px; top: -1px; } "
        "QTabBar::tab { background-color: %1; color: %3; padding: 8px 16px; border: 1px solid %5; border-bottom: none; border-top-left-radius: %12px; border-top-right-radius: %12px; margin-right: 2px; } "
        "QTabBar::tab:selected { background-color: %11; color: %2; font-weight: 600; border-bottom: 1px solid %11; } "
        "QGroupBox { font-weight: 600; color: %13; border: 1px solid %5; border-radius: %12px; margin-top: 10px; padding-top: 10px; } "
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 6px; background-color: %11; } "
        "QPushButton { background-color: %4; color: %2; border: 1px solid %5; border-radius: %6px; padding: 6px 14px; font-weight: 600; font-size: 12px; } "
        "QPushButton:hover { background-color: %14; border-color: %7; } "
        "QPushButton:pressed { background-color: %15; } "
        "QListWidget { background-color: %4; border: 1px solid %5; border-radius: %12px; color: %2; } "
        "QListWidget::item { padding: 8px; border-radius: %16px; } "
        "QListWidget::item:selected { background-color: %8; color: %9; }"
    ).arg(Theme::BgBase, Theme::TextPrimary, Theme::TextSecondary, Theme::BgRaised, Theme::BorderSubtle,
          QString::number(Theme::RadiusMd), Theme::Accent, Theme::AccentSoft, Theme::AccentText, Theme::BorderStrong,
          Theme::BgSurface, QString::number(Theme::RadiusLg), Theme::TextSecondary, Theme::BgOverlay,
          Theme::BgBase, QString::number(Theme::RadiusSm)));

    setupUi();
    populatePlatforms();
    populateBoardsForCurrentPlatform();
    updateProfileSummary();
    validateFrameworkCompatibility();
}

void NewProjectDialog::setupUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    // 1. Project Name, Target Platform, and Microcontroller / Board form
    auto* topForm = new QFormLayout();
    topForm->setSpacing(8);
    topForm->setLabelAlignment(Qt::AlignLeft);

    m_projectNameEdit = new QLineEdit("NewEmbeddedApp", this);
    m_projectNameEdit->setPlaceholderText("Enter project name...");

    m_platformCombo = new QComboBox(this);
    m_boardCombo = new QComboBox(this);

    auto* nameLabel = new QLabel("Project name", this);
    nameLabel->setStyleSheet(QString("font-weight: 600; color: %1;").arg(Theme::TextPrimary));
    auto* platformLabel = new QLabel("Target Platform", this);
    platformLabel->setStyleSheet(QString("font-weight: 600; color: %1;").arg(Theme::TextPrimary));
    auto* boardLabel = new QLabel("Microcontroller / Board", this);
    boardLabel->setStyleSheet(QString("font-weight: 600; color: %1;").arg(Theme::TextPrimary));

    topForm->addRow(nameLabel, m_projectNameEdit);
    topForm->addRow(platformLabel, m_platformCombo);
    topForm->addRow(boardLabel, m_boardCombo);

    root->addLayout(topForm);

    // 2. Dynamic Device Profile Information Summary Box (Section 4)
    m_summaryFrame = new QFrame(this);
    m_summaryFrame->setObjectName("summaryFrame");
    m_summaryFrame->setStyleSheet(QString(
        "#summaryFrame { background-color: %1; border: 1px solid %2; border-radius: %3px; padding: 6px; }"
    ).arg(Theme::BgSurface, Theme::BorderSubtle, QString::number(Theme::RadiusMd)));
    auto* summaryLayout = new QVBoxLayout(m_summaryFrame);
    summaryLayout->setContentsMargins(8, 6, 8, 6);
    summaryLayout->setSpacing(4);

    auto* summaryHeader = new QHBoxLayout();
    m_summaryTargetLabel = new QLabel(m_summaryFrame);
    m_summaryTargetLabel->setStyleSheet(QString("font-weight: bold; color: %1; font-size: 12px;").arg(Theme::Info));
    
    m_summaryToggleBtn = new QPushButton("▲ Hide Details", m_summaryFrame);
    m_summaryToggleBtn->setStyleSheet(QString("padding: 2px 8px; font-size: 11px; background: %1; color: %2; border: 1px solid %3; border-radius: %4px;").arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle, QString::number(Theme::RadiusSm)));
    m_summaryToggleBtn->setCursor(Qt::PointingHandCursor);
    connect(m_summaryToggleBtn, &QPushButton::clicked, this, &NewProjectDialog::onToggleProfileSummary);

    summaryHeader->addWidget(m_summaryTargetLabel, 1);
    summaryHeader->addWidget(m_summaryToggleBtn);
    summaryLayout->addLayout(summaryHeader);

    m_summaryDetailsContainer = new QWidget(m_summaryFrame);
    auto* detailsLayout = new QGridLayout(m_summaryDetailsContainer);
    detailsLayout->setContentsMargins(0, 4, 0, 0);
    detailsLayout->setHorizontalSpacing(16);
    detailsLayout->setVerticalSpacing(3);

    m_summaryArchLabel = new QLabel(m_summaryDetailsContainer);
    m_summaryMemoryLabel = new QLabel(m_summaryDetailsContainer);
    m_summaryDebugLabel = new QLabel(m_summaryDetailsContainer);
    m_summaryFrameworksLabel = new QLabel(m_summaryDetailsContainer);
    m_summaryDisplayLabel = new QLabel(m_summaryDetailsContainer);

    detailsLayout->addWidget(new QLabel("Architecture:", m_summaryDetailsContainer), 0, 0);
    detailsLayout->addWidget(m_summaryArchLabel, 0, 1);
    detailsLayout->addWidget(new QLabel("Flash & RAM:", m_summaryDetailsContainer), 0, 2);
    detailsLayout->addWidget(m_summaryMemoryLabel, 0, 3);

    detailsLayout->addWidget(new QLabel("Debug / Probe:", m_summaryDetailsContainer), 1, 0);
    detailsLayout->addWidget(m_summaryDebugLabel, 1, 1);
    detailsLayout->addWidget(new QLabel("Display Rec:", m_summaryDetailsContainer), 1, 2);
    detailsLayout->addWidget(m_summaryDisplayLabel, 1, 3);

    detailsLayout->addWidget(new QLabel("Frameworks:", m_summaryDetailsContainer), 2, 0);
    detailsLayout->addWidget(m_summaryFrameworksLabel, 2, 1, 1, 3);

    summaryLayout->addWidget(m_summaryDetailsContainer);
    root->addWidget(m_summaryFrame);

    // 3. Custom Target Configuration Section (Section 6)
    m_customTargetGroup = new QGroupBox("Custom Target Hardware Specification", this);
    auto* customLayout = new QGridLayout(m_customTargetGroup);
    customLayout->setContentsMargins(10, 10, 10, 10);
    customLayout->setSpacing(8);

    m_customMcuEdit = new QLineEdit("Custom_MCU_32", m_customTargetGroup);
    m_customArchCombo = new QComboBox(m_customTargetGroup);
    m_customArchCombo->addItem("ARM Cortex-M0 / M0+", "ARM Cortex-M0+");
    m_customArchCombo->addItem("ARM Cortex-M3 / M4F", "ARM Cortex-M4F");
    m_customArchCombo->addItem("ARM Cortex-M7 / M33", "ARM Cortex-M7");
    m_customArchCombo->addItem("RISC-V 32-bit", "RISC-V 32-bit");
    m_customArchCombo->addItem("Xtensa 32-bit LX6/LX7", "Xtensa 32-bit");
    m_customArchCombo->addItem("8-bit CISC (8051/AVR)", "8-bit CISC");
    m_customArchCombo->addItem("Linux ARM64 / x86_64", "Linux ARM64");
    m_customArchCombo->addItem("Generic / Other", "Generic");

    m_customFlashSpin = new QSpinBox(m_customTargetGroup);
    m_customFlashSpin->setRange(0, 65536);
    m_customFlashSpin->setValue(512);
    m_customFlashSpin->setSuffix(" KB");

    m_customRamSpin = new QSpinBox(m_customTargetGroup);
    m_customRamSpin->setRange(0, 65536);
    m_customRamSpin->setValue(64);
    m_customRamSpin->setSuffix(" KB");

    m_customGpioSpin = new QSpinBox(m_customTargetGroup);
    m_customGpioSpin->setRange(1, 256);
    m_customGpioSpin->setValue(24);
    m_customGpioSpin->setSuffix(" pins");

    m_customDebugCombo = new QComboBox(m_customTargetGroup);
    m_customDebugCombo->addItem("OpenOCD (SWD / JTAG)", "openocd");
    m_customDebugCombo->addItem("Serial / esptool", "serial");
    m_customDebugCombo->addItem("CMSIS-DAP / Picoprobe", "openocd");
    m_customDebugCombo->addItem("LinkServer (NXP)", "linkserver");
    m_customDebugCombo->addItem("STC-ISP UART Bootloader", "stcisp");
    m_customDebugCombo->addItem("Linux Direct / SSH GDB", "linux");
    m_customDebugCombo->addItem("Mock / Simulator Only", "mock");

    customLayout->addWidget(new QLabel("MCU Name:"), 0, 0);
    customLayout->addWidget(m_customMcuEdit, 0, 1);
    customLayout->addWidget(new QLabel("Architecture:"), 0, 2);
    customLayout->addWidget(m_customArchCombo, 0, 3);

    customLayout->addWidget(new QLabel("Flash Size:"), 1, 0);
    customLayout->addWidget(m_customFlashSpin, 1, 1);
    customLayout->addWidget(new QLabel("RAM Size:"), 1, 2);
    customLayout->addWidget(m_customRamSpin, 1, 3);

    customLayout->addWidget(new QLabel("GPIO Pins:"), 2, 0);
    customLayout->addWidget(m_customGpioSpin, 2, 1);
    customLayout->addWidget(new QLabel("Debug Backend:"), 2, 2);
    customLayout->addWidget(m_customDebugCombo, 2, 3);

    root->addWidget(m_customTargetGroup);
    m_customTargetGroup->setVisible(false);

    // 4. Tabs: Blank Project and Start from Template (Sections 3, 5, 8)
    m_tabs = new QTabWidget(this);

    // Tab 1: Blank Project
    auto* blankPage = new QWidget(m_tabs);
    auto* blankLayout = new QVBoxLayout(blankPage);
    blankLayout->setContentsMargins(12, 12, 12, 12);
    blankLayout->setSpacing(8);

    auto* blankForm = new QFormLayout();
    blankForm->setSpacing(8);

    m_widthSpin = new QSpinBox(blankPage);
    m_widthSpin->setObjectName("newProjectWidth");
    m_widthSpin->setRange(64, 2000);
    m_widthSpin->setValue(320);
    m_widthSpin->setSuffix(" px");

    m_heightSpin = new QSpinBox(blankPage);
    m_heightSpin->setObjectName("newProjectHeight");
    m_heightSpin->setRange(64, 2000);
    m_heightSpin->setValue(240);
    m_heightSpin->setSuffix(" px");

    m_colorDepthSpin = new QSpinBox(blankPage);
    m_colorDepthSpin->setObjectName("newProjectColorDepth");
    m_colorDepthSpin->setRange(1, 32);
    m_colorDepthSpin->setValue(16);
    m_colorDepthSpin->setSuffix(" bpp");

    m_shapeCombo = new QComboBox(blankPage);
    m_shapeCombo->setObjectName("newProjectShape");
    m_shapeCombo->addItem("Rectangular", false);
    m_shapeCombo->addItem("Round", true);

    m_frameworkCombo = new QComboBox(blankPage);
    m_frameworkCombo->setObjectName("newProjectFramework");
    m_frameworkCombo->addItem("LVGL (Light and Versatile Graphics Library)", "lvgl");
    m_frameworkCombo->addItem("uGFX (Microcontroller Graphics Library)", "ugfx");
    m_frameworkCombo->addItem("Qt for MCUs (QUL)", "qt-for-mcus");
    m_frameworkCombo->addItem("EmbedWeb (Native Bounded C Runtime - Multi-MCU)", "embedweb");

    blankForm->addRow("Display width", m_widthSpin);
    blankForm->addRow("Display height", m_heightSpin);
    blankForm->addRow("Color depth", m_colorDepthSpin);
    blankForm->addRow("Display shape", m_shapeCombo);
    blankForm->addRow("Target framework", m_frameworkCombo);
    blankLayout->addLayout(blankForm);

    // Compatibility validation banner (Section 5)
    m_compatibilityBanner = new QLabel(blankPage);
    m_compatibilityBanner->setWordWrap(true);
    m_compatibilityBanner->setStyleSheet(QString("padding: 6px 10px; border-radius: %1px; font-size: 11px;").arg(QString::number(Theme::RadiusSm)));
    blankLayout->addWidget(m_compatibilityBanner);
    blankLayout->addStretch(1);

    m_tabs->addTab(blankPage, "Blank Project");

    // Tab 2: Start from Template (Section 8)
    auto* templatePage = new QWidget(m_tabs);
    auto* templateLayout = new QHBoxLayout(templatePage);
    templateLayout->setContentsMargins(12, 12, 12, 12);
    templateLayout->setSpacing(12);

    m_templateList = new QListWidget(templatePage);
    m_templateList->setMinimumWidth(220);

    auto* item0 = new QListWidgetItem("Thermostat", m_templateList);
    item0->setData(Qt::UserRole, ":/examples/thermostat.euiproj");
    item0->setData(Qt::UserRole + 1, "Thermostat");
    item0->setData(Qt::UserRole + 2, "Room temperature, humidity, and heating controls. Standard 320x240 display designed for STM32/ESP32.");
    item0->setData(Qt::UserRole + 3, "STM32 / ESP32 (>= 64KB Flash, >= 8KB RAM)");
    item0->setData(Qt::UserRole + 4, "LVGL / uGFX");

    auto* item1 = new QListWidgetItem("Automotive Cluster", m_templateList);
    item1->setData(Qt::UserRole, ":/examples/automotive_cluster.euiproj");
    item1->setData(Qt::UserRole + 1, "AutomotiveCluster");
    item1->setData(Qt::UserRole + 2, "A high-performance instrument panel with speed and fuel indicators. 480x272 widescreen dashboard.");
    item1->setData(Qt::UserRole + 3, "STM32F4/H7, ESP32-S3, RP2350, SBC (>= 128KB RAM)");
    item1->setData(Qt::UserRole + 4, "LVGL / Qt for MCUs");

    auto* item2 = new QListWidgetItem("Smartwatch", m_templateList);
    item2->setData(Qt::UserRole, ":/examples/smartwatch.euiproj");
    item2->setData(Qt::UserRole + 1, "Smartwatch");
    item2->setData(Qt::UserRole + 2, "A round watch face with time, activity, and battery status. 240x240 round display with circular widgets.");
    item2->setData(Qt::UserRole + 3, "RP2040 / RP2350, ESP32-S3, STM32F4");
    item2->setData(Qt::UserRole + 4, "LVGL");

    auto* templateInfo = new QWidget(templatePage);
    auto* infoLayout = new QVBoxLayout(templateInfo);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(6);

    QLabel* templateTitle = new QLabel("Template Details", templateInfo);
    templateTitle->setStyleSheet(QString("font-size: 15px; font-weight: 700; color: %1;").arg(Theme::AccentText));
    m_templateDescription = new QLabel(templateInfo);
    m_templateDescription->setWordWrap(true);
    m_templateDescription->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_templateCompatibilityLabel = new QLabel(templateInfo);
    m_templateCompatibilityLabel->setWordWrap(true);
    m_templateCompatibilityLabel->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: %3px; padding: 6px; color: %4; font-size: 11px;").arg(Theme::BgRaised, Theme::BorderSubtle, QString::number(Theme::RadiusSm), Theme::TextSecondary));

    infoLayout->addWidget(templateTitle);
    infoLayout->addWidget(m_templateDescription);
    infoLayout->addWidget(m_templateCompatibilityLabel);
    infoLayout->addStretch(1);

    templateLayout->addWidget(m_templateList);
    templateLayout->addWidget(templateInfo, 1);
    m_tabs->addTab(templatePage, "Start from Template");

    root->addWidget(m_tabs, 1);

    // 5. Create Project & Cancel Buttons
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* createButton = buttons->button(QDialogButtonBox::Ok);
    if (createButton) {
        createButton->setText("Create Project");
        createButton->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: %2; border: 1px solid %3; padding: 8px 18px; border-radius: %4px; font-weight: bold; } "
            "QPushButton:hover { background-color: %3; } "
            "QPushButton:pressed { background-color: %5; }"
        ).arg(Theme::Accent, Theme::BgBase, Theme::AccentHover, QString::number(Theme::RadiusMd), Theme::AccentPressed));
    }
    root->addWidget(buttons);

    // Connect signals
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (projectName().isEmpty()) {
            QMessageBox::warning(this, "Validation Error", "Please enter a valid project name.");
            m_projectNameEdit->setFocus();
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_platformCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NewProjectDialog::onPlatformChanged);
    connect(m_boardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NewProjectDialog::onBoardChanged);
    connect(m_frameworkCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NewProjectDialog::onFrameworkChanged);

    connect(m_shapeCombo, &QComboBox::currentTextChanged, this, [this](const QString& selected) {
        const bool round = selected == "Round";
        m_heightSpin->setEnabled(!round);
        if (round) m_heightSpin->setValue(m_widthSpin->value());
    });
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (roundDisplay()) m_heightSpin->setValue(value);
    });

    connect(m_tabs, &QTabWidget::currentChanged, this, &NewProjectDialog::updateTemplateDetails);
    connect(m_templateList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem*, QListWidgetItem*) {
        updateTemplateDetails();
    });

    m_templateList->setCurrentRow(0);
    updateTemplateDetails();
}

void NewProjectDialog::populatePlatforms() {
    m_platformCombo->blockSignals(true);
    m_platformCombo->clear();

    m_platformCombo->addItem("STMicroelectronics STM32", "STM32");
    m_platformCombo->addItem("Espressif ESP32", "ESP32");
    m_platformCombo->addItem("Raspberry Pi (RP2040 / RP2350)", "RP2040");
    m_platformCombo->addItem("Raspberry Pi (Linux SBC)", "Raspberry Pi");
    m_platformCombo->addItem("NXP Semiconductors", "NXP");
    m_platformCombo->addItem("STC Microcontrollers (8051)", "STC");
    m_platformCombo->addItem("Custom Target", "Custom");
    m_platformCombo->addItem("No Target Selected (Generic)", "Generic");

    // Default to STM32 as starter
    m_platformCombo->setCurrentIndex(0);
    m_platformCombo->blockSignals(false);
}

void NewProjectDialog::populateBoardsForCurrentPlatform() {
    m_boardCombo->blockSignals(true);
    m_boardCombo->clear();

    const QString platform = targetPlatform();
    const auto allBoards = allBoardProfiles();

    for (const auto& info : allBoards) {
        if (info.family.compare(platform, Qt::CaseInsensitive) == 0) {
            m_boardCombo->addItem(info.displayName, info.id);
        }
    }

    if (m_boardCombo->count() == 0) {
        // Fallback generic
        m_boardCombo->addItem("Generic Board Profile", "generic");
    }

    m_boardCombo->setCurrentIndex(0);
    m_boardCombo->blockSignals(false);

    // Toggle custom hardware group visibility
    m_customTargetGroup->setVisible(platform == "Custom");
}

void NewProjectDialog::onPlatformChanged() {
    populateBoardsForCurrentPlatform();
    updateProfileSummary();
    validateFrameworkCompatibility();
    updateTemplateDetails();
}

void NewProjectDialog::onBoardChanged() {
    updateProfileSummary();
    validateFrameworkCompatibility();
    updateTemplateDetails();
}

void NewProjectDialog::onFrameworkChanged() {
    validateFrameworkCompatibility();
}

void NewProjectDialog::onToggleProfileSummary() {
    m_summaryExpanded = !m_summaryExpanded;
    m_summaryDetailsContainer->setVisible(m_summaryExpanded);
    m_summaryToggleBtn->setText(m_summaryExpanded ? "▲ Hide Details" : "▼ Show Details");
}

void NewProjectDialog::updateProfileSummary() {
    TargetBoardInfo info = findBoardInfo(selectedBoardId());

    m_summaryTargetLabel->setText(QString("Target: %1").arg(info.displayName));
    m_summaryArchLabel->setText(info.architecture.isEmpty() ? "Unknown" : info.architecture);

    QString memStr;
    if (info.flashBytes > 0 || info.ramBytes > 0) {
        QString flashStr = (info.flashBytes >= 1024 * 1024) 
            ? QString("%1 MB").arg(info.flashBytes / (1024.0 * 1024.0), 0, 'f', 0)
            : QString("%1 KB").arg(info.flashBytes / 1024);
        QString ramStr = (info.ramBytes >= 1024 * 1024)
            ? QString("%1 MB").arg(info.ramBytes / (1024.0 * 1024.0), 0, 'f', 0)
            : (info.ramBytes >= 1024 ? QString("%1 KB").arg(info.ramBytes / 1024) : QString("%1 Bytes").arg(info.ramBytes));
        memStr = QString("%1 Flash, %2 RAM (%3 GPIOs)").arg(flashStr, ramStr).arg(info.gpioCount);
    } else {
        memStr = "Not configured / Unknown";
    }
    m_summaryMemoryLabel->setText(memStr);
    m_summaryDebugLabel->setText(info.debugInterface.isEmpty() ? "Not configured" : info.debugInterface);
    m_summaryDisplayLabel->setText(info.displayRecommendation.isEmpty() ? "Standard Display" : info.displayRecommendation);

    QStringList fwNames;
    for (const QString& fw : info.supportedFrameworks) {
        if (fw == "lvgl") fwNames << "LVGL";
        else if (fw == "ugfx") fwNames << "uGFX";
        else if (fw == "qt-for-mcus") fwNames << "Qt for MCUs (QUL)";
    }
    m_summaryFrameworksLabel->setText(fwNames.isEmpty() ? "All standard frameworks" : fwNames.join(", "));
}

void NewProjectDialog::validateFrameworkCompatibility() {
    const QString boardId = selectedBoardId();
    const QString framework = targetFramework();
    const TargetBoardInfo info = findBoardInfo(boardId);

    bool compatible = true;
    QString message;

    if (boardId == "stc89c52") {
        if (framework == "lvgl") {
            compatible = false;
            message = "⚠ Incompatible: LVGL requires at least 4-8 KB RAM for widget buffers and heap. STC89C52 only has 512 bytes RAM. Choose uGFX instead.";
        } else if (framework == "qt-for-mcus") {
            compatible = false;
            message = "⚠ Incompatible: Qt for MCUs requires ARM Cortex-M4/M7 or 32-bit architecture. Incompatible with 8-bit 8051.";
        } else {
            compatible = true;
            message = "✓ Compatible: uGFX minimal monochrome/SPI driver fits within 8051 8-bit memory constraints.";
        }
    } else if (boardId == "stm32f030r8") {
        if (framework == "qt-for-mcus") {
            compatible = false;
            message = "⚠ Incompatible: Qt for MCUs (QUL) requires ARM Cortex-M4/M7/M33 with hardware FPU and >= 64 KB RAM. Not supported on Cortex-M0 with 8 KB RAM. Choose LVGL or uGFX.";
        } else {
            compatible = true;
            message = QString("✓ Compatible: %1 is verified for STM32F030R8 (ARM Cortex-M0, 8 KB RAM).").arg(framework == "lvgl" ? "LVGL" : "uGFX");
        }
    } else if (info.supportedFrameworks.contains(framework)) {
        compatible = true;
        message = QString("✓ Fully Compatible: %1 is fully supported on %2 (%3).")
            .arg(framework == "lvgl" ? "LVGL" : (framework == "ugfx" ? "uGFX" : "Qt for MCUs (QUL)"))
            .arg(info.displayName)
            .arg(info.architecture);
    } else {
        compatible = false;
        message = QString("⚠ Notice: %1 has not been verified for %2. Performance or memory constraints may apply.")
            .arg(framework)
            .arg(info.displayName);
    }

    if (compatible) {
        m_compatibilityBanner->setText(message);
        m_compatibilityBanner->setStyleSheet(QString(
            "background-color: %1; border: 1px solid %2; color: %3; border-radius: %4px; padding: 6px 10px; font-size: 11px; font-weight: 500;"
        ).arg(Theme::SuccessBg, Theme::Success, Theme::Success, QString::number(Theme::RadiusSm)));
    } else {
        m_compatibilityBanner->setText(message);
        m_compatibilityBanner->setStyleSheet(QString(
            "background-color: %1; border: 1px solid %2; color: %3; border-radius: %4px; padding: 6px 10px; font-size: 11px; font-weight: 500;"
        ).arg(Theme::WarningBg, Theme::Warning, Theme::Warning, QString::number(Theme::RadiusSm)));
    }
}

void NewProjectDialog::updateTemplateDetails() {
    const QListWidgetItem* item = m_templateList->currentItem();
    if (!item) {
        m_templateDescription->clear();
        m_templateCompatibilityLabel->clear();
        return;
    }

    const QString suggestedName = item->data(Qt::UserRole + 1).toString();
    const QString desc = item->data(Qt::UserRole + 2).toString();
    const QString targetReq = item->data(Qt::UserRole + 3).toString();
    const QString fwReq = item->data(Qt::UserRole + 4).toString();

    m_templateDescription->setText(QString("%1\n\nRecommended Targets: %2\nFrameworks: %3").arg(desc, targetReq, fwReq));

    const TargetBoardInfo curBoard = findBoardInfo(selectedBoardId());
    if (selectedBoardId() == "stc89c52" && suggestedName == "AutomotiveCluster") {
        m_templateCompatibilityLabel->setText("⚠ Incompatible with selected board: Automotive Cluster requires 480x272 color display and >= 128 KB RAM (STC89C52 has only 512B RAM).");
        m_templateCompatibilityLabel->setStyleSheet(QString("background-color: %1; border: 1px solid %2; color: %3; border-radius: %4px; padding: 6px; font-size: 11px;").arg(Theme::WarningBg, Theme::Warning, Theme::Warning, QString::number(Theme::RadiusSm)));
    } else {
        m_templateCompatibilityLabel->setText(QString("✓ Compatible with %1. Click 'Create Project' to instantiate template.").arg(curBoard.displayName));
        m_templateCompatibilityLabel->setStyleSheet(QString("background-color: %1; border: 1px solid %2; color: %3; border-radius: %4px; padding: 6px; font-size: 11px;").arg(Theme::SuccessBg, Theme::Success, Theme::Success, QString::number(Theme::RadiusSm)));
    }

    if (startsFromTemplate()) {
        if (m_projectNameEdit->text().isEmpty()
            || m_projectNameEdit->text() == "NewEmbeddedApp"
            || m_projectNameEdit->text() == m_suggestedName) {
            m_projectNameEdit->setText(suggestedName);
        }
        m_suggestedName = suggestedName;
    } else if (m_projectNameEdit->text() == m_suggestedName) {
        m_projectNameEdit->setText("NewEmbeddedApp");
        m_suggestedName.clear();
    }
}

void NewProjectDialog::setSelectedPlatform(const QString& family) {
    for (int i = 0; i < m_platformCombo->count(); ++i) {
        if (m_platformCombo->itemData(i).toString().compare(family, Qt::CaseInsensitive) == 0) {
            m_platformCombo->setCurrentIndex(i);
            break;
        }
    }
}

void NewProjectDialog::setSelectedBoard(const QString& boardId) {
    // If board belongs to another platform, find and switch platform first
    TargetBoardInfo info = findBoardInfo(boardId);
    setSelectedPlatform(info.family);

    for (int i = 0; i < m_boardCombo->count(); ++i) {
        if (m_boardCombo->itemData(i).toString().compare(boardId, Qt::CaseInsensitive) == 0) {
            m_boardCombo->setCurrentIndex(i);
            break;
        }
    }
}

void NewProjectDialog::setTargetFramework(const QString& framework) {
    for (int i = 0; i < m_frameworkCombo->count(); ++i) {
        if (m_frameworkCombo->itemData(i).toString().compare(framework, Qt::CaseInsensitive) == 0) {
            m_frameworkCombo->setCurrentIndex(i);
            break;
        }
    }
}

bool NewProjectDialog::startsFromTemplate() const {
    return m_tabs->currentIndex() == 1;
}

QString NewProjectDialog::templateResource() const {
    const QListWidgetItem* item = m_templateList->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

QString NewProjectDialog::projectName() const {
    const QString name = m_projectNameEdit->text().trimmed();
    return name.isEmpty() ? QString("MyEmbeddedApp") : name;
}

int NewProjectDialog::projectWidth() const {
    return m_widthSpin->value();
}

int NewProjectDialog::projectHeight() const {
    return roundDisplay() ? m_widthSpin->value() : m_heightSpin->value();
}

QString NewProjectDialog::targetFramework() const {
    return m_frameworkCombo->currentData().toString();
}

int NewProjectDialog::colorDepth() const {
    return m_colorDepthSpin->value();
}

bool NewProjectDialog::roundDisplay() const {
    return m_shapeCombo->currentData().toBool();
}

QString NewProjectDialog::targetPlatform() const {
    return m_platformCombo ? m_platformCombo->currentData().toString() : "STM32";
}

QString NewProjectDialog::hardwareBoard() const {
    return m_boardCombo ? m_boardCombo->currentText() : "";
}

QString NewProjectDialog::selectedBoardId() const {
    return m_boardCombo ? m_boardCombo->currentData().toString() : "stm32f030r8";
}

Hardware::HardwareConfig NewProjectDialog::hardwareConfiguration() const {
    Hardware::HardwareConfig hw;
    const QString boardId = selectedBoardId();
    const TargetBoardInfo info = findBoardInfo(boardId);

    if (targetPlatform() == "Custom") {
        hw.targetType = "custom";
        hw.vendor = "Custom";
        hw.family = "Custom";
        hw.series = "Custom";
        hw.deviceId = m_customMcuEdit->text().trimmed();
        hw.boardId = "CUSTOM_BOARD";
        hw.architecture = m_customArchCombo->currentData().toString();
        hw.flashBytes = static_cast<quint64>(m_customFlashSpin->value()) * 1024;
        hw.ramBytes = static_cast<quint64>(m_customRamSpin->value()) * 1024;
        hw.toolchain.framework = targetFramework();
        hw.toolchain.toolchain = m_customDebugCombo->currentData().toString();
    } else {
        hw.targetType = (targetPlatform() == "Raspberry Pi" && boardId.contains("4b")) ? "sbc" : "board";
        hw.vendor = (info.family == "STM32") ? "STMicroelectronics" :
                    (info.family == "ESP32") ? "Espressif" :
                    (info.family.contains("RP2040") || info.family.contains("Raspberry")) ? "Raspberry Pi" :
                    (info.family == "NXP") ? "NXP" :
                    (info.family == "STC") ? "STC" : "Generic";
        hw.family = info.family;
        hw.series = info.family;
        hw.deviceId = info.mcuModel;
        hw.boardId = info.id;
        hw.architecture = info.architecture;
        hw.flashBytes = info.flashBytes;
        hw.ramBytes = info.ramBytes;
        hw.toolchain.framework = targetFramework();
        hw.toolchain.toolchain = info.debugInterface;
    }

    return hw;
}

Hardware::HardwareTarget NewProjectDialog::hardwareTarget() const {
    const QString boardId = selectedBoardId();
    const TargetBoardInfo info = findBoardInfo(boardId);

    Hardware::HardwareTarget target(info.id, info.displayName, info.family, info.mcuModel, info.id);
    target.setArchitecture(info.architecture);
    target.setBackendType(info.backendType);
    target.setConnectionType(info.connectionType);

    Hardware::HardwareCapabilities caps;
    caps.gpioInput = true;
    caps.gpioOutput = true;
    caps.liveMonitoring = true;
    caps.adc = (info.family != "STC");
    caps.pwm = (info.family != "STC");
    caps.spi = true;
    caps.i2c = true;
    caps.uart = true;
    caps.atomicBsrr = (info.family == "STM32" || info.family == "RP2040");
    target.setCapabilities(caps);

    return target;
}