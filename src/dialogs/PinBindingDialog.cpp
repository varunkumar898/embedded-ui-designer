#include "PinBindingDialog.h"
#include "CanvasScene.h"
#include "UIComponent.h"
#include "ProgressBarComponent.h"
#include "LabelComponent.h"
#include "SliderComponent.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QMessageBox>
#include <QDateTime>
#include <QFileDialog>
#include "BoardConfigParser.h"

PinBindingDialog::PinBindingDialog(CanvasScene* scene, QWidget* parent)
    : QDialog(parent)
    , m_scene(scene)
    , m_bridge(HardwareBridge::instance())
{
    setWindowTitle("Hardware Pin Configuration & Live Binding (OpenOCD Path A)");
    resize(920, 720);

    setupUi();
    refreshTable();
    updatePotentiometerSection();

    connect(&m_bridge, &HardwareBridge::adcValueChanged, this, &PinBindingDialog::onHardwareBridgeUpdate);
    connect(&m_bridge, &HardwareBridge::pwmDutyChanged, this, &PinBindingDialog::onHardwareBridgeUpdate);
    connect(&m_bridge, &HardwareBridge::digitalStateChanged, this, &PinBindingDialog::onHardwareBridgeUpdate);
    connect(&m_bridge, &HardwareBridge::connectionStatusChanged, this, [this](bool connected, const QString& probeName) {
        if (connected) {
            QString name = probeName.isEmpty() ? "Hardware Target" : probeName;
            m_statusBadge->setText(QString("● OpenOCD: CONNECTED (%1)").arg(name));
            m_statusBadge->setStyleSheet("color: #4ade80; font-weight: bold; padding: 4px 8px; background: #064e3b; border: 1px solid #059669; border-radius: 4px;");
        } else {
            m_statusBadge->setText("○ OpenOCD: NOT FOUND (Simulated Test Mode)");
            m_statusBadge->setStyleSheet("color: #f59e0b; font-weight: bold; padding: 4px 8px; background: #451a03; border: 1px solid #d97706; border-radius: 4px;");
        }
    });
}

void PinBindingDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ────────────────────────────────────────────────────────────────────────
    // Header Bar: Board Selector & Probe Status
    // ────────────────────────────────────────────────────────────────────────
    auto* headerLayout = new QHBoxLayout();
    headerLayout->addWidget(new QLabel("Target Board Profile:", this));

    m_boardCombo = new QComboBox(this);
    m_boardCombo->setObjectName("boardProfileCombo");
    for (const auto& b : m_bridge.availableBoards()) {
        m_boardCombo->addItem(QString("%1 (%2)").arg(b.name, b.mcuFamily), b.id);
    }
    const int curIdx = m_boardCombo->findData(m_bridge.currentBoardId());
    if (curIdx >= 0) m_boardCombo->setCurrentIndex(curIdx);
    connect(m_boardCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PinBindingDialog::onBoardChanged);
    headerLayout->addWidget(m_boardCombo);

    QPushButton* importBtn = new QPushButton("Import Config (.ioc/sdkconfig)...", this);
    importBtn->setObjectName("importBoardConfigBtn");
    importBtn->setStyleSheet("QPushButton { background: #1e293b; color: #38bdf8; border: 1px solid #334155; border-radius: 4px; padding: 4px 8px; font-weight: bold; } QPushButton:hover { background: #334155; }");
    connect(importBtn, &QPushButton::clicked, this, &PinBindingDialog::onImportBoardConfigClicked);
    headerLayout->addWidget(importBtn);

    headerLayout->addSpacing(16);

    m_statusBadge = new QLabel(this);
    m_statusBadge->setObjectName("probeStatusBadge");
    if (m_bridge.isHardwareConnected()) {
        QString probe = m_bridge.connectedProbeName();
        m_statusBadge->setText(QString("● OpenOCD: CONNECTED (%1)").arg(probe.isEmpty() ? "Hardware Target" : probe));
        m_statusBadge->setStyleSheet("color: #4ade80; font-weight: bold; padding: 4px 8px; background: #064e3b; border: 1px solid #059669; border-radius: 4px;");
    } else {
        m_statusBadge->setText("○ OpenOCD: NOT FOUND (Simulated Test Mode)");
        m_statusBadge->setStyleSheet("color: #f59e0b; font-weight: bold; padding: 4px 8px; background: #451a03; border: 1px solid #d97706; border-radius: 4px;");
    }
    headerLayout->addWidget(m_statusBadge);
    headerLayout->addStretch();

    m_livePollButton = new QPushButton(m_bridge.isLivePolling() ? "Pause Polling" : "Resume Polling", this);
    m_livePollButton->setCheckable(true);
    m_livePollButton->setChecked(m_bridge.isLivePolling());
    connect(m_livePollButton, &QPushButton::clicked, this, [this](bool checked) {
        if (checked) {
            m_bridge.startLivePolling();
            m_livePollButton->setText("Pause Polling");
        } else {
            m_bridge.stopLivePolling();
            m_livePollButton->setText("Resume Polling");
        }
    });
    headerLayout->addWidget(m_livePollButton);

    mainLayout->addLayout(headerLayout);

    // ────────────────────────────────────────────────────────────────────────
    // Pin Configuration & Binding Table
    // ────────────────────────────────────────────────────────────────────────
    m_pinTable = new QTableWidget(this);
    m_pinTable->setObjectName("pinTable");
    m_pinTable->setColumnCount(5);
    m_pinTable->setHorizontalHeaderLabels({
        "Pin", "Pin Mode (Task A)", "Peripheral Mapping", "Canvas Component (Task C)", "Live Value"
    });
    m_pinTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_pinTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_pinTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_pinTable->verticalHeader()->setVisible(false);
    m_pinTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pinTable->setAlternatingRowColors(true);
    mainLayout->addWidget(m_pinTable, 1);

    // ────────────────────────────────────────────────────────────────────────
    // Bottom Controls: Split into ADC Potentiometer Simulator & SPI Panel
    // ────────────────────────────────────────────────────────────────────────
    auto* bottomSplitLayout = new QHBoxLayout();
    bottomSplitLayout->setSpacing(12);

    // 1. ADC Binding / Potentiometer Simulator (Task C)
    m_adcGroup = new QGroupBox("ADC Live Binding & Potentiometer Simulator (Raw Count)", this);
    m_adcGroup->setObjectName("adcSimulatorGroup");
    auto* adcLayout = new QVBoxLayout(m_adcGroup);

    auto* adcPinRow = new QHBoxLayout();
    adcPinRow->addWidget(new QLabel("ADC Pin:", m_adcGroup));
    m_adcPinSelect = new QComboBox(m_adcGroup);
    m_adcPinSelect->setObjectName("adcPinSelect");
    connect(m_adcPinSelect, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PinBindingDialog::updatePotentiometerSection);
    adcPinRow->addWidget(m_adcPinSelect);
    adcPinRow->addStretch();
    adcLayout->addLayout(adcPinRow);

    m_potentiometerSlider = new QSlider(Qt::Horizontal, m_adcGroup);
    m_potentiometerSlider->setObjectName("potentiometerSlider");
    m_potentiometerSlider->setRange(0, 4095); // 12-bit
    m_potentiometerSlider->setValue(2048);
    connect(m_potentiometerSlider, &QSlider::valueChanged,
            this, &PinBindingDialog::onPotentiometerSliderChanged);
    adcLayout->addWidget(m_potentiometerSlider);

    m_adcRawCountLabel = new QLabel("Raw ADC Count: 2048 / 4095 (12-bit raw count, uncalibrated)", m_adcGroup);
    m_adcRawCountLabel->setObjectName("adcRawCountLabel");
    m_adcRawCountLabel->setStyleSheet("font-family: monospace; font-size: 12px; color: #38bdf8;");
    adcLayout->addWidget(m_adcRawCountLabel);

    auto* adcNote = new QLabel("Note: Bound Progress Bar / Numeric Label displays raw ADC count (0 to 2^resolution-1), not calibrated engineering units.", m_adcGroup);
    adcNote->setStyleSheet("font-size: 11px; color: #94a3b8; font-style: italic;");
    adcNote->setWordWrap(true);
    adcLayout->addWidget(adcNote);

    bottomSplitLayout->addWidget(m_adcGroup, 1);

    // 2. SPI Raw Transfer (byte in/out) Panel (Task A & C)
    m_spiGroup = new QGroupBox("SPI Raw Transfer (byte in/out)", this);
    m_spiGroup->setObjectName("spiRawPanel");
    auto* spiLayout = new QVBoxLayout(m_spiGroup);

    auto* spiHeaderRow = new QHBoxLayout();
    spiHeaderRow->addWidget(new QLabel("SPI Bus:", m_spiGroup));
    m_spiBusCombo = new QComboBox(m_spiGroup);
    m_spiBusCombo->setObjectName("spiBusCombo");
    for (const QString& bus : m_bridge.currentBoard().spiBusses) {
        m_spiBusCombo->addItem(bus);
    }
    spiHeaderRow->addWidget(m_spiBusCombo, 1);
    spiLayout->addLayout(spiHeaderRow);

    auto* spiTransferRow = new QHBoxLayout();
    spiTransferRow->addWidget(new QLabel("Byte Out:", m_spiGroup));
    m_spiByteOutSpin = new QSpinBox(m_spiGroup);
    m_spiByteOutSpin->setObjectName("spiByteOutSpin");
    m_spiByteOutSpin->setDisplayIntegerBase(16);
    m_spiByteOutSpin->setPrefix("0x");
    m_spiByteOutSpin->setRange(0, 255);
    m_spiByteOutSpin->setValue(0x55);
    spiTransferRow->addWidget(m_spiByteOutSpin);

    m_spiTransferButton = new QPushButton("Send / Receive (Manual Trigger)", m_spiGroup);
    m_spiTransferButton->setObjectName("spiTransferButton");
    m_spiTransferButton->setStyleSheet("background: #0284c7; color: white; font-weight: 600; padding: 5px 12px; border-radius: 4px;");
    connect(m_spiTransferButton, &QPushButton::clicked, this, &PinBindingDialog::onSpiTransferClicked);
    spiTransferRow->addWidget(m_spiTransferButton);

    spiTransferRow->addSpacing(10);
    spiTransferRow->addWidget(new QLabel("Byte In:", m_spiGroup));
    m_spiByteInLabel = new QLabel("0x00 (0)", m_spiGroup);
    m_spiByteInLabel->setObjectName("spiByteInLabel");
    m_spiByteInLabel->setStyleSheet("font-family: monospace; font-weight: bold; color: #4ade80; background: #1e293b; padding: 4px 8px; border-radius: 4px;");
    spiTransferRow->addWidget(m_spiByteInLabel);
    spiTransferRow->addStretch();
    spiLayout->addLayout(spiTransferRow);

    m_spiLogEdit = new QTextEdit(m_spiGroup);
    m_spiLogEdit->setObjectName("spiLogEdit");
    m_spiLogEdit->setReadOnly(true);
    m_spiLogEdit->setMaximumHeight(70);
    m_spiLogEdit->setStyleSheet("font-family: monospace; font-size: 11px; background: #0f172a; color: #cbd5e1;");
    m_spiLogEdit->append("SPI Raw Transfer ready (manual byte in/out, not sensor-aware).");
    spiLayout->addWidget(m_spiLogEdit);

    bottomSplitLayout->addWidget(m_spiGroup, 1);
    mainLayout->addLayout(bottomSplitLayout);

    // Close button
    auto* bottomBtnLayout = new QHBoxLayout();
    bottomBtnLayout->addStretch();
    auto* closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomBtnLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomBtnLayout);
}

