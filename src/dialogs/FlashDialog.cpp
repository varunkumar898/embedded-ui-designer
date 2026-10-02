#include "FlashDialog.h"

#include "DeviceManager.h"

#include <QComboBox>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QApplication>

FlashDialog::FlashDialog(DeviceManager* deviceManager, QWidget* parent)
    : QDialog(parent)
    , m_deviceManager(deviceManager)
    , m_process(new QProcess(this))
{
    setWindowTitle("Flash Firmware");
    setMinimumSize(650, 520);

    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    m_boardCombo = new QComboBox(this);
    m_boardCombo->addItem("STM32", "stm32");
    m_boardCombo->addItem("ESP32", "esp32");
    m_boardCombo->addItem("RP2040", "rp2040");

    auto* portRow = new QWidget(this);
    auto* portLayout = new QHBoxLayout(portRow);
    portLayout->setContentsMargins(0, 0, 0, 0);
    m_portCombo = new QComboBox(portRow);
    auto* refreshButton = new QPushButton("Refresh", portRow);
    portLayout->addWidget(m_portCombo, 1);
    portLayout->addWidget(refreshButton);

    auto* firmwareRow = new QWidget(this);
    auto* firmwareLayout = new QHBoxLayout(firmwareRow);
    firmwareLayout->setContentsMargins(0, 0, 0, 0);
    m_firmwareEdit = new QLineEdit("firmware.bin", firmwareRow);
    auto* browseButton = new QPushButton("Browse...", firmwareRow);
    firmwareLayout->addWidget(m_firmwareEdit, 1);
    firmwareLayout->addWidget(browseButton);

    form->addRow("Board", m_boardCombo);
    form->addRow("Serial port", portRow);
    form->addRow("Firmware", firmwareRow);
    root->addLayout(form);

    root->addWidget(new QLabel("Generated command", this));
    m_commandEdit = new QPlainTextEdit(this);
    m_commandEdit->setReadOnly(true);
    m_commandEdit->document()->setMaximumBlockCount(3);
    m_commandEdit->setFixedHeight(76);
    root->addWidget(m_commandEdit);

    auto* actionRow = new QHBoxLayout();
    actionRow->addStretch(1);
    m_copyButton = new QPushButton("Copy command", this);
    m_runButton = new QPushButton("Run command", this);
    actionRow->addWidget(m_copyButton);
    actionRow->addWidget(m_runButton);
    root->addLayout(actionRow);

    root->addWidget(new QLabel("Output", this));
    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    root->addWidget(m_logEdit, 1);

    auto* closeButton = new QPushButton("Close", this);
    auto* closeRow = new QHBoxLayout();
    closeRow->addStretch(1);
    closeRow->addWidget(closeButton);
    root->addLayout(closeRow);

    connect(refreshButton, &QPushButton::clicked, this, &FlashDialog::refreshPorts);
    connect(browseButton, &QPushButton::clicked, this, &FlashDialog::chooseFirmware);
    connect(m_boardCombo, &QComboBox::currentIndexChanged, this, &FlashDialog::updateCommand);
    connect(m_portCombo, &QComboBox::currentIndexChanged, this, [this]() {
        selectDetectedProfile();
        updateCommand();
    });
    connect(m_firmwareEdit, &QLineEdit::textChanged, this, &FlashDialog::updateCommand);
    connect(m_copyButton, &QPushButton::clicked, this, &FlashDialog::copyCommand);
    connect(m_runButton, &QPushButton::clicked, this, &FlashDialog::runCommand);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &FlashDialog::appendProcessOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &FlashDialog::appendProcessOutput);
    connect(m_process, &QProcess::finished, this, &FlashDialog::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &FlashDialog::onProcessError);

    if (m_deviceManager) {
        connect(m_deviceManager, &DeviceManager::portsChanged, this, &FlashDialog::reloadPortList);
    }
    reloadPortList();
    appendLog("Commands are generated for review; flashing never starts automatically.");
}

QString FlashDialog::commandForProfile(const QString& profile, const QString& port,
                                       const QString& firmwarePath)
{
    if (profile == "stm32") {
        return QString("STM32_Programmer_CLI -c port=%1 -w %2 -v").arg(port, firmwarePath);
    }
    if (profile == "esp32") {
        return QString("esptool.py --port %1 write_flash 0x0 %2").arg(port, firmwarePath);
    }
    if (profile == "rp2040") {
        return "UF2: copy the .uf2 firmware file to the RP2040 USB drive.";
    }
    return {};
}

void FlashDialog::refreshPorts() {
    if (m_deviceManager) {
        m_deviceManager->refreshPorts();
    }
    reloadPortList();
}

