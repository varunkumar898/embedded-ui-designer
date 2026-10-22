#include "CustomPackImportDialog.h"
#include "driverpack/CmsisSvdImporter.h"
#include "driverpack/PinTableParser.h"
#include "driverpack/SdkReferenceChecker.h"
#include "driverpack/CustomPackManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QSet>

CustomPackImportDialog::CustomPackImportDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Import Custom Board / MCU Pack");
    setMinimumSize(680, 720);
    setupUi();
}

void CustomPackImportDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // ── 1. Device Identity ───────────────────────────────────────────────────
    auto* idGroup = new QGroupBox("1. Device Identity & Target", this);
    auto* idGrid = new QGridLayout(idGroup);

    idGrid->addWidget(new QLabel("Device Name:", idGroup), 0, 0);
    m_nameEdit = new QLineEdit(idGroup);
    m_nameEdit->setPlaceholderText("e.g. STM32G071RB");
    idGrid->addWidget(m_nameEdit, 0, 1);

    idGrid->addWidget(new QLabel("Vendor:", idGroup), 0, 2);
    m_vendorEdit = new QLineEdit(idGroup);
    m_vendorEdit->setPlaceholderText("e.g. STMicroelectronics");
    idGrid->addWidget(m_vendorEdit, 0, 3);

    idGrid->addWidget(new QLabel("Part Number:", idGroup), 1, 0);
    m_partNumberEdit = new QLineEdit(idGroup);
    m_partNumberEdit->setPlaceholderText("e.g. STM32G071RBT6");
    idGrid->addWidget(m_partNumberEdit, 1, 1);

    idGrid->addWidget(new QLabel("Family:", idGroup), 1, 2);
    m_familyEdit = new QLineEdit(idGroup);
    m_familyEdit->setPlaceholderText("e.g. STM32G0");
    idGrid->addWidget(m_familyEdit, 1, 3);

    idGrid->addWidget(new QLabel("CPU Core:", idGroup), 2, 0);
    m_coreEdit = new QLineEdit(idGroup);
    m_coreEdit->setPlaceholderText("e.g. Cortex-M0+");
    idGrid->addWidget(m_coreEdit, 2, 1);

    idGrid->addWidget(new QLabel("Clock Speed (MHz):", idGroup), 2, 2);
    m_clockSpin = new QSpinBox(idGroup);
    m_clockSpin->setRange(1, 2000);
    m_clockSpin->setValue(64);
    idGrid->addWidget(m_clockSpin, 2, 3);

    mainLayout->addWidget(idGroup);

    // ── 2. CMSIS-SVD Importer ────────────────────────────────────────────────
    auto* svdGroup = new QGroupBox("2. CMSIS-SVD Peripheral & Register Import", this);
    auto* svdLayout = new QVBoxLayout(svdGroup);

    auto* svdRow = new QHBoxLayout();
    m_svdPathEdit = new QLineEdit(svdGroup);
    m_svdPathEdit->setPlaceholderText("Select vendor .svd file (XML entity expansion disabled, 10MB limit)");
    auto* svdBrowseBtn = new QPushButton("Browse SVD...", svdGroup);
    connect(svdBrowseBtn, &QPushButton::clicked, this, &CustomPackImportDialog::onBrowseSvd);
    svdRow->addWidget(m_svdPathEdit);
    svdRow->addWidget(svdBrowseBtn);
    svdLayout->addLayout(svdRow);

    m_svdStatusLabel = new QLabel("No SVD loaded yet.", svdGroup);
    m_svdStatusLabel->setStyleSheet("color: #888;");
    svdLayout->addWidget(m_svdStatusLabel);

    mainLayout->addWidget(svdGroup);

    // ── 3. Pin Table (CSV & Editor) ──────────────────────────────────────────
    auto* pinGroup = new QGroupBox("3. Pin Table Configuration", this);
    auto* pinLayout = new QVBoxLayout(pinGroup);

    auto* pinBtnRow = new QHBoxLayout();
    auto* importCsvBtn = new QPushButton("Import Pin CSV...", pinGroup);
    connect(importCsvBtn, &QPushButton::clicked, this, &CustomPackImportDialog::onImportCsvPins);
    auto* addPinBtn = new QPushButton("Add Pin", pinGroup);
    connect(addPinBtn, &QPushButton::clicked, this, [this]() {
        int r = m_pinTable->rowCount();
        m_pinTable->insertRow(r);
        m_pinTable->setItem(r, 0, new QTableWidgetItem("PA0"));
        m_pinTable->setItem(r, 1, new QTableWidgetItem("GPIO_Output"));
        m_pinTable->setItem(r, 2, new QTableWidgetItem("GPIOA"));
        validateAll();
    });
    auto* removePinBtn = new QPushButton("Remove Selected", pinGroup);
    connect(removePinBtn, &QPushButton::clicked, this, [this]() {
        int r = m_pinTable->currentRow();
        if (r >= 0) m_pinTable->removeRow(r);
        validateAll();
    });

    pinBtnRow->addWidget(importCsvBtn);
    pinBtnRow->addWidget(addPinBtn);
    pinBtnRow->addWidget(removePinBtn);
    pinBtnRow->addStretch();
    pinLayout->addLayout(pinBtnRow);

    m_pinTable = new QTableWidget(0, 3, pinGroup);
    m_pinTable->setHorizontalHeaderLabels({"Pin", "Function", "Peripheral"});
    m_pinTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_pinTable->setMinimumHeight(120);
    connect(m_pinTable, &QTableWidget::cellChanged, this, [this]() { validateAll(); });
    pinLayout->addWidget(m_pinTable);

    m_pinStatusLabel = new QLabel("Pins must map to peripherals present in the SVD.", pinGroup);
    m_pinStatusLabel->setStyleSheet("color: #888;");
    pinLayout->addWidget(m_pinStatusLabel);

    mainLayout->addWidget(pinGroup);

    // ── 4. SDK Reference ─────────────────────────────────────────────────────
    auto* sdkGroup = new QGroupBox("4. Local Vendor SDK Reference (Optional)", this);
    auto* sdkLayout = new QVBoxLayout(sdkGroup);

    auto* sdkRow = new QHBoxLayout();
    m_sdkPathEdit = new QLineEdit(sdkGroup);
    m_sdkPathEdit->setPlaceholderText("Path to local vendor SDK folder (code is referenced, not copied)");
    auto* sdkBrowseBtn = new QPushButton("Browse...", sdkGroup);
    connect(sdkBrowseBtn, &QPushButton::clicked, this, &CustomPackImportDialog::onBrowseSdk);
    auto* sdkVerifyBtn = new QPushButton("Verify SDK", sdkGroup);
    connect(sdkVerifyBtn, &QPushButton::clicked, this, &CustomPackImportDialog::onVerifySdk);
    sdkRow->addWidget(m_sdkPathEdit);
    sdkRow->addWidget(sdkBrowseBtn);
    sdkRow->addWidget(sdkVerifyBtn);
    sdkLayout->addLayout(sdkRow);

    m_sdkStatusLabel = new QLabel("Optional: vendor SDK headers will be referenced during build if provided.", sdkGroup);
    m_sdkStatusLabel->setStyleSheet("color: #888;");
    sdkLayout->addWidget(m_sdkStatusLabel);

    mainLayout->addWidget(sdkGroup);

    // ── Bottom Buttons ───────────────────────────────────────────────────────
    auto* bottomRow = new QHBoxLayout();
    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    bottomRow->addWidget(m_statusLabel, 1);

    auto* cancelBtn = new QPushButton("Cancel", this);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    m_installButton = new QPushButton("Install Custom Pack", this);
    m_installButton->setStyleSheet("font-weight: bold; background-color: #2e7d32; color: white; padding: 6px 16px;");
    connect(m_installButton, &QPushButton::clicked, this, &CustomPackImportDialog::onInstallPack);

    bottomRow->addWidget(cancelBtn);
    bottomRow->addWidget(m_installButton);
    mainLayout->addLayout(bottomRow);

    validateAll();
}