void PinBindingDialog::refreshTable() {
    const auto pins = m_bridge.availablePins();
    m_pinTable->setRowCount(pins.size());

    // Gather candidate canvas components
    QList<UIComponent*> candidateComps;
    if (m_scene) {
        candidateComps = m_scene->uiComponents();
    }

    m_adcPinSelect->blockSignals(true);
    m_adcPinSelect->clear();

    for (int r = 0; r < pins.size(); ++r) {
        const QString pinName = pins.at(r);
        const PinProfile prof = m_bridge.pinProfile(pinName);

        // Column 0: Pin Name
        auto* pinItem = new QTableWidgetItem(pinName);
        pinItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        pinItem->setFont(QFont("Monospace", 10, QFont::Bold));
        m_pinTable->setItem(r, 0, pinItem);

        // Column 1: Mode Combobox
        auto* modeCombo = new QComboBox(m_pinTable);
        const QList<PinMode> allModes = {
            PinMode::None,
            PinMode::DigitalIn,
            PinMode::DigitalOut,
            PinMode::PwmOutput,
            PinMode::AnalogIn,
            PinMode::SpiRawTransfer
        };

        for (PinMode m : allModes) {
            const bool available = m_bridge.isModeAvailable(pinName, m);
            QString label = pinModeToString(m);
            if (!available && m != PinMode::None) {
                // Honest caveat: unavailable on boards without this data, not silently broken
                label += " [Unavailable]";
            }
            modeCombo->addItem(label, static_cast<int>(m));
            if (!available && m != PinMode::None) {
                // Disable item in combobox model
                auto* model = qobject_cast<QStandardItemModel*>(modeCombo->model());
                if (model) {
                    auto* item = model->item(modeCombo->count() - 1);
                    if (item) item->setEnabled(false);
                }
            }
        }

        // Set active mode
        const int modeIdx = modeCombo->findData(static_cast<int>(prof.activeMode));
        if (modeIdx >= 0) modeCombo->setCurrentIndex(modeIdx);
        connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, r](int index) { onPinModeChanged(r, index); });
        m_pinTable->setCellWidget(r, 1, modeCombo);

        // Column 2: Peripheral Mapping
        QString mapDesc;
        if (prof.supportedModes.size() == 1 && prof.supportedModes.first() == PinMode::None) {
            mapDesc = "Unused / System / Unknown (Gated)";
        } else {
            QStringList parts;
            if (prof.supportedModes.contains(PinMode::DigitalIn) || prof.supportedModes.contains(PinMode::DigitalOut)) {
                parts.append("GPIO");
            }
            if (prof.adc.available) {
                parts.append(QString("%1_CH%2 (%3-bit)").arg(prof.adc.peripheral).arg(prof.adc.channel).arg(prof.adc.resolutionBits));
                m_adcPinSelect->addItem(QString("%1 (%2_CH%3)").arg(pinName, prof.adc.peripheral).arg(prof.adc.channel), pinName);
            }
            if (prof.pwm.available) {
                parts.append(QString("%1_CH%2 (AF%3)").arg(prof.pwm.timer).arg(prof.pwm.channel).arg(prof.pwm.alternateFunction));
            }
            if (prof.spi.available) {
                parts.append(QString("%1 (AF%2)").arg(prof.spi.peripheral).arg(prof.spi.alternateFunction));
            }
            mapDesc = parts.isEmpty() ? "Unassigned" : parts.join(" | ");
        }
        auto* mapItem = new QTableWidgetItem(mapDesc);
        mapItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_pinTable->setItem(r, 2, mapItem);

        // Column 3: Canvas Component Binding
        auto* compCombo = new QComboBox(m_pinTable);
        compCombo->addItem("(None)", "");
        UIComponent* boundComp = m_bridge.componentBoundToPin(pinName);

        for (UIComponent* comp : candidateComps) {
            if (!comp) continue;
            const QString compText = QString("%1 (%2)").arg(comp->componentId(), comp->componentType());
            compCombo->addItem(compText, comp->componentId());
            if (boundComp && boundComp->componentId() == comp->componentId()) {
                compCombo->setCurrentIndex(compCombo->count() - 1);
            }
        }
        connect(compCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, r](int index) { onComponentBindingChanged(r, index); });
        m_pinTable->setCellWidget(r, 3, compCombo);

        // Column 4: Live Value
        QString valStr = "-";
        if (prof.activeMode == PinMode::AnalogIn) {
            quint32 raw = 0;
            m_bridge.pollAdc(pinName, &raw);
            valStr = QString("Raw: %1 / %2").arg(raw).arg((1u << prof.adc.resolutionBits) - 1);
        } else if (prof.activeMode == PinMode::PwmOutput) {
            valStr = "Duty: 50%";
        } else if (prof.activeMode == PinMode::SpiRawTransfer) {
            valStr = "Ready (Manual)";
        } else if (prof.activeMode == PinMode::DigitalOut || prof.activeMode == PinMode::DigitalIn) {
            bool high = false;
            m_bridge.readDigitalIn(pinName, &high);
            valStr = high ? "HIGH (1)" : "LOW (0)";
        }
        auto* valItem = new QTableWidgetItem(valStr);
        valItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        valItem->setTextAlignment(Qt::AlignCenter);
        m_pinTable->setItem(r, 4, valItem);
    }

    m_adcPinSelect->blockSignals(false);
}

