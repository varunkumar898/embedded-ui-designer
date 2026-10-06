#include "HardwareWizardDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QSplitter>
#include <QFormLayout>
#include <QDebug>

HardwareWizardDialog::HardwareWizardDialog(QWidget* parent)
    : QDialog(parent)
    , m_db(Hardware::DeviceDatabase::instance())
{
    setWindowTitle("Create Embedded Project — Universal Hardware Selector");
    resize(1080, 720);

    m_db.initialize();
    setupUi();
    applyDarkEngineeringTheme();

    populateDeviceTable();
    populateBoardTable();

    // Select first device by default if available
    if (m_deviceTable->rowCount() > 0) {
        m_deviceTable->selectRow(0);
        onDeviceSelected();
    }
}

Hardware::HardwareConfig HardwareWizardDialog::hardwareConfiguration() const {
    Hardware::HardwareConfig cfg = m_engine.currentConfig();
    cfg.targetType = m_selectedTargetType;
    if (m_selectedTargetType == "board") {
        cfg.boardId = m_selectedBoard.id;
    }
    cfg.clock.sysClockMhz = m_sysClockSpin->value();
    cfg.clock.hclkMhz = m_hclkSpin->value();
    cfg.clock.oscSource = m_oscSourceCombo->currentText();
    cfg.toolchain.framework = m_frameworkCombo->currentText();
    cfg.toolchain.toolchain = m_toolchainCombo->currentText();
    cfg.toolchain.ide = m_ideCombo->currentText();
    return cfg;
}

QString HardwareWizardDialog::projectName() const {
    return m_projectNameEdit ? m_projectNameEdit->text().trimmed() : "EmbeddedApp";
}

int HardwareWizardDialog::displayWidth() const {
    return m_dispWidthSpin ? m_dispWidthSpin->value() : 320;
}

int HardwareWizardDialog::displayHeight() const {
    return m_dispHeightSpin ? m_dispHeightSpin->value() : 240;
}

void HardwareWizardDialog::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    // Header step indicator
    m_stepIndicatorLabel = new QLabel("STEP 1: Target Hardware Selection (MCU / Board / Custom)", this);
    QFont stepFont = font();
    stepFont.setBold(true);
    stepFont.setPointSize(11);
    m_stepIndicatorLabel->setFont(stepFont);
    m_stepIndicatorLabel->setStyleSheet("color: #38bdf8; padding-bottom: 4px; border-bottom: 1px solid #27272a;");
    rootLayout->addWidget(m_stepIndicatorLabel);

    // Central stack for steps
    m_stepsStack = new QStackedWidget(this);
    m_stepsStack->addWidget(createHardwareSelectionPage()); // 0
    m_stepsStack->addWidget(createPinoutPage());            // 1
    m_stepsStack->addWidget(createPeripheralPage());        // 2
    m_stepsStack->addWidget(createClockMemoryPage());       // 3
    m_stepsStack->addWidget(createToolchainPage());          // 4
    m_stepsStack->addWidget(createReviewPage());             // 5
    rootLayout->addWidget(m_stepsStack, 1);

    // Bottom Navigation Bar
    QHBoxLayout* navLayout = new QHBoxLayout();
    m_btnBack = new QPushButton("< Back", this);
    m_btnBack->setEnabled(false);
    m_btnNext = new QPushButton("Next >", this);
    m_btnCancel = new QPushButton("Cancel", this);
    m_btnFinish = new QPushButton("Finish", this);
    m_btnFinish->setEnabled(false);

    connect(m_btnBack, &QPushButton::clicked, this, &HardwareWizardDialog::onPrevStep);
    connect(m_btnNext, &QPushButton::clicked, this, &HardwareWizardDialog::onNextStep);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_btnFinish, &QPushButton::clicked, this, &HardwareWizardDialog::onFinish);

    navLayout->addWidget(m_btnBack);
    navLayout->addWidget(m_btnNext);
    navLayout->addStretch(1);
    navLayout->addWidget(m_btnCancel);
    navLayout->addWidget(m_btnFinish);
    rootLayout->addLayout(navLayout);
}

// ── STEP 1: Hardware Selection Page ──

QWidget* HardwareWizardDialog::createHardwareSelectionPage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_hwTabs = new QTabWidget(page);
    m_hwTabs->addTab(createMcuSelectorTab(), "MCU / MPU Selector");
    m_hwTabs->addTab(createBoardSelectorTab(), "Board Selector");
    m_hwTabs->addTab(createCustomConfigTab(), "Custom Hardware Configuration");
    layout->addWidget(m_hwTabs, 1);

    // Selected Target Banner
    m_targetBanner = new QLabel("Selected Hardware: None", page);
    m_targetBanner->setStyleSheet("background-color: #1e1e24; border: 1px solid #3f3f46; border-radius: 4px; padding: 8px 12px; color: #f4f4f5; font-weight: bold;");
    layout->addWidget(m_targetBanner);

    connect(m_hwTabs, &QTabWidget::currentChanged, this, [this](int idx) {
        if (idx == 0) {
            m_selectedTargetType = "device";
            onDeviceSelected();
        } else if (idx == 1) {
            m_selectedTargetType = "board";
            onBoardSelected();
        } else if (idx == 2) {
            m_selectedTargetType = "custom";
            m_targetBanner->setText("Selected Target: Custom Hardware Target");
        }
    });

    return page;
}

QWidget* HardwareWizardDialog::createMcuSelectorTab() {
    QWidget* widget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(widget);
    mainLayout->setContentsMargins(4, 8, 4, 4);
    mainLayout->setSpacing(8);

    // Filter sidebar
    QWidget* filterPanel = new QWidget(widget);
    filterPanel->setFixedWidth(220);
    QVBoxLayout* filterLayout = new QVBoxLayout(filterPanel);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(8);

    filterLayout->addWidget(new QLabel("Device Search:", filterPanel));
    m_searchDeviceEdit = new QLineEdit(filterPanel);
    m_searchDeviceEdit->setPlaceholderText("e.g. STM32, ESP32, RP2040...");
    filterLayout->addWidget(m_searchDeviceEdit);

    filterLayout->addWidget(new QLabel("Vendor:", filterPanel));
    m_vendorFilterCombo = new QComboBox(filterPanel);
    m_vendorFilterCombo->addItem("All Vendors");
    for (const QString& v : m_db.allVendors()) m_vendorFilterCombo->addItem(v);
    filterLayout->addWidget(m_vendorFilterCombo);

    filterLayout->addWidget(new QLabel("Architecture:", filterPanel));
    m_archFilterCombo = new QComboBox(filterPanel);
    m_archFilterCombo->addItem("All Architectures");
    for (const QString& a : m_db.allArchitectures()) m_archFilterCombo->addItem(a);
    filterLayout->addWidget(m_archFilterCombo);

    filterLayout->addWidget(new QLabel("Family / Series:", filterPanel));
    m_familyFilterCombo = new QComboBox(filterPanel);
    m_familyFilterCombo->addItem("All Families");
    for (const QString& f : m_db.allFamilies()) m_familyFilterCombo->addItem(f);
    filterLayout->addWidget(m_familyFilterCombo);

    filterLayout->addStretch(1);
    mainLayout->addWidget(filterPanel);

    connect(m_searchDeviceEdit, &QLineEdit::textChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_vendorFilterCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_archFilterCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_familyFilterCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);

    // Results table
    m_deviceTable = new QTableWidget(widget);
    m_deviceTable->setColumnCount(7);
    m_deviceTable->setHorizontalHeaderLabels({"Part Number", "Vendor", "Core / Arch", "Package", "Flash", "RAM", "Clock"});
    m_deviceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_deviceTable, 1);

    connect(m_deviceTable, &QTableWidget::itemSelectionChanged, this, &HardwareWizardDialog::onDeviceSelected);

    return widget;
}

