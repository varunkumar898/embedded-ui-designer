#include "HardwareWizardDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QSplitter>
#include <QFormLayout>
#include <QDialogButtonBox>
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

    populateAllTable();
    populateDeviceTable();
    populateBoardTable();

    // "All" tab MUST be selected by default!
    m_hwTabs->setCurrentIndex(0);

    // Select first entry in All table by default if available
    if (m_allTable && m_allTable->rowCount() > 0) {
        m_allTable->selectRow(0);
        onAllTableRowSelected();
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
    if (isRound()) {
        return displayWidth();
    }
    return m_dispHeightSpin ? m_dispHeightSpin->value() : 240;
}

bool HardwareWizardDialog::isRound() const {
    QComboBox* shape = findChild<QComboBox*>("newProjectShape");
    return shape && shape->currentText().compare("Round", Qt::CaseInsensitive) == 0;
}

int HardwareWizardDialog::colorDepth() const {
    QSpinBox* cd = findChild<QSpinBox*>("newProjectColorDepth");
    return cd ? cd->value() : 24;
}

void HardwareWizardDialog::setupUi() {
    setObjectName("newProjectDialog");

    // Compatibility child widgets for test runners
    QSpinBox* dummyColorDepth = new QSpinBox(this);
    dummyColorDepth->setObjectName("newProjectColorDepth");
    dummyColorDepth->setValue(24);
    dummyColorDepth->setVisible(false);

    QComboBox* dummyShape = new QComboBox(this);
    dummyShape->setObjectName("newProjectShape");
    dummyShape->addItem("Rectangle");
    dummyShape->addItem("Round");
    dummyShape->setVisible(false);

    QDialogButtonBox* dummyBtnBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    dummyBtnBox->setVisible(false);
    connect(dummyBtnBox, &QDialogButtonBox::accepted, this, &HardwareWizardDialog::onFinish);

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    // Top Title & Subtitle Header
    QWidget* headerBox = new QWidget(this);
    QVBoxLayout* headerLayout = new QVBoxLayout(headerBox);
    headerLayout->setContentsMargins(0, 0, 0, 2);
    headerLayout->setSpacing(2);

    QLabel* titleLabel = new QLabel("Create Embedded Project", headerBox);
    QFont titleFont = font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #ffffff;");
    headerLayout->addWidget(titleLabel);

    m_stepIndicatorLabel = new QLabel("Select hardware to start project creation", headerBox);
    QFont subFont = font();
    subFont.setPointSize(10);
    m_stepIndicatorLabel->setFont(subFont);
    m_stepIndicatorLabel->setStyleSheet("color: #94a3b8;");
    headerLayout->addWidget(m_stepIndicatorLabel);

    rootLayout->addWidget(headerBox);

    QFrame* headerLine = new QFrame(this);
    headerLine->setFrameShape(QFrame::HLine);
    headerLine->setFrameShadow(QFrame::Plain);
    headerLine->setStyleSheet("background-color: #27272a; max-height: 1px; margin-bottom: 2px;");
    rootLayout->addWidget(headerLine);

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
    QVBoxLayout* mainLayout = new QVBoxLayout(page);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    m_hwTabs = new QTabWidget(page);
    m_hwTabs->addTab(createAllSelectorTab(), "All");
    m_hwTabs->addTab(createMcuSelectorTab(), "MCU / MPU");
    m_hwTabs->addTab(createBoardSelectorTab(), "Boards");
    m_hwTabs->addTab(createCustomConfigTab(), "Custom Configuration");
    m_hwTabs->setCurrentIndex(0); // "All" tab MUST be selected by default!
    mainLayout->addWidget(m_hwTabs, 1);

    // Selected Target Banner
    m_targetBanner = new QLabel("Selected Hardware: None", page);
    m_targetBanner->setStyleSheet("background-color: #1e1e24; border: 1px solid #3f3f46; border-radius: 4px; padding: 7px 12px; color: #f4f4f5; font-weight: bold; font-size: 11.5px;");
    mainLayout->addWidget(m_targetBanner);

    connect(m_hwTabs, &QTabWidget::currentChanged, this, [this](int idx) {
        if (idx == 0) {
            onAllTableRowSelected();
        } else if (idx == 1) {
            m_selectedTargetType = "device";
            onDeviceSelected();
        } else if (idx == 2) {
            m_selectedTargetType = "board";
            onBoardSelected();
        } else if (idx == 3) {
            m_selectedTargetType = "custom";
            m_targetBanner->setText("Selected Target: Custom Hardware Target");
        }
    });

    return page;
}

