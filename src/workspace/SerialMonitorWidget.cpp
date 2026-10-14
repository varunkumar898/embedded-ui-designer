#include "SerialMonitorWidget.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QScrollBar>

#ifdef HAVE_QT_SERIALPORT
#include <QSerialPortInfo>
#endif

SerialMonitorWidget::SerialMonitorWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    refreshPorts();
    updateControls();
}

SerialMonitorWidget::~SerialMonitorWidget() {
#ifdef HAVE_QT_SERIALPORT
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
    }
#endif
}

void SerialMonitorWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    const QString comboStyle = QString(
        "QComboBox { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: 4px; "
        "  padding: 3px 8px; "
        "  font-size: 11px; "
        "} "
        "QComboBox:disabled { "
        "  background-color: %4; "
        "  color: %5; "
        "} "
        "QComboBox::drop-down { "
        "  border: none; "
        "  width: 16px; "
        "} "
        "QComboBox QAbstractItemView { "
        "  background-color: %1; "
        "  color: %2; "
        "  selection-background-color: %6; "
        "  selection-color: %7; "
        "  border: 1px solid %3; "
        "} "
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, Theme::BgBase, Theme::TextMuted, Theme::Accent, Theme::AccentText);

    const QString btnSecondaryStyle = QString(
        "QPushButton { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: 4px; "
        "  padding: 4px 10px; "
        "  font-size: 11px; "
        "  font-weight: 500; "
        "} "
        "QPushButton:hover:enabled { "
        "  background-color: %4; "
        "  color: %5; "
        "} "
        "QPushButton:disabled { "
        "  background-color: %6; "
        "  color: %7; "
        "  border-color: %8; "
        "} "
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, Theme::Accent, Theme::AccentText, Theme::BgBase, Theme::TextMuted, Theme::BorderSubtle);

    // 1. Connection Toolbar
    // Layout: Port: [ports] [⟳]  Baud: [115200]  [Connect] [Disconnect]  ● Disconnected  Format: [Text]  [Clear]
    auto* toolBar = new QHBoxLayout();
    toolBar->setContentsMargins(0, 0, 0, 0);
    toolBar->setSpacing(8);

    auto* portLabel = new QLabel("Port:", this);
    portLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(Theme::TextSecondary));
    toolBar->addWidget(portLabel);

    m_portCombo = new QComboBox(this);
    m_portCombo->setObjectName("serialPortCombo");
    m_portCombo->setMinimumWidth(160);
    m_portCombo->setStyleSheet(comboStyle);
    toolBar->addWidget(m_portCombo);

    m_refreshBtn = new QPushButton("Refresh", this);
    m_refreshBtn->setObjectName("serialRefreshBtn");
    m_refreshBtn->setToolTip("Refresh Available Serial Ports");
    m_refreshBtn->setMinimumWidth(64);
    m_refreshBtn->setStyleSheet(btnSecondaryStyle);
    connect(m_refreshBtn, &QPushButton::clicked, this, &SerialMonitorWidget::refreshPorts);
    toolBar->addWidget(m_refreshBtn);

    auto* baudLabel = new QLabel("Baud:", this);
    baudLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;").arg(Theme::TextSecondary));
    toolBar->addWidget(baudLabel);

    m_baudCombo = new QComboBox(this);
    m_baudCombo->setObjectName("serialBaudCombo");
    m_baudCombo->addItems({"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"});
    m_baudCombo->setCurrentText("115200");
    m_baudCombo->setStyleSheet(comboStyle);
    toolBar->addWidget(m_baudCombo);

    m_connectBtn = new QPushButton("Connect", this);
    m_connectBtn->setObjectName("serialConnectBtn");
    m_connectBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #0284c7; "
        "  color: #ffffff; "
        "  border: 1px solid #0284c7; "
        "  border-radius: 4px; "
        "  padding: 4px 16px; "
        "  font-size: 11px; "
        "  font-weight: bold; "
        "} "
        "QPushButton:hover:enabled { "
        "  background-color: #0369a1; "
        "  border-color: #0369a1; "
        "} "
        "QPushButton:disabled { "
        "  background-color: #262626; "
        "  color: #666666; "
        "  border-color: #3f3f3f; "
        "} "
    );
    connect(m_connectBtn, &QPushButton::clicked, this, [this]() { connectPort(); });
    toolBar->addWidget(m_connectBtn);

    m_disconnectBtn = new QPushButton("Disconnect", this);
    m_disconnectBtn->setObjectName("serialDisconnectBtn");
    m_disconnectBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #2d2d2d; "
        "  color: #f48771; "
        "  border: 1px solid #3f3f3f; "
        "  border-radius: 4px; "
        "  padding: 4px 16px; "
        "  font-size: 11px; "
        "  font-weight: bold; "
        "} "
        "QPushButton:hover:enabled { "
        "  background-color: #dc2626; "
        "  color: #ffffff; "
        "  border-color: #dc2626; "
        "} "
        "QPushButton:disabled { "
        "  background-color: #262626; "
        "  color: #666666; "
        "  border-color: #3f3f3f; "
        "} "
    );
    connect(m_disconnectBtn, &QPushButton::clicked, this, &SerialMonitorWidget::disconnectPort);
    toolBar->addWidget(m_disconnectBtn);

    m_statusBadge = new QLabel("○ Disconnected", this);
    m_statusBadge->setObjectName("serialStatusLabel");
    m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 11px; margin-left: 4px;").arg(Theme::TextMuted));
    toolBar->addWidget(m_statusBadge);

    toolBar->addStretch(1);

    auto* formatLabel = new QLabel("Mode:", this);
    formatLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TextSecondary));
    toolBar->addWidget(formatLabel);

    m_formatCombo = new QComboBox(this);
    m_formatCombo->setObjectName("serialModeCombo");
    m_formatCombo->addItems({"Text", "Hex"});
    m_formatCombo->setStyleSheet(comboStyle);
    toolBar->addWidget(m_formatCombo);

    auto* endingLabel = new QLabel("Line Ending:", this);
    endingLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(Theme::TextSecondary));
    toolBar->addWidget(endingLabel);

    m_lineEndingCombo = new QComboBox(this);
    m_lineEndingCombo->setObjectName("serialLineEndingCombo");
    m_lineEndingCombo->addItems({"CRLF (\\r\\n)", "LF (\\n)", "CR (\\r)", "None"});
    m_lineEndingCombo->setStyleSheet(comboStyle);
    toolBar->addWidget(m_lineEndingCombo);

    m_clearBtn = new QPushButton("Clear", this);
    m_clearBtn->setObjectName("serialClearBtn");
    m_clearBtn->setStyleSheet(btnSecondaryStyle);
    connect(m_clearBtn, &QPushButton::clicked, this, &SerialMonitorWidget::clearLog);
    toolBar->addWidget(m_clearBtn);

    mainLayout->addLayout(toolBar);

    // 2. Serial Output Area
    m_logView = new QPlainTextEdit(this);
    m_logView->setObjectName("serialLogArea");
    m_logView->setReadOnly(true);
    QFont font("Monospace", 10);
    font.setStyleHint(QFont::Monospace);
    m_logView->setFont(font);
    m_logView->setStyleSheet(QString(
        "QPlainTextEdit { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: 4px; "
        "  padding: 6px; "
        "  font-family: monospace, 'Courier New', 'Consolas'; "
        "} "
    ).arg(Theme::BgBase, Theme::TextPrimary, Theme::BorderSubtle));
    mainLayout->addWidget(m_logView, 1);

    // 3. Send Input Line
    auto* sendLayout = new QHBoxLayout();
    sendLayout->setContentsMargins(0, 0, 0, 0);
    sendLayout->setSpacing(6);

    m_inputLine = new QLineEdit(this);
    m_inputLine->setObjectName("serialSendInput");
    m_inputLine->setFont(font);
    m_inputLine->setPlaceholderText("Enter data to send...");
    m_inputLine->setStyleSheet(QString(
        "QLineEdit { "
        "  background-color: %1; "
        "  color: %2; "
        "  border: 1px solid %3; "
        "  border-radius: 4px; "
        "  padding: 5px 10px; "
        "} "
        "QLineEdit:focus { "
        "  border: 1px solid %4; "
        "} "
        "QLineEdit:disabled { "
        "  background-color: %5; "
        "  color: %6; "
        "} "
    ).arg(Theme::BgRaised, Theme::TextPrimary, Theme::BorderSubtle, Theme::Accent, Theme::BgBase, Theme::TextMuted));
    connect(m_inputLine, &QLineEdit::returnPressed, this, [this]() { sendData(); });
    sendLayout->addWidget(m_inputLine, 1);

    m_sendBtn = new QPushButton("Send", this);
    m_sendBtn->setObjectName("serialSendBtn");
    m_sendBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #0284c7; "
        "  color: #ffffff; "
        "  font-weight: bold; "
        "  border: 1px solid #0284c7; "
        "  border-radius: 4px; "
        "  padding: 5px 18px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover:enabled { "
        "  background-color: #0369a1; "
        "  border-color: #0369a1; "
        "} "
        "QPushButton:disabled { "
        "  background-color: #262626; "
        "  color: #666666; "
        "  border-color: #3f3f3f; "
        "} "
    );
    connect(m_sendBtn, &QPushButton::clicked, this, [this]() { sendData(); });
    sendLayout->addWidget(m_sendBtn);

    mainLayout->addLayout(sendLayout);
}