void PinBindingDialog::onBoardChanged(int index) {
    const QString boardId = m_boardCombo->itemData(index).toString();
    m_bridge.setBoard(boardId);

    // Refresh SPI bus combo
    m_spiBusCombo->clear();
    for (const QString& bus : m_bridge.currentBoard().spiBusses) {
        m_spiBusCombo->addItem(bus);
    }

    refreshTable();
    updatePotentiometerSection();
}

void PinBindingDialog::onPinModeChanged(int row, int index) {
    if (row < 0 || row >= m_pinTable->rowCount()) return;
    const QString pinName = m_pinTable->item(row, 0)->text();
    auto* combo = qobject_cast<QComboBox*>(m_pinTable->cellWidget(row, 1));
    if (!combo) return;

    const PinMode mode = static_cast<PinMode>(combo->itemData(index).toInt());
    if (!m_bridge.setPinMode(pinName, mode)) {
        // Revert to current mode
        const PinMode cur = m_bridge.activePinMode(pinName);
        const int curIdx = combo->findData(static_cast<int>(cur));
        combo->blockSignals(true);
        combo->setCurrentIndex(curIdx);
        combo->blockSignals(false);
        QMessageBox::warning(this, "Mode Unavailable",
            QString("Mode '%1' is not available on pin %2 for the selected board profile.")
                .arg(pinModeToString(mode), pinName));
    }

    // Refresh row live value
    onHardwareBridgeUpdate();
}