void CustomPackImportDialog::onBrowseSvd() {
    QString path = QFileDialog::getOpenFileName(
        this, "Select CMSIS-SVD XML File", QString(),
        "CMSIS-SVD Files (*.svd *.xml);;All Files (*)");
    if (path.isEmpty()) return;

    m_svdPathEdit->setText(path);

    driverpack::SvdImportResult res = driverpack::CmsisSvdImporter::importFromFile(path);
    if (!res.success) {
        m_svdStatusLabel->setStyleSheet("color: #c62828; font-weight: bold;");
        m_svdStatusLabel->setText(QString("SVD Import Failed: %1").arg(res.errorMessage));
        return;
    }

    m_pack.peripherals = res.peripherals;
    if (m_pack.provenance.sourceFiles.indexOf(path) < 0) {
        m_pack.provenance.sourceFiles.append(path);
    }

    if (!res.deviceName.isEmpty() && m_nameEdit->text().isEmpty()) {
        m_nameEdit->setText(res.deviceName);
        m_partNumberEdit->setText(res.deviceName);
    }
    if (!res.vendor.isEmpty() && m_vendorEdit->text().isEmpty()) {
        m_vendorEdit->setText(res.vendor);
    }
    if (!res.cpuName.isEmpty() && m_coreEdit->text().isEmpty()) {
        m_coreEdit->setText(res.cpuName);
    }
    if (res.clockFrequencyHz > 0) {
        m_clockSpin->setValue(static_cast<int>(res.clockFrequencyHz / 1000000));
    }

    int totalRegs = 0;
    for (const auto& p : res.peripherals) totalRegs += p.registers.size();

    m_svdStatusLabel->setStyleSheet("color: #2e7d32; font-weight: bold;");
    m_svdStatusLabel->setText(QString("✓ Successfully parsed SVD: %1 peripherals, %2 registers.")
                                  .arg(res.peripherals.size())
                                  .arg(totalRegs));

    validateAll();
}