void FlashDialog::reloadPortList() {
    const QString previousPort = m_portCombo->currentData().toString();
    const QSignalBlocker blocker(m_portCombo);
    m_portCombo->clear();

    const QStringList ports = m_deviceManager ? m_deviceManager->portNames() : QStringList();
    if (ports.isEmpty()) {
        m_portCombo->addItem("No serial ports detected", QString());
    } else {
        for (const QString& port : ports) {
            const QVariantMap details = m_deviceManager->getPortDetails(port);
            m_portCombo->addItem(port, port);
            m_portCombo->setItemData(m_portCombo->count() - 1,
                                     details.value("description").toString(), Qt::ToolTipRole);
        }
        const int previousIndex = m_portCombo->findData(previousPort);
        if (previousIndex >= 0) {
            m_portCombo->setCurrentIndex(previousIndex);
        }
    }
    selectDetectedProfile();
    updateCommand();
}

void FlashDialog::selectDetectedProfile() {
    if (!m_deviceManager || m_portCombo->currentData().toString().isEmpty()) {
        return;
    }

    const QString detected = m_deviceManager->getPortDetails(
        m_portCombo->currentData().toString()).value("detectedBoard").toString().toLower();
    QString profile;
    if (detected.contains("esp32")) {
        profile = "esp32";
    } else if (detected.contains("stm32")) {
        profile = "stm32";
    } else if (detected.contains("rp2040") || detected.contains("pico")) {
        profile = "rp2040";
    }
    if (!profile.isEmpty()) {
        const int index = m_boardCombo->findData(profile);
        if (index >= 0) {
            const QSignalBlocker blocker(m_boardCombo);
            m_boardCombo->setCurrentIndex(index);
        }
    }
}

void FlashDialog::updateCommand() {
    const QString profile = m_boardCombo->currentData().toString();
    const QString port = m_portCombo->currentData().toString();
    const QString firmware = m_firmwareEdit->text().trimmed();
    const QString command = commandForProfile(profile, port, firmware);
    m_commandEdit->setPlainText(command.isEmpty() ? "Select a supported board profile." : command);

    const bool supportedCommand = profile == "stm32" || profile == "esp32";
    m_copyButton->setEnabled(supportedCommand && !port.isEmpty());
    m_runButton->setEnabled(supportedCommand && !port.isEmpty()
                            && QFileInfo::exists(firmware)
                            && m_process->state() == QProcess::NotRunning);
}

void FlashDialog::chooseFirmware() {
    const bool rp2040 = m_boardCombo->currentData().toString() == "rp2040";
    const QString filter = rp2040 ? "UF2 firmware (*.uf2);;All files (*)"
                                  : "Firmware binary (*.bin);;All files (*)";
    const QString file = QFileDialog::getOpenFileName(this, "Select Firmware", QString(), filter);
    if (!file.isEmpty()) {
        m_firmwareEdit->setText(file);
    }
}

void FlashDialog::copyCommand() {
    const QString profile = m_boardCombo->currentData().toString();
    QApplication::clipboard()->setText(commandForProfile(
        profile, m_portCombo->currentData().toString(), m_firmwareEdit->text().trimmed()));
}

void FlashDialog::runCommand() {
    const QString profile = m_boardCombo->currentData().toString();
    const QString port = m_portCombo->currentData().toString();
    const QString firmware = m_firmwareEdit->text().trimmed();
    if (!QFileInfo(firmware).isFile() || port.isEmpty()) {
        appendLog("Select a detected port and an existing firmware file before running.");
        return;
    }

    QString program;
    QStringList arguments;
    if (profile == "stm32") {
        program = "STM32_Programmer_CLI";
        arguments << "-c" << QString("port=%1").arg(port) << "-w" << firmware << "-v";
    } else if (profile == "esp32") {
        program = "esptool.py";
        arguments << "--port" << port << "write_flash" << "0x0" << firmware;
    } else {
        return;
    }

    appendLog(QString("$ %1").arg(commandForProfile(profile, port, firmware)));
    m_runButton->setEnabled(false);
    m_process->start(program, arguments);
}

void FlashDialog::appendProcessOutput() {
    appendLog(QString::fromLocal8Bit(m_process->readAllStandardOutput()));
    appendLog(QString::fromLocal8Bit(m_process->readAllStandardError()));
}

void FlashDialog::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    appendProcessOutput();
    appendLog(status == QProcess::NormalExit
                  ? QString("Process finished with exit code %1.").arg(exitCode)
                  : "Process terminated unexpectedly.");
    updateCommand();
}

void FlashDialog::onProcessError(QProcess::ProcessError error) {
    Q_UNUSED(error);
    appendLog(QString("Could not start command: %1").arg(m_process->errorString()));
    updateCommand();
}

void FlashDialog::appendLog(const QString& text) {
    if (!text.isEmpty()) {
        m_logEdit->appendPlainText(text);
    }
}