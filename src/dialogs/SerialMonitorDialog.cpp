#include "SerialMonitorDialog.h"

#include "DeviceManager.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTextCursor>
#include <QVBoxLayout>
#ifdef HAVE_QT_SERIALPORT
#include <QSerialPort>
#endif

SerialMonitorDialog::SerialMonitorDialog(DeviceManager* deviceManager, QWidget* parent)
    : QDialog(parent)
    , m_deviceManager(deviceManager)
{
    setWindowTitle("Serial Device Monitor");
    resize(720, 520);

    auto* root = new QVBoxLayout(this);
    auto* connectionRow = new QHBoxLayout();
    m_portPicker = new QComboBox(this);
    m_portPicker->setObjectName("serialPortPicker");
    m_portPicker->setMinimumWidth(170);
    m_baudRate = new QSpinBox(this);
    m_baudRate->setObjectName("serialBaudRate");
    m_baudRate->setRange(300, 3000000);
    m_baudRate->setValue(115200);
    m_connectButton = new QPushButton("Connect", this);
    m_disconnectButton = new QPushButton("Disconnect", this);
    auto* refreshButton = new QPushButton("Refresh", this);
    connectionRow->addWidget(new QLabel("Port", this));
    connectionRow->addWidget(m_portPicker, 1);
    connectionRow->addWidget(refreshButton);
    connectionRow->addWidget(new QLabel("Baud", this));
    connectionRow->addWidget(m_baudRate);
    connectionRow->addWidget(m_connectButton);
    connectionRow->addWidget(m_disconnectButton);
    root->addLayout(connectionRow);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(10000);
    m_log->setStyleSheet("QPlainTextEdit { background: #101216; color: #c8d2df; border: 1px solid #303642; border-radius: 4px; font-family: monospace; }");
    root->addWidget(m_log, 1);

    auto* sendRow = new QHBoxLayout();
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Send text to the connected device");
    m_sendButton = new QPushButton("Send", this);
    sendRow->addWidget(m_input, 1);
    sendRow->addWidget(m_sendButton);
    root->addLayout(sendRow);

    connect(refreshButton, &QPushButton::clicked, this, &SerialMonitorDialog::refreshPorts);
    connect(m_connectButton, &QPushButton::clicked, this, &SerialMonitorDialog::connectPort);
    connect(m_disconnectButton, &QPushButton::clicked, this, &SerialMonitorDialog::disconnectPort);
    connect(m_sendButton, &QPushButton::clicked, this, &SerialMonitorDialog::sendText);
    connect(m_input, &QLineEdit::returnPressed, this, &SerialMonitorDialog::sendText);
    if (m_deviceManager) {
        connect(m_deviceManager, &DeviceManager::portsChanged, this, &SerialMonitorDialog::refreshPorts);
    }

#ifdef HAVE_QT_SERIALPORT
    m_port = new QSerialPort(this);
    connect(m_port, &QSerialPort::readyRead, this, &SerialMonitorDialog::readIncoming);
    connect(m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) appendLog(QString("[serial error] %1").arg(m_port->errorString()));
        updateControls();
    });
#else
    appendLog("Qt SerialPort support is unavailable in this build.");
#endif
    refreshPorts();
    updateControls();
}

void SerialMonitorDialog::refreshPorts() {
    const QString previousPort = m_portPicker->currentData().toString();
    const QSignalBlocker blocker(m_portPicker);
    m_portPicker->clear();
    const QStringList ports = m_deviceManager ? m_deviceManager->portNames() : QStringList();
    if (ports.isEmpty()) m_portPicker->addItem("No serial ports detected", QString());
    for (const QString& port : ports) {
        const QVariantMap details = m_deviceManager->getPortDetails(port);
        m_portPicker->addItem(port, port);
        m_portPicker->setItemData(m_portPicker->count() - 1,
                                  details.value("description").toString(), Qt::ToolTipRole);
    }
    const int previousIndex = m_portPicker->findData(previousPort);
    if (previousIndex >= 0) m_portPicker->setCurrentIndex(previousIndex);
    updateControls();
}

void SerialMonitorDialog::connectPort() {
#ifdef HAVE_QT_SERIALPORT
    const QString portName = m_portPicker->currentData().toString();
    if (portName.isEmpty()) return;
    m_port->setPortName(portName);
    m_port->setBaudRate(m_baudRate->value());
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);
    if (!m_port->open(QIODevice::ReadWrite)) {
        appendLog(QString("[connect failed] %1").arg(m_port->errorString()));
    } else {
        appendLog(QString("[connected] %1 at %2 baud").arg(portName).arg(m_baudRate->value()));
    }
    updateControls();
#endif
}

void SerialMonitorDialog::disconnectPort() {
#ifdef HAVE_QT_SERIALPORT
    if (m_port && m_port->isOpen()) {
        const QString portName = m_port->portName();
        m_port->close();
        appendLog(QString("[disconnected] %1").arg(portName));
    }
    updateControls();
#endif
}

void SerialMonitorDialog::sendText() {
#ifdef HAVE_QT_SERIALPORT
    if (!m_port || !m_port->isOpen() || m_input->text().isEmpty()) return;
    const QByteArray payload = (m_input->text() + "\n").toUtf8();
    if (m_port->write(payload) < 0) {
        appendLog(QString("[send failed] %1").arg(m_port->errorString()));
        return;
    }
    appendLog(QString("[TX] %1").arg(m_input->text()));
    m_input->clear();
#endif
}

void SerialMonitorDialog::readIncoming() {
#ifdef HAVE_QT_SERIALPORT
    appendLog(QString::fromUtf8(m_port->readAll()));
#endif
}

void SerialMonitorDialog::appendLog(const QString& text) {
    m_log->moveCursor(QTextCursor::End);
    m_log->insertPlainText(text);
    if (!text.endsWith('\n')) m_log->insertPlainText("\n");
    m_log->moveCursor(QTextCursor::End);
}

void SerialMonitorDialog::updateControls() {
#ifdef HAVE_QT_SERIALPORT
    const bool connected = m_port && m_port->isOpen();
    m_portPicker->setEnabled(!connected);
    m_baudRate->setEnabled(!connected);
    m_connectButton->setEnabled(!connected && !m_portPicker->currentData().toString().isEmpty());
    m_disconnectButton->setEnabled(connected);
    m_input->setEnabled(connected);
    m_sendButton->setEnabled(connected);
#else
    m_portPicker->setEnabled(false);
    m_baudRate->setEnabled(false);
    m_connectButton->setEnabled(false);
    m_disconnectButton->setEnabled(false);
    m_input->setEnabled(false);
    m_sendButton->setEnabled(false);
#endif
}