void CustomPackImportDialog::onImportCsvPins() {
    QString path = QFileDialog::getOpenFileName(
        this, "Select Pin Table CSV", QString(),
        "CSV Files (*.csv);;Text Files (*.txt);;All Files (*)");
    if (path.isEmpty()) return;

    QSet<QString> allowedPeripherals;
    for (const auto& p : m_pack.peripherals) allowedPeripherals.insert(p.name);

    driverpack::PinTableParseResult res = driverpack::PinTableParser::parseCsvFile(path, allowedPeripherals);

    if (!res.errors.isEmpty()) {
        m_pinStatusLabel->setStyleSheet("color: #c62828; font-weight: bold;");
        m_pinStatusLabel->setText(QString("Pin CSV errors: %1").arg(res.errors.join("; ")));
        QMessageBox::warning(this, "Pin Table Validation Error", res.errors.join("\n"));
    }

    if (!res.pins.isEmpty()) {
        m_pinTable->setRowCount(0);
        for (const auto& pin : res.pins) {
            int r = m_pinTable->rowCount();
            m_pinTable->insertRow(r);
            m_pinTable->setItem(r, 0, new QTableWidgetItem(pin.pin));
            m_pinTable->setItem(r, 1, new QTableWidgetItem(pin.function));
            m_pinTable->setItem(r, 2, new QTableWidgetItem(pin.peripheral));
        }

        if (res.errors.isEmpty()) {
            m_pinStatusLabel->setStyleSheet("color: #2e7d32; font-weight: bold;");
            m_pinStatusLabel->setText(QString("✓ Loaded %1 valid pins from CSV.").arg(res.pins.size()));
        }
    }

    validateAll();
}

void CustomPackImportDialog::onBrowseSdk() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Local Vendor SDK Folder");
    if (!dir.isEmpty()) {
        m_sdkPathEdit->setText(dir);
        onVerifySdk();
    }
}