QWidget* HardwareWizardDialog::createBoardSelectorTab() {
    QWidget* widget = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(widget);
    mainLayout->setContentsMargins(4, 8, 4, 4);
    mainLayout->setSpacing(8);

    QWidget* filterPanel = new QWidget(widget);
    filterPanel->setFixedWidth(220);
    QVBoxLayout* filterLayout = new QVBoxLayout(filterPanel);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(8);

    filterLayout->addWidget(new QLabel("Board Search:", filterPanel));
    m_searchBoardEdit = new QLineEdit(filterPanel);
    m_searchBoardEdit->setPlaceholderText("e.g. Discovery, Pico, DevKit...");
    filterLayout->addWidget(m_searchBoardEdit);

    filterLayout->addWidget(new QLabel("Manufacturer:", filterPanel));
    m_boardVendorFilterCombo = new QComboBox(filterPanel);
    m_boardVendorFilterCombo->addItem("All Manufacturers");
    for (const QString& v : m_db.allVendors()) m_boardVendorFilterCombo->addItem(v);
    filterLayout->addWidget(m_boardVendorFilterCombo);

    filterLayout->addStretch(1);
    mainLayout->addWidget(filterPanel);

    connect(m_searchBoardEdit, &QLineEdit::textChanged, this, &HardwareWizardDialog::populateBoardTable);
    connect(m_boardVendorFilterCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::populateBoardTable);

    m_boardTable = new QTableWidget(widget);
    m_boardTable->setColumnCount(6);
    m_boardTable->setHorizontalHeaderLabels({"Board Name", "Manufacturer", "Target MCU", "Architecture", "Debug Probe", "Supply"});
    m_boardTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_boardTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_boardTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_boardTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_boardTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_boardTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_boardTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_boardTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_boardTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_boardTable, 1);

    connect(m_boardTable, &QTableWidget::itemSelectionChanged, this, &HardwareWizardDialog::onBoardSelected);

    return widget;
}

QWidget* HardwareWizardDialog::createCustomConfigTab() {
    QWidget* widget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(widget);
    mainLayout->setContentsMargins(4, 8, 4, 4);
    mainLayout->setSpacing(8);

    QGroupBox* identGroup = new QGroupBox("Hardware Identity", widget);
    QGridLayout* identLayout = new QGridLayout(identGroup);
    identLayout->setSpacing(8);

    m_customVendorEdit = new QLineEdit("Custom Vendor", identGroup);
    m_customFamilyEdit = new QLineEdit("Custom Family", identGroup);
    m_customPartEdit = new QLineEdit("CustomMCU_v1", identGroup);
    m_customArchEdit = new QLineEdit("ARM Cortex-M4", identGroup);
    m_customCoreEdit = new QLineEdit("Cortex-M4", identGroup);
    m_customPackageEdit = new QLineEdit("LQFP64", identGroup);
    m_customFlashSpin = new QSpinBox(identGroup);
    m_customFlashSpin->setRange(16, 65536);
    m_customFlashSpin->setValue(512);
    m_customFlashSpin->setSuffix(" KB");

    m_customRamSpin = new QSpinBox(identGroup);
    m_customRamSpin->setRange(4, 65536);
    m_customRamSpin->setValue(128);
    m_customRamSpin->setSuffix(" KB");

    m_customClockSpin = new QSpinBox(identGroup);
    m_customClockSpin->setRange(1, 2000);
    m_customClockSpin->setValue(168);
    m_customClockSpin->setSuffix(" MHz");

    identLayout->addWidget(new QLabel("Vendor:"), 0, 0);
    identLayout->addWidget(m_customVendorEdit, 0, 1);
    identLayout->addWidget(new QLabel("Family:"), 0, 2);
    identLayout->addWidget(m_customFamilyEdit, 0, 3);

    identLayout->addWidget(new QLabel("Part Number:"), 1, 0);
    identLayout->addWidget(m_customPartEdit, 1, 1);
    identLayout->addWidget(new QLabel("Architecture:"), 1, 2);
    identLayout->addWidget(m_customArchEdit, 1, 3);

    identLayout->addWidget(new QLabel("Core:"), 2, 0);
    identLayout->addWidget(m_customCoreEdit, 2, 1);
    identLayout->addWidget(new QLabel("Package:"), 2, 2);
    identLayout->addWidget(m_customPackageEdit, 2, 3);

    identLayout->addWidget(new QLabel("Flash:"), 3, 0);
    identLayout->addWidget(m_customFlashSpin, 3, 1);
    identLayout->addWidget(new QLabel("RAM:"), 3, 2);
    identLayout->addWidget(m_customRamSpin, 3, 3);

    identLayout->addWidget(new QLabel("Clock:"), 4, 0);
    identLayout->addWidget(m_customClockSpin, 4, 1);
    mainLayout->addWidget(identGroup);

    // Custom Pins
    QGroupBox* pinsGroup = new QGroupBox("Custom Physical Pins", widget);
    QVBoxLayout* pinsLayout = new QVBoxLayout(pinsGroup);

    QHBoxLayout* pinBtns = new QHBoxLayout();
    QPushButton* btnAddPin = new QPushButton("+ Add Pin", pinsGroup);
    QPushButton* btnRemPin = new QPushButton("- Remove Pin", pinsGroup);
    QPushButton* btnSaveDef = new QPushButton("Save Custom Hardware Definition...", pinsGroup);
    QPushButton* btnImportDef = new QPushButton("Import Hardware Definition...", pinsGroup);

    connect(btnAddPin, &QPushButton::clicked, this, &HardwareWizardDialog::onAddCustomPin);
    connect(btnRemPin, &QPushButton::clicked, this, &HardwareWizardDialog::onRemoveCustomPin);
    connect(btnSaveDef, &QPushButton::clicked, this, &HardwareWizardDialog::onSaveCustomHardware);
    connect(btnImportDef, &QPushButton::clicked, this, &HardwareWizardDialog::onImportCustomHardware);

    pinBtns->addWidget(btnAddPin);
    pinBtns->addWidget(btnRemPin);
    pinBtns->addStretch(1);
    pinBtns->addWidget(btnSaveDef);
    pinBtns->addWidget(btnImportDef);
    pinsLayout->addLayout(pinBtns);

    m_customPinsTable = new QTableWidget(pinsGroup);
    m_customPinsTable->setColumnCount(4);
    m_customPinsTable->setHorizontalHeaderLabels({"Pin Name", "Physical Pin #", "Alternate Functions (comma-separated)", "Special Role"});
    m_customPinsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_customPinsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_customPinsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_customPinsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    pinsLayout->addWidget(m_customPinsTable);

    // Add a couple default starter pins
    QStringList starterPins = {"PA0", "PA1", "PA2", "PA3", "PA4", "PA5", "PA6", "PA7", "VDD", "GND"};
    for (int i = 0; i < starterPins.size(); ++i) {
        int r = m_customPinsTable->rowCount();
        m_customPinsTable->insertRow(r);
        m_customPinsTable->setItem(r, 0, new QTableWidgetItem(starterPins[i]));
        m_customPinsTable->setItem(r, 1, new QTableWidgetItem(QString::number(i + 1)));
        m_customPinsTable->setItem(r, 2, new QTableWidgetItem(starterPins[i] == "PA5" ? "SPI1_SCK, GPIO_Output" : "GPIO_Input, GPIO_Output"));
        m_customPinsTable->setItem(r, 3, new QTableWidgetItem(starterPins[i] == "VDD" ? "Power" : (starterPins[i] == "GND" ? "Ground" : "IO")));
    }

    mainLayout->addWidget(pinsGroup, 1);
    return widget;
}