void SerialMonitorWidget::refreshPorts() {
    QString currentSelected = m_portCombo->currentData().toString();
    m_portCombo->clear();

#ifdef HAVE_QT_SERIALPORT
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& port : ports) {
        QString desc = port.description().isEmpty() ? port.portName() : QString("%1 (%2)").arg(port.portName(), port.description());
        m_portCombo->addItem(desc, port.portName());
    }
#endif

    if (m_portCombo->count() == 0) {
        m_portCombo->addItem("No Serial Ports Detected", "");
    } else if (!currentSelected.isEmpty()) {
        int idx = m_portCombo->findData(currentSelected);
        if (idx >= 0) m_portCombo->setCurrentIndex(idx);
    }
}

void SerialMonitorWidget::updateControls() {
    m_connectBtn->setEnabled(!m_isConnected && m_portCombo->count() > 0 && !m_portCombo->currentData().toString().isEmpty());
    m_disconnectBtn->setEnabled(m_isConnected);
    m_portCombo->setEnabled(!m_isConnected);
    m_baudCombo->setEnabled(!m_isConnected);
    m_refreshBtn->setEnabled(!m_isConnected);

    m_inputLine->setEnabled(m_isConnected);
    m_sendBtn->setEnabled(m_isConnected);
}

void SerialMonitorWidget::connectPort() {
    if (m_isConnected) return;

    QString portName = m_portCombo->currentData().toString();
    if (portName.isEmpty()) {
        portName = m_portCombo->currentText();
    }
    if (portName.isEmpty()) {
        appendLog("[Error: No serial port selected. Please connect a device and refresh.]\n", false);
        return;
    }

    int baudRate = m_baudCombo->currentText().toInt();
    connectPort(portName, baudRate);
}