void PinBindingDialog::onComponentBindingChanged(int row, int index) {
    if (row < 0 || row >= m_pinTable->rowCount()) return;
    const QString pinName = m_pinTable->item(row, 0)->text();
    auto* compCombo = qobject_cast<QComboBox*>(m_pinTable->cellWidget(row, 3));
    if (!compCombo) return;

    const QString compId = compCombo->itemData(index).toString();
    if (compId.isEmpty()) {
        m_bridge.unbindPin(pinName);
    } else if (m_scene) {
        for (UIComponent* comp : m_scene->uiComponents()) {
            if (comp && comp->componentId() == compId) {
                PinMode mode = m_bridge.activePinMode(pinName);
                if (mode == PinMode::None) {
                    // Default to AnalogIn if ADC is available, else PWM, else Digital
                    if (m_bridge.isModeAvailable(pinName, PinMode::AnalogIn)) mode = PinMode::AnalogIn;
                    else if (m_bridge.isModeAvailable(pinName, PinMode::PwmOutput)) mode = PinMode::PwmOutput;
                    else mode = PinMode::DigitalOut;
                    m_bridge.setPinMode(pinName, mode);
                    auto* modeCombo = qobject_cast<QComboBox*>(m_pinTable->cellWidget(row, 1));
                    if (modeCombo) {
                        modeCombo->setCurrentIndex(modeCombo->findData(static_cast<int>(mode)));
                    }
                }
                m_bridge.bindComponent(comp, pinName, mode);
                break;
            }
        }
    }

    onHardwareBridgeUpdate();
}

void PinBindingDialog::updatePotentiometerSection() {
    const QString pinName = m_adcPinSelect->currentData().toString();
    if (pinName.isEmpty()) {
        m_potentiometerSlider->setEnabled(false);
        m_adcRawCountLabel->setText("No ADC pin selected");
        return;
    }

    m_potentiometerSlider->setEnabled(true);
    const PinProfile prof = m_bridge.pinProfile(pinName);
    const int maxVal = (1 << prof.adc.resolutionBits) - 1;
    m_potentiometerSlider->setMaximum(maxVal);

    quint32 currentVal = m_bridge.simulatedAdcCount(pinName);
    if (currentVal == 0) currentVal = maxVal / 2;
    m_potentiometerSlider->setValue(currentVal);

    m_adcRawCountLabel->setText(QString("Raw ADC Count: %1 / %2 (%3-bit raw count, uncalibrated)")
        .arg(currentVal).arg(maxVal).arg(prof.adc.resolutionBits));
}