void HardwareWizardDialog::onAddCustomPin() {
    int r = m_customPinsTable->rowCount();
    m_customPinsTable->insertRow(r);
    m_customPinsTable->setItem(r, 0, new QTableWidgetItem(QString("P%1").arg(r)));
    m_customPinsTable->setItem(r, 1, new QTableWidgetItem(QString::number(r + 1)));
    m_customPinsTable->setItem(r, 2, new QTableWidgetItem("GPIO_Input, GPIO_Output"));
    m_customPinsTable->setItem(r, 3, new QTableWidgetItem("IO"));
}

void HardwareWizardDialog::onRemoveCustomPin() {
    int r = m_customPinsTable->currentRow();
    if (r >= 0) m_customPinsTable->removeRow(r);
}

void HardwareWizardDialog::onSaveCustomHardware() {
    QString savePath = QFileDialog::getSaveFileName(this, "Save Custom Hardware Definition", "", "Hardware Definition (*.json)");
    if (savePath.isEmpty()) return;

    Hardware::DeviceDefinition dev;
    dev.vendor = m_customVendorEdit->text().trimmed();
    dev.family = m_customFamilyEdit->text().trimmed();
    dev.partNumber = m_customPartEdit->text().trimmed();
    dev.architecture = m_customArchEdit->text().trimmed();
    dev.core = m_customCoreEdit->text().trimmed();
    dev.package = m_customPackageEdit->text().trimmed();
    dev.flashBytes = static_cast<quint64>(m_customFlashSpin->value()) * 1024ULL;
    dev.ramBytes = static_cast<quint64>(m_customRamSpin->value()) * 1024ULL;
    dev.maxClockMhz = m_customClockSpin->value();
    dev.sourceProvenance = "User Defined Custom Hardware";

    for (int r = 0; r < m_customPinsTable->rowCount(); ++r) {
        Hardware::PinDefinition pin;
        pin.name = m_customPinsTable->item(r, 0) ? m_customPinsTable->item(r, 0)->text().trimmed() : "";
        pin.physicalPin = m_customPinsTable->item(r, 1) ? m_customPinsTable->item(r, 1)->text().toInt() : (r + 1);
        QString afs = m_customPinsTable->item(r, 2) ? m_customPinsTable->item(r, 2)->text() : "";
        for (const QString& part : afs.split(',')) {
            if (!part.trimmed().isEmpty()) pin.alternateFunctions.append(part.trimmed());
        }
        QString role = m_customPinsTable->item(r, 3) ? m_customPinsTable->item(r, 3)->text() : "";
        if (role.contains("Power", Qt::CaseInsensitive)) pin.isPower = true;
        if (role.contains("Ground", Qt::CaseInsensitive)) pin.isGround = true;
        if (role.contains("Reset", Qt::CaseInsensitive)) pin.isReset = true;
        dev.pins.append(pin);
    }

    m_db.addCustomDevice(dev);
    QString err;
    if (m_db.exportHardwareDefinition(dev.partNumber, savePath, &err)) {
        QMessageBox::information(this, "Hardware Saved", QString("Custom hardware '%1' saved successfully.").arg(dev.partNumber));
    } else {
        QMessageBox::warning(this, "Save Error", QString("Could not save hardware definition: %1").arg(err));
    }
}

void HardwareWizardDialog::onImportCustomHardware() {
    QString importPath = QFileDialog::getOpenFileName(this, "Import Hardware Definition", "", "Hardware Definition (*.json)");
    if (importPath.isEmpty()) return;

    QString outId, outErr;
    if (m_db.importHardwareDefinition(importPath, &outId, &outErr)) {
        QMessageBox::information(this, "Import Succeeded", QString("Hardware definition '%1' imported successfully.").arg(outId));
        populateDeviceTable();
        populateBoardTable();
    } else {
        QMessageBox::warning(this, "Import Error", QString("Failed to import hardware definition: %1").arg(outErr));
    }
}

// ── STEP 2: Pinout Configuration Page ──

QWidget* HardwareWizardDialog::createPinoutPage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // View mode selector
    QHBoxLayout* topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel("View Mode:"));
    m_pinViewModeCombo = new QComboBox(page);
    m_pinViewModeCombo->addItems({"Pin Table View", "Visual Package / Header View", "Split View"});
    topBar->addWidget(m_pinViewModeCombo);
    topBar->addStretch(1);
    layout->addLayout(topBar);

    connect(m_pinViewModeCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onPinViewModeChanged);

    m_pinViewStack = new QStackedWidget(page);

    // 0: Table View
    m_pinTable = new QTableWidget(page);
    m_pinTable->setColumnCount(9);
    m_pinTable->setHorizontalHeaderLabels({"Pin", "Label", "GPIO", "Mode", "Pull", "Speed", "Output Type", "Alternate Function", "Interrupt"});
    m_pinTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    m_pinTable->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    m_pinTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pinTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pinViewStack->addWidget(m_pinTable);

    // 1: Visual Pinout View
    m_pinoutWidget = new Hardware::PinoutViewWidget(page);
    m_pinoutWidget->setEngine(&m_engine);
    m_pinViewStack->addWidget(m_pinoutWidget);

    // 2: Split View
    QSplitter* splitter = new QSplitter(Qt::Horizontal, page);
    // clone table or keep widget in stack
    splitter->addWidget(new QLabel("Switch view above to inspect Pin Table or Package View in full detail."));
    m_pinViewStack->addWidget(splitter);

    layout->addWidget(m_pinViewStack, 1);

    connect(m_pinTable, &QTableWidget::itemSelectionChanged, this, &HardwareWizardDialog::onPinTableRowSelected);
    connect(m_pinoutWidget, &Hardware::PinoutViewWidget::pinClicked, this, &HardwareWizardDialog::onPinoutWidgetPinClicked);

    return page;
}

void HardwareWizardDialog::onPinViewModeChanged(int index) {
    if (m_pinViewStack) {
        m_pinViewStack->setCurrentIndex(index % m_pinViewStack->count());
    }
}