void SerialMonitorWidget::connectPort(const QString& portName, int baudRate) {
    if (portName.isEmpty()) {
        appendLog("[Error: No serial port specified.]\n", false);
        return;
    }

#ifdef HAVE_QT_SERIALPORT
    if (!m_serialPort) {
        m_serialPort = new QSerialPort(this);
        connect(m_serialPort, &QSerialPort::readyRead, this, &SerialMonitorWidget::onReadyRead);
    }

    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_isConnected = true;
        m_statusBadge->setText(QString("● Connected (%1)").arg(portName));
        m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::Success));
        appendLog(QString("[Connected to %1 @ %2 baud]\n").arg(portName).arg(baudRate), false);
        updateControls();
        return;
    } else {
        m_isConnected = false;
        m_statusBadge->setText("○ Connection Failed");
        m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::Danger));
        appendLog(QString("[Error: Failed to open %1: %2]\n").arg(portName, m_serialPort->errorString()), false);
        updateControls();
        return;
    }
#else
    Q_UNUSED(portName);
    Q_UNUSED(baudRate);
    m_isConnected = false;
    m_statusBadge->setText("○ Serial Port Unavailable");
    m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::Warning));
    appendLog("[Qt SerialPort module not compiled in build]\n", false);
    updateControls();
#endif
}

void SerialMonitorWidget::disconnectPort() {
    if (!m_isConnected) return;

#ifdef HAVE_QT_SERIALPORT
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
    }
#endif

    m_isConnected = false;
    m_statusBadge->setText("○ Disconnected");
    m_statusBadge->setStyleSheet(QString("color: %1; font-weight: bold;").arg(Theme::TextMuted));
    appendLog("[Serial Monitor Disconnected]\n", false);
    updateControls();
}

void SerialMonitorWidget::sendData() {
    QString text = m_inputLine->text();
    m_inputLine->clear();
    sendData(text);
}

void SerialMonitorWidget::sendData(const QString& text) {
    if (!m_isConnected) {
        appendLog("[Error: Cannot send while disconnected.]\n", false);
        return;
    }

    if (text.isEmpty()) return;

    QString endingStr;
    QString endingChoice = m_lineEndingCombo->currentText();
    if (endingChoice.startsWith("CRLF")) endingStr = "\r\n";
    else if (endingChoice.startsWith("LF")) endingStr = "\n";
    else if (endingChoice.startsWith("CR")) endingStr = "\r";

    appendLog(QString(">> %1\n").arg(text), false);

#ifdef HAVE_QT_SERIALPORT
    if (m_serialPort && m_serialPort->isOpen()) {
        QByteArray payload = text.toUtf8() + endingStr.toUtf8();
        qint64 written = m_serialPort->write(payload);
        if (written == -1) {
            appendLog(QString("[Write Error: %1]\n").arg(m_serialPort->errorString()), false);
        }
    }
#endif
}

void SerialMonitorWidget::onReadyRead() {
#ifdef HAVE_QT_SERIALPORT
    if (m_serialPort) {
        QByteArray data = m_serialPort->readAll();
        if (m_formatCombo->currentText() == "Hex") {
            QString hexStr;
            for (int i = 0; i < data.size(); ++i) {
                hexStr += QString("%1 ").arg(static_cast<uint8_t>(data.at(i)), 2, 16, QChar('0')).toUpper();
            }
            appendLog(hexStr + "\n", true);
        } else {
            appendLog(QString::fromUtf8(data), true);
        }
    }
#endif
}

void SerialMonitorWidget::clearLog() {
    m_logView->clear();
}

void SerialMonitorWidget::appendLog(const QString& text, bool isIncoming) {
    QTextCursor cursor = m_logView->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat format;
    if (isIncoming) {
        format.setForeground(QColor(Theme::TextPrimary));
    } else {
        format.setForeground(QColor(Theme::Accent));
    }
    cursor.setCharFormat(format);
    cursor.insertText(text);

    m_logView->verticalScrollBar()->setValue(m_logView->verticalScrollBar()->maximum());
}