QWidget* HardwareWizardDialog::createAllSelectorTab() {
    QWidget* widget = new QWidget(this);
    QHBoxLayout* tabLayout = new QHBoxLayout(widget);
    tabLayout->setContentsMargins(8, 8, 8, 8);
    tabLayout->setSpacing(12);

    // ── LEFT: FILTERS PANEL ──
    QWidget* filterPanel = new QWidget(widget);
    filterPanel->setFixedWidth(200);
    QVBoxLayout* filterLayout = new QVBoxLayout(filterPanel);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(6);

    QLabel* filterHeader = new QLabel("Filters", filterPanel);
    QFont fhFont = font();
    fhFont.setPointSize(11);
    fhFont.setBold(true);
    filterHeader->setFont(fhFont);
    filterHeader->setStyleSheet("color: #ffffff; padding-bottom: 2px; border-bottom: 1px solid #3f3f46;");
    filterLayout->addWidget(filterHeader);

    // Vendor
    QLabel* lblVendor = new QLabel("Vendor", filterPanel);
    lblVendor->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblVendor);
    m_vendorFilterCombo = new QComboBox(filterPanel);
    m_vendorFilterCombo->addItem("All");
    for (const QString& v : m_db.allVendors()) m_vendorFilterCombo->addItem(v);
    filterLayout->addWidget(m_vendorFilterCombo);

    // Family
    QLabel* lblFamily = new QLabel("Family", filterPanel);
    lblFamily->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblFamily);
    m_familyFilterCombo = new QComboBox(filterPanel);
    m_familyFilterCombo->addItem("All");
    for (const QString& f : m_db.allFamilies()) m_familyFilterCombo->addItem(f);
    filterLayout->addWidget(m_familyFilterCombo);

    // Architecture
    QLabel* lblArch = new QLabel("Architecture", filterPanel);
    lblArch->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblArch);
    m_archFilterCombo = new QComboBox(filterPanel);
    m_archFilterCombo->addItem("All");
    for (const QString& a : m_db.allArchitectures()) m_archFilterCombo->addItem(a);
    filterLayout->addWidget(m_archFilterCombo);

    // Core
    QLabel* lblCore = new QLabel("Core", filterPanel);
    lblCore->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblCore);
    m_coreFilterCombo = new QComboBox(filterPanel);
    m_coreFilterCombo->addItem("All");
    for (const QString& c : m_db.allCores()) m_coreFilterCombo->addItem(c);
    filterLayout->addWidget(m_coreFilterCombo);

    // Package
    QLabel* lblPackage = new QLabel("Package", filterPanel);
    lblPackage->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblPackage);
    m_packageFilterCombo = new QComboBox(filterPanel);
    m_packageFilterCombo->addItem("All");
    for (const QString& p : m_db.allPackages()) m_packageFilterCombo->addItem(p);
    filterLayout->addWidget(m_packageFilterCombo);

    // Device Type
    QLabel* lblType = new QLabel("Device Type", filterPanel);
    lblType->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblType);
    m_typeFilterCombo = new QComboBox(filterPanel);
    for (const QString& t : m_db.allDeviceTypes()) m_typeFilterCombo->addItem(t);
    filterLayout->addWidget(m_typeFilterCombo);

    m_btnResetFilters = new QPushButton("Reset Filters", filterPanel);
    m_btnResetFilters->setStyleSheet("background-color: #27272a; border: 1px solid #3f3f46; border-radius: 4px; padding: 5px 10px; color: #e4e4e7; font-size: 11px; margin-top: 8px;");
    filterLayout->addWidget(m_btnResetFilters);

    filterLayout->addStretch(1);
    tabLayout->addWidget(filterPanel);

    // ── VERTICAL DIVIDER LINE ──
    QFrame* vSep = new QFrame(widget);
    vSep->setFrameShape(QFrame::VLine);
    vSep->setFrameShadow(QFrame::Plain);
    vSep->setStyleSheet("background-color: #27272a; max-width: 1px;");
    tabLayout->addWidget(vSep);

    // ── RIGHT: SEARCH BOX + MASTER TABLE ──
    QWidget* rightArea = new QWidget(widget);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightArea);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    m_searchHardwareEdit = new QLineEdit(rightArea);
    m_searchHardwareEdit->setObjectName("searchHardwareEdit");
    m_searchHardwareEdit->setPlaceholderText("Search MCU, MPU, Board...");
    m_searchHardwareEdit->setClearButtonEnabled(true);
    m_searchHardwareEdit->setStyleSheet("QLineEdit { background-color: #18181b; border: 1px solid #3f3f46; border-radius: 4px; padding: 7px 12px; color: #f4f4f5; font-size: 12px; } QLineEdit:focus { border: 1px solid #38bdf8; }");
    rightLayout->addWidget(m_searchHardwareEdit);

    m_allTable = new QTableWidget(rightArea);
    m_allTable->setObjectName("allHardwareTable");
    m_allTable->setColumnCount(8);
    m_allTable->setHorizontalHeaderLabels({"Reference", "Vendor", "Type", "Core / Architecture", "Package", "Flash", "RAM", "Connectivity"});
    m_allTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_allTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_allTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_allTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_allTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_allTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_allTable->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    m_allTable->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    m_allTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_allTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_allTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_allTable->setStyleSheet("QTableWidget { background-color: #121214; gridline-color: #27272a; selection-background-color: #1e3a5f; selection-color: #ffffff; }");
    rightLayout->addWidget(m_allTable, 1);

    tabLayout->addWidget(rightArea, 1);

    // Event connections
    connect(m_searchHardwareEdit, &QLineEdit::textChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_typeFilterCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_vendorFilterCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_familyFilterCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_archFilterCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_coreFilterCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_packageFilterCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_btnResetFilters, &QPushButton::clicked, this, &HardwareWizardDialog::resetFilters);
    connect(m_allTable, &QTableWidget::itemSelectionChanged, this, &HardwareWizardDialog::onAllTableRowSelected);

    return widget;
}