void HardwareWizardDialog::onPinoutWidgetPinClicked(const QString& pinName) {
    if (!m_pinTable) return;
    for (int r = 0; r < m_pinTable->rowCount(); ++r) {
        if (m_pinTable->item(r, 0) && m_pinTable->item(r, 0)->text() == pinName) {
            m_pinTable->selectRow(r);
            m_pinTable->scrollToItem(m_pinTable->item(r, 0));
            break;
        }
    }
}

void HardwareWizardDialog::onPinTableRowSelected() {
    int r = m_pinTable->currentRow();
    if (r >= 0 && m_pinTable->item(r, 0)) {
        QString pin = m_pinTable->item(r, 0)->text();
        if (m_pinoutWidget) m_pinoutWidget->setSelectedPin(pin);
    }
}

// ── STEP 3: Peripheral Configuration Page ──

QWidget* HardwareWizardDialog::createPeripheralPage() {
    QWidget* page = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    // Peripheral list
    m_peripheralListTable = new QTableWidget(page);
    m_peripheralListTable->setColumnCount(3);
    m_peripheralListTable->setHorizontalHeaderLabels({"Peripheral", "Type", "Status"});
    m_peripheralListTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_peripheralListTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_peripheralListTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_peripheralListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_peripheralListTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_peripheralListTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_peripheralListTable->setFixedWidth(300);
    layout->addWidget(m_peripheralListTable);

    connect(m_peripheralListTable, &QTableWidget::itemSelectionChanged, this, &HardwareWizardDialog::onPeripheralSelected);

    // Peripheral settings group
    m_periDetailsGroup = new QGroupBox("Peripheral Configuration", page);
    QVBoxLayout* dLayout = new QVBoxLayout(m_periDetailsGroup);
    dLayout->setSpacing(8);

    m_periEnabledCheck = new QCheckBox("Enabled", m_periDetailsGroup);
    dLayout->addWidget(m_periEnabledCheck);
    connect(m_periEnabledCheck, &QCheckBox::toggled, this, &HardwareWizardDialog::onPeripheralEnabledToggled);

    m_btnAutoAssign = new QPushButton("Auto-Assign Free Pins", m_periDetailsGroup);
    connect(m_btnAutoAssign, &QPushButton::clicked, this, &HardwareWizardDialog::onAutoAssignPeripheralPins);
    dLayout->addWidget(m_btnAutoAssign);

    m_periPinsContainer = new QWidget(m_periDetailsGroup);
    m_periPinsLayout = new QVBoxLayout(m_periPinsContainer);
    m_periPinsLayout->setContentsMargins(0, 0, 0, 0);
    dLayout->addWidget(m_periPinsContainer);

    QHBoxLayout* paramLayout = new QHBoxLayout();
    m_lblParamBaudOrFreq = new QLabel("Baud Rate / Speed:", m_periDetailsGroup);
    m_periParamBaudOrFreq = new QSpinBox(m_periDetailsGroup);
    m_periParamBaudOrFreq->setRange(1, 100000000);
    m_periParamBaudOrFreq->setValue(115200);
    paramLayout->addWidget(m_lblParamBaudOrFreq);
    paramLayout->addWidget(m_periParamBaudOrFreq);
    dLayout->addLayout(paramLayout);

    dLayout->addStretch(1);
    layout->addWidget(m_periDetailsGroup, 1);

    return page;
}

// ── STEP 4: Clock & Memory Configuration Page ──

QWidget* HardwareWizardDialog::createClockMemoryPage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    QGroupBox* clockGroup = new QGroupBox("System Clock & Oscillators", page);
    QFormLayout* cLayout = new QFormLayout(clockGroup);

    m_sysClockSpin = new QSpinBox(clockGroup);
    m_sysClockSpin->setRange(1, 2500);
    m_sysClockSpin->setValue(168);
    m_sysClockSpin->setSuffix(" MHz");
    cLayout->addRow("System Core Clock (SYSCLK):", m_sysClockSpin);

    m_oscSourceCombo = new QComboBox(clockGroup);
    m_oscSourceCombo->addItems({"Internal RC Oscillator", "External Crystal (HSE / XOSC)", "External Clock Bypass"});
    cLayout->addRow("Oscillator Source:", m_oscSourceCombo);

    m_hclkSpin = new QSpinBox(clockGroup);
    m_hclkSpin->setRange(1, 2500);
    m_hclkSpin->setValue(168);
    m_hclkSpin->setSuffix(" MHz");
    cLayout->addRow("AHB Bus Clock (HCLK):", m_hclkSpin);

    layout->addWidget(clockGroup);

    QGroupBox* memGroup = new QGroupBox("Memory Geometry", page);
    QFormLayout* mLayout = new QFormLayout(memGroup);

    m_flashSizeSpin = new QSpinBox(memGroup);
    m_flashSizeSpin->setRange(16, 65536);
    m_flashSizeSpin->setValue(1024);
    m_flashSizeSpin->setSuffix(" KB");
    mLayout->addRow("Internal Flash:", m_flashSizeSpin);

    m_ramSizeSpin = new QSpinBox(memGroup);
    m_ramSizeSpin->setRange(4, 65536);
    m_ramSizeSpin->setValue(192);
    m_ramSizeSpin->setSuffix(" KB");
    mLayout->addRow("Internal SRAM:", m_ramSizeSpin);

    layout->addWidget(memGroup);
    layout->addStretch(1);
    return page;
}

// ── STEP 5: Framework & Toolchain Selection Page ──

QWidget* HardwareWizardDialog::createToolchainPage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    QGroupBox* projGroup = new QGroupBox("Project Properties", page);
    QFormLayout* pLayout = new QFormLayout(projGroup);

    m_projectNameEdit = new QLineEdit("EmbeddedProject1", projGroup);
    pLayout->addRow("Project Name:", m_projectNameEdit);

    m_dispWidthSpin = new QSpinBox(projGroup);
    m_dispWidthSpin->setRange(64, 3840);
    m_dispWidthSpin->setValue(320);
    pLayout->addRow("Display Width (px):", m_dispWidthSpin);

    m_dispHeightSpin = new QSpinBox(projGroup);
    m_dispHeightSpin->setRange(64, 2160);
    m_dispHeightSpin->setValue(240);
    pLayout->addRow("Display Height (px):", m_dispHeightSpin);

    layout->addWidget(projGroup);

    QGroupBox* tcGroup = new QGroupBox("Embedded Development Environment", page);
    QFormLayout* tcLayout = new QFormLayout(tcGroup);

    m_frameworkCombo = new QComboBox(tcGroup);
    m_frameworkCombo->addItems({"STM32Cube (HAL/LL)", "ESP-IDF", "Pico SDK", "Linux Embedded", "Arduino", "µGFX Native", "LVGL", "Qt for MCUs"});
    tcLayout->addRow("Target Framework / Ecosystem:", m_frameworkCombo);

    m_toolchainCombo = new QComboBox(tcGroup);
    m_toolchainCombo->addItems({"arm-none-eabi-gcc", "xtensa-esp32-elf-gcc", "aarch64-linux-gnu-gcc", "clang/llvm", "gcc (native)"});
    tcLayout->addRow("Cross Compiler Toolchain:", m_toolchainCombo);

    m_ideCombo = new QComboBox(tcGroup);
    m_ideCombo->addItems({"STM32CubeIDE", "VS Code (CMake / clangd)", "PlatformIO", "CLion", "Standalone Makefile"});
    tcLayout->addRow("Target IDE / Project Generator:", m_ideCombo);

    layout->addWidget(tcGroup);
    layout->addStretch(1);

    connect(m_frameworkCombo, &QComboBox::currentTextChanged, this, &HardwareWizardDialog::onFrameworkChanged);

    return page;
}

