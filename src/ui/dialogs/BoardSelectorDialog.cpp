#include "BoardSelectorDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>

BoardSelectorDialog::BoardSelectorDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Select Target Board & MCU Specifications");
    resize(640, 560);
    setupUi();
    populateBoards();
}

void BoardSelectorDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // Top selection row
    auto* topGroup = new QGroupBox("Target Board Selection", this);
    auto* topLayout = new QHBoxLayout(topGroup);

    m_boardCombo = new QComboBox(this);
    m_boardCombo->setMinimumWidth(320);
    topLayout->addWidget(m_boardCombo, 1);

    m_autoDetectBtn = new QPushButton("Auto-Detect", this);
    m_autoDetectBtn->setIcon(QIcon::fromTheme("system-search"));
    topLayout->addWidget(m_autoDetectBtn);

    mainLayout->addWidget(topGroup);

    // Detection status banner
    m_detectedStatusLabel = new QLabel("Click 'Auto-Detect' to scan connected USB debug adapters.", this);
    m_detectedStatusLabel->setStyleSheet("color: #888888; font-style: italic; padding: 4px;");
    mainLayout->addWidget(m_detectedStatusLabel);

    // Specs Display Group
    auto* specGroup = new QGroupBox("Hardware Specifications & Resources", this);
    auto* formLayout = new QFormLayout(specGroup);

    m_nameLabel = new QLabel("-", this);
    m_nameLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #3498db;");
    formLayout->addRow("Board Name:", m_nameLabel);

    m_mcuLabel = new QLabel("-", this);
    formLayout->addRow("MCU Model:", m_mcuLabel);

    m_archLabel = new QLabel("-", this);
    formLayout->addRow("Architecture:", m_archLabel);

    m_freqLabel = new QLabel("-", this);
    formLayout->addRow("Clock Speed:", m_freqLabel);

    m_flashLabel = new QLabel("-", this);
    formLayout->addRow("Flash Memory:", m_flashLabel);

    m_ramLabel = new QLabel("-", this);
    formLayout->addRow("SRAM Memory:", m_ramLabel);

    m_gpioLabel = new QLabel("-", this);
    formLayout->addRow("GPIO Pins:", m_gpioLabel);

    m_adapterLabel = new QLabel("-", this);
    formLayout->addRow("Debug Adapter:", m_adapterLabel);

    m_openocdConfigLabel = new QLabel("-", this);
    m_openocdConfigLabel->setStyleSheet("font-family: monospace; color: #2ecc71;");
    formLayout->addRow("OpenOCD Config:", m_openocdConfigLabel);

    m_toolchainLabel = new QLabel("-", this);
    formLayout->addRow("Compiler Toolchain:", m_toolchainLabel);

    mainLayout->addWidget(specGroup);

    // Pinout & Capabilities summary
    auto* pinsGroup = new QGroupBox("Available Hardware Pins & Channels", this);
    auto* pinsLayout = new QVBoxLayout(pinsGroup);
    m_pinsBrowser = new QTextBrowser(this);
    m_pinsBrowser->setMaximumHeight(100);
    pinsLayout->addWidget(m_pinsBrowser);
    mainLayout->addWidget(pinsGroup);

    // Action buttons
    auto* actionLayout = new QHBoxLayout();
    m_datasheetBtn = new QPushButton("View Datasheet", this);
    m_datasheetBtn->setIcon(QIcon::fromTheme("help-browser"));
    actionLayout->addWidget(m_datasheetBtn);

    actionLayout->addStretch(1);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    actionLayout->addWidget(buttonBox);
    mainLayout->addLayout(actionLayout);

    // Connect signals
    connect(m_boardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BoardSelectorDialog::onBoardIndexChanged);
    connect(m_autoDetectBtn, &QPushButton::clicked, this, &BoardSelectorDialog::onAutoDetectClicked);
    connect(m_datasheetBtn, &QPushButton::clicked, this, &BoardSelectorDialog::onViewDatasheet);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void BoardSelectorDialog::populateBoards() {
    m_boardCombo->blockSignals(true);
    m_boardCombo->clear();

    const auto boards = BoardDatabase::instance().allBoards();
    for (const auto& b : boards) {
        m_boardCombo->addItem(QString("%1 (%2)").arg(b.name, b.mcu), b.id);
    }
    m_boardCombo->blockSignals(false);

    if (m_boardCombo->count() > 0) {
        m_boardCombo->setCurrentIndex(0);
        onBoardIndexChanged(0);
    }
}

void BoardSelectorDialog::selectBoardById(const QString& id) {
    int idx = m_boardCombo->findData(id);
    if (idx >= 0) {
        m_boardCombo->setCurrentIndex(idx);
    }
}

void BoardSelectorDialog::onBoardIndexChanged(int index) {
    if (index < 0) return;
    QString boardId = m_boardCombo->itemData(index).toString();
    m_selectedBoard = BoardDatabase::instance().findBoardById(boardId);
    updateSpecDisplay(m_selectedBoard);
    emit boardSelected(m_selectedBoard);
}

void BoardSelectorDialog::updateSpecDisplay(const BoardSpec& spec) {
    if (!spec.isValid()) return;

    m_nameLabel->setText(spec.name);
    m_mcuLabel->setText(QString("%1 (%2)").arg(spec.mcu, spec.manufacturer));
    m_archLabel->setText(spec.architecture);
    m_freqLabel->setText(QString("%1 MHz").arg(spec.frequencyMhz));
    m_flashLabel->setText(QString("%1 KB").arg(spec.flashKb));
    m_ramLabel->setText(QString("%1 KB").arg(spec.ramKb));
    m_gpioLabel->setText(QString("%1 total pins").arg(spec.gpioCount));
    m_adapterLabel->setText(QString("%1 [VID:%2 PID:%3]").arg(spec.usbAdapterType, spec.vid, spec.pid));
    m_openocdConfigLabel->setText(spec.openocdConfigFile);
    m_toolchainLabel->setText(QString("%1 (%2)").arg(spec.compiler, spec.halHeader));

    QString pinHtml = QString("<b>Digital Pins:</b> %1<br>"
                              "<b>Analog ADC Pins:</b> %2<br>"
                              "<b>PWM Pins:</b> %3")
                          .arg(spec.digitalPins.join(", "),
                               spec.analogPins.join(", "),
                               spec.pwmPins.join(", "));
    m_pinsBrowser->setHtml(pinHtml);

    m_datasheetBtn->setEnabled(!spec.datasheetUrl.isEmpty());
}

void BoardSelectorDialog::onViewDatasheet() {
    if (m_selectedBoard.isValid() && !m_selectedBoard.datasheetUrl.isEmpty()) {
        QDesktopServices::openUrl(QUrl(m_selectedBoard.datasheetUrl));
    }
}

void BoardSelectorDialog::onAutoDetectClicked() {
    DetectedBoardInfo detected = m_detector.detectConnectedBoard();
    if (detected.detected) {
        m_detectedStatusLabel->setText(QString("Auto-detected: %1 (%2)").arg(detected.boardName, detected.adapterType));
        m_detectedStatusLabel->setStyleSheet("color: #2ecc71; font-weight: bold; padding: 4px;");
        selectBoardById(detected.boardId);
    } else {
        m_detectedStatusLabel->setText("No USB debug probe detected. Please select board manually from the list.");
        m_detectedStatusLabel->setStyleSheet("color: #e67e22; font-style: italic; padding: 4px;");
    }
}