void PinBindingDialog::onPotentiometerSliderChanged(int value) {
    const QString pinName = m_adcPinSelect->currentData().toString();
    if (pinName.isEmpty()) return;

    const PinProfile prof = m_bridge.pinProfile(pinName);
    const int maxVal = (1 << prof.adc.resolutionBits) - 1;

    m_bridge.setSimulatedAdcCount(pinName, static_cast<quint32>(value));
    m_adcRawCountLabel->setText(QString("Raw ADC Count: %1 / %2 (%3-bit raw count, uncalibrated)")
        .arg(value).arg(maxVal).arg(prof.adc.resolutionBits));

    onHardwareBridgeUpdate();
}

void PinBindingDialog::onSpiTransferClicked() {
    const QString bus = m_spiBusCombo->currentText();
    const quint8 byteOut = static_cast<quint8>(m_spiByteOutSpin->value());
    quint8 byteIn = 0;
    QString logMsg;

    bool ok = m_bridge.spiRawTransfer(bus, byteOut, &byteIn, &logMsg);
    if (ok) {
        m_spiByteInLabel->setText(QString("0x%1 (%2)")
            .arg(QString::number(byteIn, 16).toUpper().rightJustified(2, '0'))
            .arg(byteIn));
        m_spiLogEdit->append(QString("[%1] %2")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss.zzz"), logMsg));
    }
}

void PinBindingDialog::onHardwareBridgeUpdate() {
    for (int r = 0; r < m_pinTable->rowCount(); ++r) {
        const QString pinName = m_pinTable->item(r, 0)->text();
        const PinProfile prof = m_bridge.pinProfile(pinName);
        auto* valItem = m_pinTable->item(r, 4);
        if (!valItem) continue;

        if (prof.activeMode == PinMode::AnalogIn) {
            quint32 raw = m_bridge.simulatedAdcCount(pinName);
            valItem->setText(QString("Raw: %1 / %2").arg(raw).arg((1u << prof.adc.resolutionBits) - 1));
        } else if (prof.activeMode == PinMode::PwmOutput) {
            valItem->setText("Duty: Active");
        } else if (prof.activeMode == PinMode::SpiRawTransfer) {
            valItem->setText("Ready (Manual)");
        } else if (prof.activeMode == PinMode::DigitalOut || prof.activeMode == PinMode::DigitalIn) {
            bool high = false;
            m_bridge.readDigitalIn(pinName, &high);
            valItem->setText(high ? "HIGH (1)" : "LOW (0)");
        } else {
            valItem->setText("-");
        }
    }
}

void PinBindingDialog::onImportBoardConfigClicked() {
    QString filter = "Board Config Files (*.ioc sdkconfig* *.h *.ini *.txt);;STM32CubeMX (*.ioc);;ESP-IDF (sdkconfig*);;All Files (*)";
    QString filePath = QFileDialog::getOpenFileName(this, "Import Board Configuration (.ioc / sdkconfig)", QString(), filter);
    if (filePath.isEmpty()) return;

    auto res = BoardConfigParser::parseFile(filePath);
    if (!res.success) {
        QMessageBox::warning(this, "Board Configuration Import Failed", res.errorMessage);
        return;
    }

    BoardProfile bp = res.toBoardProfile();
    m_bridge.addBoard(bp);
    m_bridge.setBoard(bp.id);

    // Refresh m_boardCombo
    m_boardCombo->blockSignals(true);
    m_boardCombo->clear();
    for (const auto& b : m_bridge.availableBoards()) {
        m_boardCombo->addItem(QString("%1 (%2)").arg(b.name, b.mcuFamily), b.id);
    }
    const int curIdx = m_boardCombo->findData(bp.id);
    if (curIdx >= 0) m_boardCombo->setCurrentIndex(curIdx);
    m_boardCombo->blockSignals(false);

    refreshTable();
}