void HardwareWizardDialog::onFrameworkChanged(const QString& framework) {
    if (framework.contains("STM32", Qt::CaseInsensitive)) {
        m_toolchainCombo->setCurrentText("arm-none-eabi-gcc");
        m_ideCombo->setCurrentText("STM32CubeIDE");
    } else if (framework.contains("ESP", Qt::CaseInsensitive)) {
        m_toolchainCombo->setCurrentText("xtensa-esp32-elf-gcc");
        m_ideCombo->setCurrentText("VS Code (CMake / clangd)");
    } else if (framework.contains("Pico", Qt::CaseInsensitive)) {
        m_toolchainCombo->setCurrentText("arm-none-eabi-gcc");
        m_ideCombo->setCurrentText("VS Code (CMake / clangd)");
    } else if (framework.contains("Linux", Qt::CaseInsensitive)) {
        m_toolchainCombo->setCurrentText("aarch64-linux-gnu-gcc");
        m_ideCombo->setCurrentText("VS Code (CMake / clangd)");
    }
}

// ── STEP 6: Review Configuration Page ──

QWidget* HardwareWizardDialog::createReviewPage() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);

    layout->addWidget(new QLabel("Configuration Summary:", page));
    m_reviewSummaryEdit = new QTextEdit(page);
    m_reviewSummaryEdit->setReadOnly(true);
    layout->addWidget(m_reviewSummaryEdit, 1);

    m_validationStatusLabel = new QLabel("Validation: Checking...", page);
    m_validationStatusLabel->setStyleSheet("padding: 8px; border-radius: 4px; font-weight: bold; background-color: #14532d; color: #86efac;");
    layout->addWidget(m_validationStatusLabel);

    return page;
}

// ── Step Navigation & Transitions ──

void HardwareWizardDialog::onNextStep() {
    int cur = m_stepsStack->currentIndex();
    if (cur < m_stepsStack->count() - 1) {
        m_stepsStack->setCurrentIndex(cur + 1);

        m_btnBack->setEnabled(true);
        if (cur + 1 == m_stepsStack->count() - 1) {
            m_btnNext->setEnabled(false);
            m_btnFinish->setEnabled(true);
            updateReviewSummary();
        }

        // Update step indicator
        QString titles[] = {
            "STEP 1: Target Hardware Selection (MCU / Board / Custom)",
            "STEP 2: Pinout Configuration & Pin Multiplexing",
            "STEP 3: Peripherals & Communication Protocols",
            "STEP 4: Clock & Memory Geometry",
            "STEP 5: Framework, Toolchain & Display Configuration",
            "STEP 6: Review & Project Initialization"
        };
        m_stepIndicatorLabel->setText(titles[cur + 1]);

        if (cur + 1 == 1) {
            populatePinTable();
        } else if (cur + 1 == 2) {
            populatePeripheralList();
        }
    }
}

void HardwareWizardDialog::onPrevStep() {
    int cur = m_stepsStack->currentIndex();
    if (cur > 0) {
        m_stepsStack->setCurrentIndex(cur - 1);
        m_btnNext->setEnabled(true);
        if (cur - 1 == 0) {
            m_btnBack->setEnabled(false);
        }

        QString titles[] = {
            "STEP 1: Target Hardware Selection (MCU / Board / Custom)",
            "STEP 2: Pinout Configuration & Pin Multiplexing",
            "STEP 3: Peripherals & Communication Protocols",
            "STEP 4: Clock & Memory Geometry",
            "STEP 5: Framework, Toolchain & Display Configuration",
            "STEP 6: Review & Project Initialization"
        };
        m_stepIndicatorLabel->setText(titles[cur - 1]);
    }
}

void HardwareWizardDialog::onFinish() {
    QStringList errors, warnings;
    bool valid = m_engine.validateConfiguration(&errors, &warnings);
    if (!valid && !errors.isEmpty()) {
        auto res = QMessageBox::question(this, "Hardware Configuration Warning",
            QString("There are configuration issues:\n- %1\n\nDo you want to proceed anyway?")
                .arg(errors.join("\n- ")),
            QMessageBox::Yes | QMessageBox::No);
        if (res != QMessageBox::Yes) {
            return;
        }
    }
    accept();
}

// ── Populate & Sync Handlers ──

void HardwareWizardDialog::populateDeviceTable() {
    m_deviceTable->setRowCount(0);
    QString search = m_searchDeviceEdit ? m_searchDeviceEdit->text().trimmed() : "";
    QString vendorFilter = m_vendorFilterCombo ? m_vendorFilterCombo->currentText() : "All Vendors";
    QString archFilter = m_archFilterCombo ? m_archFilterCombo->currentText() : "All Architectures";
    QString familyFilter = m_familyFilterCombo ? m_familyFilterCombo->currentText() : "All Families";

    for (const auto& dev : m_db.allDevices()) {
        if (!search.isEmpty()) {
            bool matches = dev.partNumber.contains(search, Qt::CaseInsensitive) ||
                           dev.vendor.contains(search, Qt::CaseInsensitive) ||
                           dev.architecture.contains(search, Qt::CaseInsensitive) ||
                           dev.core.contains(search, Qt::CaseInsensitive);
            if (!matches) continue;
        }
        if (vendorFilter != "All Vendors" && dev.vendor.compare(vendorFilter, Qt::CaseInsensitive) != 0) continue;
        if (archFilter != "All Architectures" && dev.architecture.compare(archFilter, Qt::CaseInsensitive) != 0) continue;
        if (familyFilter != "All Families" && dev.family.compare(familyFilter, Qt::CaseInsensitive) != 0) continue;

        int r = m_deviceTable->rowCount();
        m_deviceTable->insertRow(r);
        m_deviceTable->setItem(r, 0, new QTableWidgetItem(dev.partNumber));
        m_deviceTable->setItem(r, 1, new QTableWidgetItem(dev.vendor));
        m_deviceTable->setItem(r, 2, new QTableWidgetItem(QString("%1 (%2)").arg(dev.core, dev.architecture)));
        m_deviceTable->setItem(r, 3, new QTableWidgetItem(dev.package));
        m_deviceTable->setItem(r, 4, new QTableWidgetItem(QString("%1 KB").arg(dev.flashBytes / 1024)));
        m_deviceTable->setItem(r, 5, new QTableWidgetItem(QString("%1 KB").arg(dev.ramBytes / 1024)));
        m_deviceTable->setItem(r, 6, new QTableWidgetItem(QString("%1 MHz").arg(dev.maxClockMhz)));
    }
}