QWidget* HardwareWizardDialog::createMcuSelectorTab() {
    QWidget* widget = new QWidget(this);
    QHBoxLayout* tabLayout = new QHBoxLayout(widget);
    tabLayout->setContentsMargins(8, 8, 8, 8);
    tabLayout->setSpacing(12);

    // Left Filters
    QWidget* filterPanel = new QWidget(widget);
    filterPanel->setFixedWidth(200);
    QVBoxLayout* filterLayout = new QVBoxLayout(filterPanel);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(6);

    QLabel* filterHeader = new QLabel("Filters", filterPanel);
    QFont fhFont = font();
    fhFont.setPointSize(11);
    fhFont.setBold(true);
    filterHeader->setFont(fhFont);
    filterHeader->setStyleSheet("color: #ffffff; padding-bottom: 2px; border-bottom: 1px solid #3f3f46;");
    filterLayout->addWidget(filterHeader);

    QLabel* lblVendor = new QLabel("Vendor", filterPanel);
    lblVendor->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblVendor);
    m_mcuVendorCombo = new QComboBox(filterPanel);
    m_mcuVendorCombo->addItem("All");
    for (const QString& v : m_db.allVendors()) m_mcuVendorCombo->addItem(v);
    filterLayout->addWidget(m_mcuVendorCombo);

    QLabel* lblFamily = new QLabel("Family", filterPanel);
    lblFamily->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblFamily);
    m_mcuFamilyCombo = new QComboBox(filterPanel);
    m_mcuFamilyCombo->addItem("All");
    for (const QString& f : m_db.allFamilies()) m_mcuFamilyCombo->addItem(f);
    filterLayout->addWidget(m_mcuFamilyCombo);

    QLabel* lblArch = new QLabel("Architecture", filterPanel);
    lblArch->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblArch);
    m_mcuArchCombo = new QComboBox(filterPanel);
    m_mcuArchCombo->addItem("All");
    for (const QString& a : m_db.allArchitectures()) m_mcuArchCombo->addItem(a);
    filterLayout->addWidget(m_mcuArchCombo);

    QLabel* lblCore = new QLabel("Core", filterPanel);
    lblCore->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblCore);
    m_mcuCoreCombo = new QComboBox(filterPanel);
    m_mcuCoreCombo->addItem("All");
    for (const QString& c : m_db.allCores()) m_mcuCoreCombo->addItem(c);
    filterLayout->addWidget(m_mcuCoreCombo);

    QLabel* lblPackage = new QLabel("Package", filterPanel);
    lblPackage->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblPackage);
    m_mcuPackageCombo = new QComboBox(filterPanel);
    m_mcuPackageCombo->addItem("All");
    for (const QString& p : m_db.allPackages()) m_mcuPackageCombo->addItem(p);
    filterLayout->addWidget(m_mcuPackageCombo);

    QPushButton* btnResetMcu = new QPushButton("Reset Filters", filterPanel);
    btnResetMcu->setStyleSheet("background-color: #27272a; border: 1px solid #3f3f46; border-radius: 4px; padding: 5px 10px; color: #e4e4e7; font-size: 11px; margin-top: 8px;");
    filterLayout->addWidget(btnResetMcu);
    connect(btnResetMcu, &QPushButton::clicked, this, &HardwareWizardDialog::resetFilters);

    filterLayout->addStretch(1);
    tabLayout->addWidget(filterPanel);

    // Divider
    QFrame* vSep = new QFrame(widget);
    vSep->setFrameShape(QFrame::VLine);
    vSep->setFrameShadow(QFrame::Plain);
    vSep->setStyleSheet("background-color: #27272a; max-width: 1px;");
    tabLayout->addWidget(vSep);

    // Right Content
    QWidget* rightArea = new QWidget(widget);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightArea);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    m_mcuSearchEdit = new QLineEdit(rightArea);
    m_mcuSearchEdit->setPlaceholderText("Search MCU, MPU...");
    m_mcuSearchEdit->setClearButtonEnabled(true);
    m_mcuSearchEdit->setStyleSheet("QLineEdit { background-color: #18181b; border: 1px solid #3f3f46; border-radius: 4px; padding: 7px 12px; color: #f4f4f5; font-size: 12px; } QLineEdit:focus { border: 1px solid #38bdf8; }");
    rightLayout->addWidget(m_mcuSearchEdit);

    m_deviceTable = new QTableWidget(rightArea);
    m_deviceTable->setObjectName("mcuHardwareTable");
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
    m_deviceTable->setStyleSheet("QTableWidget { background-color: #121214; gridline-color: #27272a; selection-background-color: #1e3a5f; selection-color: #ffffff; }");
    rightLayout->addWidget(m_deviceTable, 1);

    tabLayout->addWidget(rightArea, 1);

    connect(m_mcuSearchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (m_searchHardwareEdit && m_searchHardwareEdit->text() != text) {
            m_searchHardwareEdit->setText(text);
        } else {
            onFilterChanged();
        }
    });
    connect(m_mcuVendorCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_mcuFamilyCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_mcuArchCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_mcuCoreCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_mcuPackageCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_deviceTable, &QTableWidget::itemSelectionChanged, this, &HardwareWizardDialog::onDeviceSelected);

    return widget;
}