void CustomPackImportDialog::onVerifySdk() {
    const QString path = m_sdkPathEdit->text().trimmed();
    if (path.isEmpty()) {
        m_sdkStatusLabel->setStyleSheet("color: #888;");
        m_sdkStatusLabel->setText("No SDK path specified (optional).");
        return;
    }

    driverpack::SdkCheckResult res = driverpack::SdkReferenceChecker::verifySdkDirectory(path);
    if (res.valid) {
        m_sdkStatusLabel->setStyleSheet("color: #2e7d32; font-weight: bold;");
        m_sdkStatusLabel->setText(QString("✓ SDK Verified: %1").arg(res.message));
        m_pack.sdkReference.path = path;
        m_pack.sdkReference.detected = true;
    } else {
        m_sdkStatusLabel->setStyleSheet("color: #e65100;");
        m_sdkStatusLabel->setText(QString("SDK Warning: %1").arg(res.message));
        m_pack.sdkReference.detected = false;
    }
}

void CustomPackImportDialog::validateAll() {
    // Collect pin table rows
    m_pack.pinTable.clear();
    QSet<QString> periphNames;
    for (const auto& p : m_pack.peripherals) periphNames.insert(p.name);

    bool pinError = false;
    for (int r = 0; r < m_pinTable->rowCount(); ++r) {
        auto* pItem = m_pinTable->item(r, 0);
        auto* fItem = m_pinTable->item(r, 1);
        auto* mItem = m_pinTable->item(r, 2);
        if (pItem && fItem && mItem) {
            driverpack::CustomPinDef pin;
            pin.pin = pItem->text().trimmed();
            pin.function = fItem->text().trimmed();
            pin.peripheral = mItem->text().trimmed();
            if (!periphNames.isEmpty() && !periphNames.contains(pin.peripheral)) {
                pinError = true;
                mItem->setBackground(QColor("#ffebee"));
                mItem->setToolTip(QString("Unknown peripheral '%1'").arg(pin.peripheral));
            } else {
                mItem->setBackground(QBrush());
                mItem->setToolTip(QString());
            }
            m_pack.pinTable.append(pin);
        }
    }

    if (pinError) {
        m_pinStatusLabel->setStyleSheet("color: #c62828; font-weight: bold;");
        m_pinStatusLabel->setText("Error: Some pins reference unknown peripherals (highlighted in red).");
    }

    // Default flash & sram regions if none explicitly defined
    if (m_pack.memoryRegions.isEmpty()) {
        driverpack::CustomMemoryRegion flash;
        flash.name = "FLASH";
        flash.type = "flash";
        flash.startAddress = "0x08000000";
        flash.sizeKb = 512;
        m_pack.memoryRegions.append(flash);

        driverpack::CustomMemoryRegion sram;
        sram.name = "SRAM";
        sram.type = "sram";
        sram.startAddress = "0x20000000";
        sram.sizeKb = 128;
        sram.usableForFramebuffer = 1;
        m_pack.memoryRegions.append(sram);
    }
}

void CustomPackImportDialog::onInstallPack() {
    validateAll();

    m_pack.name = m_nameEdit->text().trimmed();
    m_pack.vendor = m_vendorEdit->text().trimmed();
    m_pack.partNumber = m_partNumberEdit->text().trimmed();
    m_pack.family = m_familyEdit->text().trimmed();
    m_pack.core = m_coreEdit->text().trimmed();
    m_pack.clockSpeedMhz = static_cast<uint32_t>(m_clockSpin->value());

    // Provenance
    m_pack.provenance.importerVersion = "1.0.0";
    m_pack.provenance.generatedBy = "importer";
    m_pack.provenance.verified = false; // NEVER verified by default for imported packs

    QStringList errors;
    if (!m_pack.isValid(&errors)) {
        QMessageBox::critical(this, "Validation Failed", "Cannot install custom pack:\n\n" + errors.join("\n"));
        return;
    }

    QString installErr;
    if (!driverpack::CustomPackManager::instance().installPack(m_pack, nullptr, &installErr)) {
        QMessageBox::critical(this, "Installation Failed", QString("Failed to save pack: %1").arg(installErr));
        return;
    }

    QMessageBox::information(this, "Pack Installed",
        QString("Custom pack for '%1' has been installed into your user data directory.\n\n"
                "It is now available in the Board & Display target selector.")
            .arg(m_pack.name));

    accept();
}