void HardwareWizardDialog::populateBoardTable() {
    m_boardTable->setRowCount(0);
    QString search = m_searchBoardEdit ? m_searchBoardEdit->text().trimmed() : "";
    QString vendorFilter = m_boardVendorFilterCombo ? m_boardVendorFilterCombo->currentText() : "All Manufacturers";

    for (const auto& b : m_db.allBoards()) {
        if (!search.isEmpty()) {
            bool matches = b.name.contains(search, Qt::CaseInsensitive) ||
                           b.manufacturer.contains(search, Qt::CaseInsensitive) ||
                           b.mcuPartNumber.contains(search, Qt::CaseInsensitive);
            if (!matches) continue;
        }
        if (vendorFilter != "All Manufacturers" && b.manufacturer.compare(vendorFilter, Qt::CaseInsensitive) != 0) continue;

        int r = m_boardTable->rowCount();
        m_boardTable->insertRow(r);
        m_boardTable->setItem(r, 0, new QTableWidgetItem(b.name));
        m_boardTable->setItem(r, 1, new QTableWidgetItem(b.manufacturer));
        m_boardTable->setItem(r, 2, new QTableWidgetItem(b.mcuPartNumber));
        m_boardTable->setItem(r, 3, new QTableWidgetItem(b.architecture));
        m_boardTable->setItem(r, 4, new QTableWidgetItem(b.debugInterface));
        m_boardTable->setItem(r, 5, new QTableWidgetItem(b.supplyVoltage));
    }
}

void HardwareWizardDialog::onFilterChanged() {
    populateDeviceTable();
}

void HardwareWizardDialog::onDeviceSelected() {
    int r = m_deviceTable->currentRow();
    if (r < 0 || !m_deviceTable->item(r, 0)) return;
    QString part = m_deviceTable->item(r, 0)->text();
    m_selectedDevice = m_db.findDevice(part);
    m_selectedBoard = Hardware::BoardDefinition();
    m_selectedTargetType = "device";

    syncEngineFromTarget();
    updateSelectedTargetBanner();
}

void HardwareWizardDialog::onBoardSelected() {
    int r = m_boardTable->currentRow();
    if (r < 0 || !m_boardTable->item(r, 0)) return;
    QString boardName = m_boardTable->item(r, 0)->text();
    for (const auto& b : m_db.allBoards()) {
        if (b.name == boardName) {
            m_selectedBoard = b;
            m_selectedDevice = m_db.findDevice(b.mcuPartNumber);
            m_selectedTargetType = "board";
            break;
        }
    }

    syncEngineFromTarget();
    updateSelectedTargetBanner();
}

void HardwareWizardDialog::syncEngineFromTarget() {
    if (!m_selectedDevice.partNumber.isEmpty()) {
        m_engine.setDevice(m_selectedDevice);
        if (m_sysClockSpin) m_sysClockSpin->setValue(m_selectedDevice.maxClockMhz);
        if (m_hclkSpin) m_hclkSpin->setValue(m_selectedDevice.maxClockMhz);
        if (m_flashSizeSpin) m_flashSizeSpin->setValue(static_cast<int>(m_selectedDevice.flashBytes / 1024));
        if (m_ramSizeSpin) m_ramSizeSpin->setValue(static_cast<int>(m_selectedDevice.ramBytes / 1024));

        // Sync framework default
        if (m_selectedDevice.vendor.contains("STMicro", Qt::CaseInsensitive)) {
            m_frameworkCombo->setCurrentText("STM32Cube (HAL/LL)");
        } else if (m_selectedDevice.vendor.contains("Espressif", Qt::CaseInsensitive)) {
            m_frameworkCombo->setCurrentText("ESP-IDF");
        } else if (m_selectedDevice.partNumber.contains("RP2040", Qt::CaseInsensitive)) {
            m_frameworkCombo->setCurrentText("Pico SDK");
        } else if (m_selectedDevice.partNumber.contains("BCM", Qt::CaseInsensitive)) {
            m_frameworkCombo->setCurrentText("Linux Embedded");
        }
    }
}

void HardwareWizardDialog::updateSelectedTargetBanner() {
    if (m_selectedTargetType == "board" && !m_selectedBoard.name.isEmpty()) {
        m_targetBanner->setText(QString("Selected Board: %1  |  MCU: %2  |  Core: %3  |  Flash: %4 KB  |  RAM: %5 KB")
            .arg(m_selectedBoard.name, m_selectedDevice.partNumber, m_selectedDevice.core)
            .arg(m_selectedDevice.flashBytes / 1024).arg(m_selectedDevice.ramBytes / 1024));
    } else if (!m_selectedDevice.partNumber.isEmpty()) {
        m_targetBanner->setText(QString("Selected MCU: %1  |  Vendor: %2  |  Core: %3  |  Package: %4  |  Flash: %5 KB  |  RAM: %6 KB")
            .arg(m_selectedDevice.partNumber, m_selectedDevice.vendor, m_selectedDevice.core, m_selectedDevice.package)
            .arg(m_selectedDevice.flashBytes / 1024).arg(m_selectedDevice.ramBytes / 1024));
    } else {
        m_targetBanner->setText("Selected Hardware: None");
    }
}

void HardwareWizardDialog::populatePinTable() {
    m_pinTable->blockSignals(true);
    m_pinTable->setRowCount(0);

    const auto& pins = m_selectedDevice.pins;
    for (int i = 0; i < pins.size(); ++i) {
        const auto& pinDef = pins[i];
        int r = m_pinTable->rowCount();
        m_pinTable->insertRow(r);

        m_pinTable->setItem(r, 0, new QTableWidgetItem(pinDef.name));
        m_pinTable->setItem(r, 1, new QTableWidgetItem(pinDef.name));
        m_pinTable->setItem(r, 2, new QTableWidgetItem(pinDef.name));

        // Mode combo
        QComboBox* modeCombo = new QComboBox(m_pinTable);
        modeCombo->addItems({"None", "GPIO_Input", "GPIO_Output", "Analog", "AlternateFunction", "Disabled"});
        m_pinTable->setCellWidget(r, 3, modeCombo);

        // Pull combo
        QComboBox* pullCombo = new QComboBox(m_pinTable);
        pullCombo->addItems({"NoPull", "PullUp", "PullDown"});
        m_pinTable->setCellWidget(r, 4, pullCombo);

        // Speed combo
        QComboBox* speedCombo = new QComboBox(m_pinTable);
        speedCombo->addItems({"Low", "Medium", "High", "VeryHigh"});
        m_pinTable->setCellWidget(r, 5, speedCombo);

        // Output type combo
        QComboBox* otCombo = new QComboBox(m_pinTable);
        otCombo->addItems({"PushPull", "OpenDrain"});
        m_pinTable->setCellWidget(r, 6, otCombo);

        // Alternate function combo derived from authoritative pin definition
        QComboBox* afCombo = new QComboBox(m_pinTable);
        afCombo->addItem("None");
        for (const QString& af : pinDef.alternateFunctions) {
            afCombo->addItem(af);
        }
        m_pinTable->setCellWidget(r, 7, afCombo);

        // Interrupt combo
        QComboBox* intCombo = new QComboBox(m_pinTable);
        intCombo->addItems({"None", "RisingEdge", "FallingEdge", "BothEdges"});
        m_pinTable->setCellWidget(r, 8, intCombo);

        // Wire change signals
        QString pinName = pinDef.name;
        connect(modeCombo, &QComboBox::currentTextChanged, this, [this, r](const QString& mode) {
            onPinModeChanged(r, mode);
        });
        connect(afCombo, &QComboBox::currentTextChanged, this, [this, r](const QString& af) {
            onPinAfChanged(r, af);
        });
    }

    m_pinTable->blockSignals(false);
    if (m_pinoutWidget) m_pinoutWidget->update();
}