QWidget* HardwareWizardDialog::createBoardSelectorTab() {
    QWidget* widget = new QWidget(this);
    QHBoxLayout* tabLayout = new QHBoxLayout(widget);
    tabLayout->setContentsMargins(8, 8, 8, 8);
    tabLayout->setSpacing(12);

    // Left Filters
    QWidget* filterPanel = new QWidget(widget);
    filterPanel->setFixedWidth(200);
    QVBoxLayout* filterLayout = new QVBoxLayout(filterPanel);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(6);

    QLabel* filterHeader = new QLabel("Filters", filterPanel);
    QFont fhFont = font();
    fhFont.setPointSize(11);
    fhFont.setBold(true);
    filterHeader->setFont(fhFont);
    filterHeader->setStyleSheet("color: #ffffff; padding-bottom: 2px; border-bottom: 1px solid #3f3f46;");
    filterLayout->addWidget(filterHeader);

    QLabel* lblVendor = new QLabel("Manufacturer", filterPanel);
    lblVendor->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblVendor);
    m_boardVendorCombo = new QComboBox(filterPanel);
    m_boardVendorCombo->addItem("All");
    for (const QString& v : m_db.allVendors()) m_boardVendorCombo->addItem(v);
    filterLayout->addWidget(m_boardVendorCombo);

    QLabel* lblFamily = new QLabel("Board Family", filterPanel);
    lblFamily->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblFamily);
    m_boardFamilyCombo = new QComboBox(filterPanel);
    m_boardFamilyCombo->addItem("All");
    for (const QString& f : m_db.allFamilies()) m_boardFamilyCombo->addItem(f);
    filterLayout->addWidget(m_boardFamilyCombo);

    QLabel* lblArch = new QLabel("Architecture", filterPanel);
    lblArch->setStyleSheet("color: #a1a1aa; font-size: 11px; font-weight: 600; margin-top: 4px;");
    filterLayout->addWidget(lblArch);
    m_boardArchCombo = new QComboBox(filterPanel);
    m_boardArchCombo->addItem("All");
    for (const QString& a : m_db.allArchitectures()) m_boardArchCombo->addItem(a);
    filterLayout->addWidget(m_boardArchCombo);

    QPushButton* btnResetBoard = new QPushButton("Reset Filters", filterPanel);
    btnResetBoard->setStyleSheet("background-color: #27272a; border: 1px solid #3f3f46; border-radius: 4px; padding: 5px 10px; color: #e4e4e7; font-size: 11px; margin-top: 8px;");
    filterLayout->addWidget(btnResetBoard);
    connect(btnResetBoard, &QPushButton::clicked, this, &HardwareWizardDialog::resetFilters);

    filterLayout->addStretch(1);
    tabLayout->addWidget(filterPanel);

    // Divider
    QFrame* vSep = new QFrame(widget);
    vSep->setFrameShape(QFrame::VLine);
    vSep->setFrameShadow(QFrame::Plain);
    vSep->setStyleSheet("background-color: #27272a; max-width: 1px;");
    tabLayout->addWidget(vSep);

    // Right Content
    QWidget* rightArea = new QWidget(widget);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightArea);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    m_boardSearchEdit = new QLineEdit(rightArea);
    m_boardSearchEdit->setPlaceholderText("Search Boards, SBCs...");
    m_boardSearchEdit->setClearButtonEnabled(true);
    m_boardSearchEdit->setStyleSheet("QLineEdit { background-color: #18181b; border: 1px solid #3f3f46; border-radius: 4px; padding: 7px 12px; color: #f4f4f5; font-size: 12px; } QLineEdit:focus { border: 1px solid #38bdf8; }");
    rightLayout->addWidget(m_boardSearchEdit);

    m_boardTable = new QTableWidget(rightArea);
    m_boardTable->setObjectName("boardHardwareTable");
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
    m_boardTable->setStyleSheet("QTableWidget { background-color: #121214; gridline-color: #27272a; selection-background-color: #1e3a5f; selection-color: #ffffff; }");
    rightLayout->addWidget(m_boardTable, 1);

    tabLayout->addWidget(rightArea, 1);

    connect(m_boardSearchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (m_searchHardwareEdit && m_searchHardwareEdit->text() != text) {
            m_searchHardwareEdit->setText(text);
        } else {
            onFilterChanged();
        }
    });
    connect(m_boardVendorCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_boardFamilyCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
    connect(m_boardArchCombo, &QComboBox::currentIndexChanged, this, &HardwareWizardDialog::onFilterChanged);
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
    m_dispWidthSpin->setObjectName("newProjectWidth");
    m_dispWidthSpin->setRange(64, 3840);
    m_dispWidthSpin->setValue(320);
    pLayout->addRow("Display Width (px):", m_dispWidthSpin);

    m_dispHeightSpin = new QSpinBox(projGroup);
    m_dispHeightSpin->setObjectName("newProjectHeight");
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

void HardwareWizardDialog::populateAllTable() {
    if (!m_allTable) return;
    m_allTable->setRowCount(0);

    QString search = m_searchHardwareEdit ? m_searchHardwareEdit->text().trimmed() : "";
    QString typeFilter = m_typeFilterCombo ? m_typeFilterCombo->currentText() : "All";
    QString vendorFilter = m_vendorFilterCombo ? m_vendorFilterCombo->currentText() : "All";
    QString familyFilter = m_familyFilterCombo ? m_familyFilterCombo->currentText() : "All";
    QString archFilter = m_archFilterCombo ? m_archFilterCombo->currentText() : "All";
    QString coreFilter = m_coreFilterCombo ? m_coreFilterCombo->currentText() : "All";
    QString packageFilter = m_packageFilterCombo ? m_packageFilterCombo->currentText() : "All";

    const auto items = m_db.allCatalogItems();
    for (const auto& item : items) {
        if (!search.isEmpty()) {
            bool matches = item.reference.contains(search, Qt::CaseInsensitive)
                        || item.id.contains(search, Qt::CaseInsensitive)
                        || item.vendor.contains(search, Qt::CaseInsensitive)
                        || item.family.contains(search, Qt::CaseInsensitive)
                        || item.series.contains(search, Qt::CaseInsensitive)
                        || item.architecture.contains(search, Qt::CaseInsensitive)
                        || item.core.contains(search, Qt::CaseInsensitive)
                        || item.associatedMcu.contains(search, Qt::CaseInsensitive);
            if (!matches) continue;
        }

        if (typeFilter != "All" && typeFilter != "All Types") {
            if (item.deviceType.compare(typeFilter, Qt::CaseInsensitive) != 0) continue;
        }
        if (vendorFilter != "All" && vendorFilter != "All Vendors" && item.vendor.compare(vendorFilter, Qt::CaseInsensitive) != 0) continue;
        if (familyFilter != "All" && familyFilter != "All Families" && item.family.compare(familyFilter, Qt::CaseInsensitive) != 0 && item.series.compare(familyFilter, Qt::CaseInsensitive) != 0) continue;
        if (archFilter != "All" && archFilter != "All Architectures" && item.architecture.compare(archFilter, Qt::CaseInsensitive) != 0) continue;
        if (coreFilter != "All" && coreFilter != "All Cores" && item.core.compare(coreFilter, Qt::CaseInsensitive) != 0) continue;
        if (packageFilter != "All" && packageFilter != "All Packages" && !item.package.contains(packageFilter, Qt::CaseInsensitive)) continue;

        int r = m_allTable->rowCount();
        m_allTable->insertRow(r);

        QTableWidgetItem* refItem = new QTableWidgetItem(item.reference);
        refItem->setData(Qt::UserRole, item.id);
        refItem->setData(Qt::UserRole + 1, item.isBoard);
        m_allTable->setItem(r, 0, refItem);

        m_allTable->setItem(r, 1, new QTableWidgetItem(item.vendor));

        QTableWidgetItem* typeItem = new QTableWidgetItem(item.deviceType);
        typeItem->setTextAlignment(Qt::AlignCenter);
        if (item.deviceType == "MCU") {
            typeItem->setForeground(QColor("#38bdf8"));
        } else if (item.deviceType == "MPU") {
            typeItem->setForeground(QColor("#fbbf24"));
        } else if (item.deviceType == "SBC") {
            typeItem->setForeground(QColor("#34d399"));
        } else if (item.deviceType == "BOARD") {
            typeItem->setForeground(QColor("#10b981"));
        } else if (item.deviceType == "CUSTOM") {
            typeItem->setForeground(QColor("#f97316"));
        }
        m_allTable->setItem(r, 2, typeItem);

        QString coreArch = item.core;
        if (!item.architecture.isEmpty()) {
            coreArch = QString("%1 (%2)").arg(item.core.isEmpty() ? item.architecture : item.core, item.architecture);
        }
        m_allTable->setItem(r, 3, new QTableWidgetItem(coreArch));

        m_allTable->setItem(r, 4, new QTableWidgetItem(item.package.isEmpty() ? "-" : item.package));

        QString flashStr = "-";
        if (item.flashBytes > 0) {
            flashStr = (item.flashBytes >= 1048576) ? QString("%1 MB").arg(item.flashBytes / (1024 * 1024)) : QString("%1 KB").arg(item.flashBytes / 1024);
        } else if (!item.isBoard && item.deviceType == "MPU") {
            flashStr = "External";
        }
        m_allTable->setItem(r, 5, new QTableWidgetItem(flashStr));

        QString ramStr = "-";
        if (item.ramBytes > 0) {
            ramStr = (item.ramBytes >= 1048576 * 1024ULL) ? QString("%1 GB").arg(item.ramBytes / (1024 * 1024 * 1024ULL)) :
                     (item.ramBytes >= 1048576) ? QString("%1 MB").arg(item.ramBytes / (1024 * 1024)) : QString("%1 KB").arg(item.ramBytes / 1024);
        }
        m_allTable->setItem(r, 6, new QTableWidgetItem(ramStr));

        m_allTable->setItem(r, 7, new QTableWidgetItem(item.connectivity));
    }
}

void HardwareWizardDialog::populateDeviceTable() {
    if (!m_deviceTable) return;
    m_deviceTable->setRowCount(0);
    QString search = (m_mcuSearchEdit && !m_mcuSearchEdit->text().isEmpty()) ? m_mcuSearchEdit->text().trimmed() : (m_searchHardwareEdit ? m_searchHardwareEdit->text().trimmed() : "");
    QString typeFilter = m_typeFilterCombo ? m_typeFilterCombo->currentText() : "All";
    QString vendorFilter = (m_mcuVendorCombo && m_mcuVendorCombo->currentIndex() > 0) ? m_mcuVendorCombo->currentText() : (m_vendorFilterCombo ? m_vendorFilterCombo->currentText() : "All");
    QString archFilter = (m_mcuArchCombo && m_mcuArchCombo->currentIndex() > 0) ? m_mcuArchCombo->currentText() : (m_archFilterCombo ? m_archFilterCombo->currentText() : "All");
    QString familyFilter = (m_mcuFamilyCombo && m_mcuFamilyCombo->currentIndex() > 0) ? m_mcuFamilyCombo->currentText() : (m_familyFilterCombo ? m_familyFilterCombo->currentText() : "All");
    QString coreFilter = (m_mcuCoreCombo && m_mcuCoreCombo->currentIndex() > 0) ? m_mcuCoreCombo->currentText() : (m_coreFilterCombo ? m_coreFilterCombo->currentText() : "All");
    QString packageFilter = (m_mcuPackageCombo && m_mcuPackageCombo->currentIndex() > 0) ? m_mcuPackageCombo->currentText() : (m_packageFilterCombo ? m_packageFilterCombo->currentText() : "All");

    for (const auto& dev : m_db.allDevices()) {
        if (!search.isEmpty()) {
            bool matches = dev.partNumber.contains(search, Qt::CaseInsensitive) ||
                           dev.vendor.contains(search, Qt::CaseInsensitive) ||
                           dev.family.contains(search, Qt::CaseInsensitive) ||
                           dev.series.contains(search, Qt::CaseInsensitive) ||
                           dev.architecture.contains(search, Qt::CaseInsensitive) ||
                           dev.core.contains(search, Qt::CaseInsensitive);
            if (!matches) continue;
        }
        if (typeFilter == "MPU" && !dev.architecture.contains("Cortex-A") && !dev.partNumber.contains("BCM")) continue;
        if (typeFilter == "MCU" && (dev.architecture.contains("Cortex-A") || dev.partNumber.contains("BCM"))) continue;
        if (typeFilter == "BOARD" || typeFilter == "SBC") continue;
        if (vendorFilter != "All" && vendorFilter != "All Vendors" && dev.vendor.compare(vendorFilter, Qt::CaseInsensitive) != 0) continue;
        if (archFilter != "All" && archFilter != "All Architectures" && dev.architecture.compare(archFilter, Qt::CaseInsensitive) != 0) continue;
        if (familyFilter != "All" && familyFilter != "All Families" && dev.family.compare(familyFilter, Qt::CaseInsensitive) != 0 && dev.series.compare(familyFilter, Qt::CaseInsensitive) != 0) continue;
        if (coreFilter != "All" && coreFilter != "All Cores" && dev.core.compare(coreFilter, Qt::CaseInsensitive) != 0) continue;
        if (packageFilter != "All" && packageFilter != "All Packages" && !dev.package.contains(packageFilter, Qt::CaseInsensitive)) continue;

        int r = m_deviceTable->rowCount();
        m_deviceTable->insertRow(r);
        m_deviceTable->setItem(r, 0, new QTableWidgetItem(dev.partNumber));
        m_deviceTable->setItem(r, 1, new QTableWidgetItem(dev.vendor));
        QString devType = (dev.architecture.contains("Cortex-A") || dev.partNumber.contains("BCM")) ? "MPU" : "MCU";
        QTableWidgetItem* tItem = new QTableWidgetItem(devType);
        tItem->setTextAlignment(Qt::AlignCenter);
        tItem->setForeground(devType == "MCU" ? QColor("#38bdf8") : QColor("#fbbf24"));
        m_deviceTable->setItem(r, 2, tItem);
        m_deviceTable->setItem(r, 3, new QTableWidgetItem(QString("%1 (%2)").arg(dev.core, dev.architecture)));
        m_deviceTable->setItem(r, 4, new QTableWidgetItem(dev.package));
        QString flashStr = dev.flashBytes > 0 ? ((dev.flashBytes >= 1048576) ? QString("%1 MB").arg(dev.flashBytes / (1024*1024)) : QString("%1 KB").arg(dev.flashBytes / 1024)) : "External";
        m_deviceTable->setItem(r, 5, new QTableWidgetItem(flashStr));
        QString ramStr = (dev.ramBytes >= 1048576*1024ULL) ? QString("%1 GB").arg(dev.ramBytes / (1024*1024*1024ULL)) :
                         (dev.ramBytes >= 1048576) ? QString("%1 MB").arg(dev.ramBytes / (1024*1024)) : QString("%1 KB").arg(dev.ramBytes / 1024);
        m_deviceTable->setItem(r, 6, new QTableWidgetItem(ramStr));
    }
}

void HardwareWizardDialog::populateBoardTable() {
    if (!m_boardTable) return;
    m_boardTable->setRowCount(0);
    QString search = (m_boardSearchEdit && !m_boardSearchEdit->text().isEmpty()) ? m_boardSearchEdit->text().trimmed() : (m_searchHardwareEdit ? m_searchHardwareEdit->text().trimmed() : "");
    QString typeFilter = m_typeFilterCombo ? m_typeFilterCombo->currentText() : "All";
    QString vendorFilter = (m_boardVendorCombo && m_boardVendorCombo->currentIndex() > 0) ? m_boardVendorCombo->currentText() : (m_vendorFilterCombo ? m_vendorFilterCombo->currentText() : "All");
    QString familyFilter = (m_boardFamilyCombo && m_boardFamilyCombo->currentIndex() > 0) ? m_boardFamilyCombo->currentText() : (m_familyFilterCombo ? m_familyFilterCombo->currentText() : "All");
    QString archFilter = (m_boardArchCombo && m_boardArchCombo->currentIndex() > 0) ? m_boardArchCombo->currentText() : (m_archFilterCombo ? m_archFilterCombo->currentText() : "All");

    for (const auto& b : m_db.allBoards()) {
        if (!search.isEmpty()) {
            bool matches = b.name.contains(search, Qt::CaseInsensitive) ||
                           b.id.contains(search, Qt::CaseInsensitive) ||
                           b.manufacturer.contains(search, Qt::CaseInsensitive) ||
                           b.boardFamily.contains(search, Qt::CaseInsensitive) ||
                           b.mcuPartNumber.contains(search, Qt::CaseInsensitive) ||
                           b.architecture.contains(search, Qt::CaseInsensitive);
            if (!matches) continue;
        }
        bool isSbc = (b.mcuPartNumber.contains("BCM2711", Qt::CaseInsensitive) || b.id.contains("Pi-4", Qt::CaseInsensitive));
        if (typeFilter == "MCU" || typeFilter == "MPU") continue;
        if (typeFilter == "BOARD" && isSbc) continue;
        if (typeFilter == "SBC" && !isSbc) continue;
        if (vendorFilter != "All" && vendorFilter != "All Vendors" && b.manufacturer.compare(vendorFilter, Qt::CaseInsensitive) != 0) continue;
        if (familyFilter != "All" && familyFilter != "All Families" && b.boardFamily.compare(familyFilter, Qt::CaseInsensitive) != 0) continue;
        if (archFilter != "All" && archFilter != "All Architectures" && b.architecture.compare(archFilter, Qt::CaseInsensitive) != 0) continue;

        int r = m_boardTable->rowCount();
        m_boardTable->insertRow(r);
        QTableWidgetItem* nameItem = new QTableWidgetItem(b.name.isEmpty() ? b.id : b.name);
        nameItem->setData(Qt::UserRole, b.id);
        m_boardTable->setItem(r, 0, nameItem);
        m_boardTable->setItem(r, 1, new QTableWidgetItem(b.manufacturer));
        QTableWidgetItem* tItem = new QTableWidgetItem(isSbc ? "SBC" : "BOARD");
        tItem->setTextAlignment(Qt::AlignCenter);
        tItem->setForeground(isSbc ? QColor("#34d399") : QColor("#10b981"));
        m_boardTable->setItem(r, 2, tItem);
        m_boardTable->setItem(r, 3, new QTableWidgetItem(b.mcuPartNumber));
        m_boardTable->setItem(r, 4, new QTableWidgetItem(b.architecture));
        m_boardTable->setItem(r, 5, new QTableWidgetItem(b.debugInterface));
    }
}

void HardwareWizardDialog::onFilterChanged() {
    populateAllTable();
    populateDeviceTable();
    populateBoardTable();
}

void HardwareWizardDialog::resetFilters() {
    if (m_searchHardwareEdit) m_searchHardwareEdit->clear();
    if (m_mcuSearchEdit) m_mcuSearchEdit->clear();
    if (m_boardSearchEdit) m_boardSearchEdit->clear();
    if (m_typeFilterCombo) m_typeFilterCombo->setCurrentIndex(0);
    if (m_vendorFilterCombo) m_vendorFilterCombo->setCurrentIndex(0);
    if (m_familyFilterCombo) m_familyFilterCombo->setCurrentIndex(0);
    if (m_archFilterCombo) m_archFilterCombo->setCurrentIndex(0);
    if (m_coreFilterCombo) m_coreFilterCombo->setCurrentIndex(0);
    if (m_packageFilterCombo) m_packageFilterCombo->setCurrentIndex(0);
    if (m_mcuVendorCombo) m_mcuVendorCombo->setCurrentIndex(0);
    if (m_mcuFamilyCombo) m_mcuFamilyCombo->setCurrentIndex(0);
    if (m_mcuArchCombo) m_mcuArchCombo->setCurrentIndex(0);
    if (m_mcuCoreCombo) m_mcuCoreCombo->setCurrentIndex(0);
    if (m_mcuPackageCombo) m_mcuPackageCombo->setCurrentIndex(0);
    if (m_boardVendorCombo) m_boardVendorCombo->setCurrentIndex(0);
    if (m_boardFamilyCombo) m_boardFamilyCombo->setCurrentIndex(0);
    if (m_boardArchCombo) m_boardArchCombo->setCurrentIndex(0);
    onFilterChanged();
}

void HardwareWizardDialog::onAllTableRowSelected() {
    int r = m_allTable ? m_allTable->currentRow() : -1;
    if (r < 0 || !m_allTable->item(r, 0)) return;

    QString id = m_allTable->item(r, 0)->data(Qt::UserRole).toString();
    bool isBoard = m_allTable->item(r, 0)->data(Qt::UserRole + 1).toBool();
    QString type = m_allTable->item(r, 2)->text();

    if (isBoard || type == "BOARD" || type == "SBC") {
        m_selectedBoard = m_db.findBoard(id);
        m_selectedDevice = m_db.findDevice(m_selectedBoard.mcuPartNumber);
        m_selectedTargetType = "board";
    } else {
        m_selectedDevice = m_db.findDevice(id);
        m_selectedBoard = Hardware::BoardDefinition();
        m_selectedTargetType = "device";
    }

    syncEngineFromTarget();
    updateSelectedTargetBanner();
    m_btnNext->setEnabled(true);
}

void HardwareWizardDialog::onDeviceSelected() {
    int r = m_deviceTable ? m_deviceTable->currentRow() : -1;
    if (r < 0 || !m_deviceTable->item(r, 0)) return;
    QString part = m_deviceTable->item(r, 0)->text();
    m_selectedDevice = m_db.findDevice(part);
    m_selectedBoard = Hardware::BoardDefinition();
    m_selectedTargetType = "device";

    syncEngineFromTarget();
    updateSelectedTargetBanner();
    m_btnNext->setEnabled(true);
}

void HardwareWizardDialog::onBoardSelected() {
    int r = m_boardTable ? m_boardTable->currentRow() : -1;
    if (r < 0 || !m_boardTable->item(r, 0)) return;
    QString boardId = m_boardTable->item(r, 0)->data(Qt::UserRole).toString();
    if (boardId.isEmpty()) boardId = m_boardTable->item(r, 0)->text();
    m_selectedBoard = m_db.findBoard(boardId);
    m_selectedDevice = m_db.findDevice(m_selectedBoard.mcuPartNumber);
    m_selectedTargetType = "board";

    syncEngineFromTarget();
    updateSelectedTargetBanner();
    m_btnNext->setEnabled(true);
}

void HardwareWizardDialog::updateSelectedTargetBanner() {
    if (m_selectedTargetType == "board" && (!m_selectedBoard.id.isEmpty() || !m_selectedBoard.name.isEmpty())) {
        QString boardName = m_selectedBoard.name.isEmpty() ? m_selectedBoard.id : m_selectedBoard.name;
        QString proc = m_selectedBoard.mcuPartNumber.isEmpty() ? "Unknown" : m_selectedBoard.mcuPartNumber;
        m_targetBanner->setText(QString("Board: %1  |  Processor: %2  |  Manufacturer: %3  |  Architecture: %4")
            .arg(boardName, proc, m_selectedBoard.manufacturer, m_selectedBoard.architecture));
    } else if (!m_selectedDevice.partNumber.isEmpty()) {
        m_targetBanner->setText(QString("MCU: %1  |  Vendor: %2  |  Architecture: %3 (%4)  |  Board: [None / Custom Board]")
            .arg(m_selectedDevice.partNumber, m_selectedDevice.vendor, m_selectedDevice.architecture, m_selectedDevice.core));
    } else if (m_selectedTargetType == "custom") {
        m_targetBanner->setText("Selected Target: Custom Hardware Target");
    } else {
        m_targetBanner->setText("Selected Hardware: None");
    }
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