void HardwareWizardDialog::onPinModeChanged(int row, const QString& mode) {
    if (row < 0 || !m_pinTable->item(row, 0)) return;
    QString pin = m_pinTable->item(row, 0)->text();

    Hardware::ConflictInfo conflict = m_engine.checkPinConflict(pin, mode);
    if (conflict.hasConflict && conflict.currentOwner != mode) {
        auto res = QMessageBox::question(this, "Pin Conflict",
            QString("%1 is already assigned to: %2.\n\nDo you want to replace the existing assignment?")
                .arg(pin, conflict.currentOwner),
            QMessageBox::Yes | QMessageBox::No);
        if (res != QMessageBox::Yes) {
            // Revert
            QComboBox* cb = qobject_cast<QComboBox*>(m_pinTable->cellWidget(row, 3));
            if (cb) {
                cb->blockSignals(true);
                cb->setCurrentText("None");
                cb->blockSignals(false);
            }
            return;
        }
    }

    Hardware::PinConfiguration cfg;
    cfg.pin = pin;
    cfg.gpio = pin;
    cfg.mode = mode;
    m_engine.assignPin(pin, cfg, true);
    if (m_pinoutWidget) m_pinoutWidget->update();
}

void HardwareWizardDialog::onPinAfChanged(int row, const QString& af) {
    if (row < 0 || !m_pinTable->item(row, 0)) return;
    QString pin = m_pinTable->item(row, 0)->text();
    if (af == "None") return;

    Hardware::ConflictInfo conflict = m_engine.checkPinConflict(pin, af);
    if (conflict.hasConflict && conflict.currentOwner != af) {
        auto res = QMessageBox::question(this, "Pin Conflict",
            QString("%1 is already assigned to: %2.\n\nDo you want to replace the existing assignment?")
                .arg(pin, conflict.currentOwner),
            QMessageBox::Yes | QMessageBox::No);
        if (res != QMessageBox::Yes) {
            QComboBox* cb = qobject_cast<QComboBox*>(m_pinTable->cellWidget(row, 7));
            if (cb) {
                cb->blockSignals(true);
                cb->setCurrentText("None");
                cb->blockSignals(false);
            }
            return;
        }
    }

    Hardware::PinConfiguration cfg;
    cfg.pin = pin;
    cfg.gpio = pin;
    cfg.mode = "AlternateFunction";
    cfg.alternateFunction = af;
    m_engine.assignPin(pin, cfg, true);

    // Also sync the Mode cell to AlternateFunction
    QComboBox* modeCb = qobject_cast<QComboBox*>(m_pinTable->cellWidget(row, 3));
    if (modeCb) {
        modeCb->blockSignals(true);
        modeCb->setCurrentText("AlternateFunction");
        modeCb->blockSignals(false);
    }
    if (m_pinoutWidget) m_pinoutWidget->update();
}

void HardwareWizardDialog::populatePeripheralList() {
    m_peripheralListTable->setRowCount(0);
    const auto& descs = m_selectedDevice.peripheralDescriptors;
    for (const auto& desc : descs) {
        int r = m_peripheralListTable->rowCount();
        m_peripheralListTable->insertRow(r);
        m_peripheralListTable->setItem(r, 0, new QTableWidgetItem(desc.name));
        m_peripheralListTable->setItem(r, 1, new QTableWidgetItem(desc.type));
        bool en = m_engine.currentConfig().peripherals.contains(desc.name) &&
                  m_engine.currentConfig().peripherals.value(desc.name).enabled;
        m_peripheralListTable->setItem(r, 2, new QTableWidgetItem(en ? "Enabled" : "Disabled"));
    }
    if (m_peripheralListTable->rowCount() > 0) {
        m_peripheralListTable->selectRow(0);
        onPeripheralSelected();
    }
}

void HardwareWizardDialog::onPeripheralSelected() {
    int r = m_peripheralListTable->currentRow();
    if (r < 0 || !m_peripheralListTable->item(r, 0)) return;
    QString periName = m_peripheralListTable->item(r, 0)->text();
    const Hardware::PeripheralDescriptor* desc = m_selectedDevice.findPeripheral(periName);
    if (!desc) return;

    m_periDetailsGroup->setTitle(QString("Peripheral: %1 (%2)").arg(desc->name, desc->type));

    bool isEnabled = m_engine.currentConfig().peripherals.contains(periName) &&
                     m_engine.currentConfig().peripherals.value(periName).enabled;
    m_periEnabledCheck->blockSignals(true);
    m_periEnabledCheck->setChecked(isEnabled);
    m_periEnabledCheck->blockSignals(false);

    // Rebuild signal pin combos
    QLayoutItem* item;
    while ((item = m_periPinsLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    m_periSignalCombos.clear();

    for (auto it = desc->signalOptions.begin(); it != desc->signalOptions.end(); ++it) {
        QString signalName = it.key();
        const QStringList& pinOptions = it.value();

        QHBoxLayout* row = new QHBoxLayout();
        row->addWidget(new QLabel(QString("%1 Pin:").arg(signalName)));

        QComboBox* cb = new QComboBox(m_periPinsContainer);
        cb->addItem("None");
        for (const QString& p : pinOptions) cb->addItem(p);

        // If currently configured, select it
        if (isEnabled) {
            QString curPin = m_engine.currentConfig().peripherals.value(periName).assignedPins.value(signalName);
            int idx = cb->findText(curPin);
            if (idx >= 0) cb->setCurrentIndex(idx);
        }

        row->addWidget(cb, 1);
        m_periPinsLayout->addLayout(row);
        m_periSignalCombos[signalName] = cb;

        connect(cb, &QComboBox::currentTextChanged, this, [this, signalName](const QString& pin) {
            onPeripheralSignalPinChanged(signalName, pin);
        });
    }
}

void HardwareWizardDialog::onPeripheralEnabledToggled(bool enabled) {
    int r = m_peripheralListTable->currentRow();
    if (r < 0 || !m_peripheralListTable->item(r, 0)) return;
    QString periName = m_peripheralListTable->item(r, 0)->text();

    if (!enabled) {
        m_engine.unassignPeripheral(periName);
        m_peripheralListTable->setItem(r, 2, new QTableWidgetItem("Disabled"));
    } else {
        onAutoAssignPeripheralPins();
        m_peripheralListTable->setItem(r, 2, new QTableWidgetItem("Enabled"));
    }
}

void HardwareWizardDialog::onPeripheralSignalPinChanged(const QString& signal, const QString& pin) {
    int r = m_peripheralListTable->currentRow();
    if (r < 0 || !m_peripheralListTable->item(r, 0)) return;
    QString periName = m_peripheralListTable->item(r, 0)->text();
    const Hardware::PeripheralDescriptor* desc = m_selectedDevice.findPeripheral(periName);
    if (!desc) return;

    QMap<QString, QString> signalMap;
    for (auto it = m_periSignalCombos.begin(); it != m_periSignalCombos.end(); ++it) {
        signalMap[it.key()] = it.value()->currentText() == "None" ? "" : it.value()->currentText();
    }
    signalMap[signal] = (pin == "None" ? "" : pin);

    QMap<QString, QVariant> params;
    params["speedOrFreq"] = m_periParamBaudOrFreq->value();

    m_engine.assignPeripheral(periName, desc->type, signalMap, params, true);
    m_peripheralListTable->setItem(r, 2, new QTableWidgetItem("Enabled"));
}

void HardwareWizardDialog::onAutoAssignPeripheralPins() {
    int r = m_peripheralListTable->currentRow();
    if (r < 0 || !m_peripheralListTable->item(r, 0)) return;
    QString periName = m_peripheralListTable->item(r, 0)->text();

    QString err;
    if (m_engine.autoAssignPeripheral(periName, &err)) {
        onPeripheralSelected();
        m_peripheralListTable->setItem(r, 2, new QTableWidgetItem("Enabled"));
    } else {
        QMessageBox::warning(this, "Auto-Assign Failed", err);
    }
}

void HardwareWizardDialog::updateReviewSummary() {
    QString summary;
    summary += "=== PROJECT TARGET HARDWARE ===\n";
    if (m_selectedTargetType == "board") {
        summary += QString("Board: %1\n").arg(m_selectedBoard.name);
        summary += QString("Manufacturer: %1\n").arg(m_selectedBoard.manufacturer);
    }
    summary += QString("MCU / MPU: %1\n").arg(m_selectedDevice.partNumber);
    summary += QString("Vendor: %1\n").arg(m_selectedDevice.vendor);
    summary += QString("Core / Arch: %1 (%2)\n").arg(m_selectedDevice.core, m_selectedDevice.architecture);
    summary += QString("Package: %1\n").arg(m_selectedDevice.package);

    summary += "\n=== MEMORY & CLOCK GEOMETRY ===\n";
    summary += QString("System Core Clock: %1 MHz\n").arg(m_sysClockSpin->value());
    summary += QString("Oscillator: %1\n").arg(m_oscSourceCombo->currentText());
    summary += QString("Flash: %1 KB\n").arg(m_flashSizeSpin->value());
    summary += QString("SRAM: %1 KB\n").arg(m_ramSizeSpin->value());

    summary += "\n=== EMBEDDED ECOSYSTEM & TOOLCHAIN ===\n";
    summary += QString("Project Name: %1\n").arg(projectName());
    summary += QString("Display Resolution: %1x%2\n").arg(displayWidth()).arg(displayHeight());
    summary += QString("Target Framework: %1\n").arg(m_frameworkCombo->currentText());
    summary += QString("Toolchain: %1\n").arg(m_toolchainCombo->currentText());
    summary += QString("Target IDE: %1\n").arg(m_ideCombo->currentText());

    summary += "\n=== CONFIGURED PERIPHERALS ===\n";
    int periCount = 0;
    for (auto it = m_engine.currentConfig().peripherals.begin(); it != m_engine.currentConfig().peripherals.end(); ++it) {
        if (it.value().enabled) {
            periCount++;
            summary += QString("• %1 (%2): ").arg(it.key(), it.value().type);
            QStringList pList;
            for (auto sit = it.value().assignedPins.begin(); sit != it.value().assignedPins.end(); ++sit) {
                if (!sit.value().isEmpty()) pList.append(QString("%1=%2").arg(sit.key(), sit.value()));
            }
            summary += pList.join(", ") + "\n";
        }
    }
    if (periCount == 0) summary += "No active peripherals configured.\n";

    summary += "\n=== CONFIGURED PINS ===\n";
    int pinCount = 0;
    for (auto it = m_engine.currentConfig().pins.begin(); it != m_engine.currentConfig().pins.end(); ++it) {
        if (it.value().mode != "None" && it.value().mode != "Disabled") {
            pinCount++;
            summary += QString("• %1: Mode=%2, AF=%3, Pull=%4\n")
                .arg(it.key(), it.value().mode, it.value().alternateFunction.isEmpty() ? "None" : it.value().alternateFunction, it.value().pull);
        }
    }
    if (pinCount == 0) summary += "No individual GPIO pins configured.\n";

    m_reviewSummaryEdit->setPlainText(summary);

    QStringList errors, warnings;
    bool valid = m_engine.validateConfiguration(&errors, &warnings);
    if (valid) {
        m_validationStatusLabel->setText("✓ Validation: Hardware configuration is valid. Ready to generate project.");
        m_validationStatusLabel->setStyleSheet("padding: 8px; border-radius: 4px; font-weight: bold; background-color: #14532d; color: #86efac;");
    } else {
        m_validationStatusLabel->setText(QString("⚠ Validation Warnings:\n- %1").arg(errors.join("\n- ")));
        m_validationStatusLabel->setStyleSheet("padding: 8px; border-radius: 4px; font-weight: bold; background-color: #78350f; color: #fde68a;");
    }
}

void HardwareWizardDialog::applyDarkEngineeringTheme() {
    setStyleSheet(
        "QDialog { background-color: #141418; color: #f4f4f5; }"
        "QTabWidget::pane { border: 1px solid #27272a; background-color: #18181c; border-radius: 4px; }"
        "QTabBar::tab { background: #202026; color: #a1a1aa; padding: 6px 14px; border: 1px solid #27272a; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
        "QTabBar::tab:selected { background: #272730; color: #38bdf8; font-weight: bold; }"
        "QTableWidget { background-color: #18181c; color: #f4f4f5; gridline-color: #27272a; border: 1px solid #27272a; selection-background-color: #0284c7; }"
        "QHeaderView::section { background-color: #202026; color: #d4d4d8; padding: 4px; border: 1px solid #27272a; font-weight: bold; }"
        "QGroupBox { border: 1px solid #27272a; border-radius: 4px; margin-top: 10px; font-weight: bold; color: #e4e4e7; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 4px; }"
        "QLineEdit, QSpinBox, QComboBox { background-color: #202026; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 3px; padding: 4px 6px; }"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border: 1px solid #38bdf8; }"
        "QPushButton { background-color: #272730; color: #f4f4f5; border: 1px solid #3f3f46; border-radius: 3px; padding: 6px 14px; min-width: 70px; font-weight: 500; }"
        "QPushButton:hover { background-color: #32323e; border-color: #52525b; }"
        "QPushButton:pressed { background-color: #1c1c22; }"
        "QPushButton:disabled { color: #52525b; border-color: #27272a; background-color: #18181c; }"
        "QTextEdit { background-color: #18181c; color: #f4f4f5; border: 1px solid #27272a; font-family: monospace; font-size: 11px; }"
    );
}
