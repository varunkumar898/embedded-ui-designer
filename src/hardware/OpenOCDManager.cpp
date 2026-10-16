#include "OpenOCDManager.h"
#include "HardwareBridge.h"
#include "BoardFactory.h"
#include "BoardRegistry.h"

#include <QTcpSocket>
#include <QEventLoop>
#include <QRegularExpression>
#include <QThread>

static int simulatedStates[8][16] = {}; // 8 ports, 16 pins each

OpenOCDManager::OpenOCDManager(QObject* parent)
    : QObject(parent)
{
    // Default fallback HAL from saved preset if available
    BoardPreset active = BoardRegistry::instance().activePreset();
    QString boardToSet = !active.boardId.isEmpty() ? active.boardId : "STM32F030R8";
    setBoard(boardToSet);
}

OpenOCDManager::~OpenOCDManager() {
    disconnect();
}

bool OpenOCDManager::createSocketConnection() {
    // Create socket if not already created
    if (!m_telnetSocket) {
        if (thread() == QThread::currentThread()) {
            m_telnetSocket = new QTcpSocket(this);
        } else {
            m_telnetSocket = new QTcpSocket();
            m_telnetSocket->moveToThread(this->thread());
            m_telnetSocket->setParent(this);
        }
    }

    // Close any existing connection
    if (m_telnetSocket->state() != QAbstractSocket::UnconnectedState) {
        m_telnetSocket->disconnectFromHost();
        if (m_telnetSocket->state() != QAbstractSocket::UnconnectedState) {
            m_telnetSocket->waitForDisconnected(1000);
        }
    }

    qDebug() << "Attempting to connect to OpenOCD at" << m_telnetHost << ":" << m_telnetPort;

    // Attempt connection
    m_telnetSocket->connectToHost(m_telnetHost, m_telnetPort);

    // Wait up to 2 seconds for connection to establish
    if (!m_telnetSocket->waitForConnected(2000)) {
        qWarning() << "Failed to connect to OpenOCD:" << m_telnetSocket->errorString();
        return false;
    }

    qDebug() << "Connected to OpenOCD Telnet";

    // Read welcome message (OpenOCD sends "Open On-Chip Debugger\n")
    if (!m_telnetSocket->waitForReadyRead(1000)) {
        qWarning() << "Timeout waiting for OpenOCD welcome message";
        return false;
    }

    QByteArray welcome = m_telnetSocket->readAll();
    qDebug() << "OpenOCD welcome:" << welcome;

    return true;
}

bool OpenOCDManager::connectToTelnet(const QString& host, int port) {
    m_telnetHost = host;
    m_telnetPort = port;

    bool ok = createSocketConnection();
    m_connected = ok;
    if (ok) {
        emit connectionChanged(true);
    }
    return ok;
}

bool OpenOCDManager::connect(const QString& host, int port) {
    if (!connectToTelnet(host, port)) {
        qWarning() << "[OpenOCDManager] OpenOCD connection failed on" << host << port;
        m_connected = false;
        return false;
    }

    // Auto-detect and initialize board
    if (!autoDetectAndInitializeBoard()) {
        BoardPreset active = BoardRegistry::instance().activePreset();
        QString fallbackBoard = !active.boardId.isEmpty() ? active.boardId : "STM32F030R8";
        qWarning() << "Board auto-detection failed. Falling back to" << fallbackBoard;
        setBoard(fallbackBoard);
    }

    return true;
}

bool OpenOCDManager::connectToOpenOCD(const QString& config, int timeoutMs) {
    Q_UNUSED(config);
    Q_UNUSED(timeoutMs);
    return connect(m_telnetHost, m_telnetPort);
}

void OpenOCDManager::autoDetectAndConnect() {
    m_lastDetectedBoard = m_boardDetector.detectBoard();

    if (!m_lastDetectedBoard.boardName.isEmpty()) {
        bool ok = connectToOpenOCD(m_lastDetectedBoard.openOcdConfig, 5000);
        if (ok) {
            emit connectionSucceeded(m_lastDetectedBoard.boardName);
        } else {
            setBoard(m_lastDetectedBoard.boardName);
            emit connectionFailed("OpenOCD server not reachable. Board detected: " + m_lastDetectedBoard.boardName);
        }
    } else {
        emit connectionFailed("No board detected. Please check USB connection or select manually.");
    }
}

void OpenOCDManager::disconnect() {
    if (m_telnetSocket) {
        if (m_telnetSocket->state() != QAbstractSocket::UnconnectedState) {
            m_telnetSocket->disconnectFromHost();
            if (m_telnetSocket->state() != QAbstractSocket::UnconnectedState) {
                m_telnetSocket->waitForDisconnected(1000);
            }
        }
        delete m_telnetSocket;
        m_telnetSocket = nullptr;
    }
    m_connected = false;
    emit connectionChanged(false);
}

bool OpenOCDManager::isConnected() const {
    if (m_telnetSocket) {
        return m_telnetSocket->state() == QAbstractSocket::ConnectedState;
    }
    return m_connected;
}

void OpenOCDManager::setConnected(bool connected) {
    m_connected = connected;
    emit connectionChanged(connected);
}

bool OpenOCDManager::autoDetectAndInitializeBoard() {
    if (!isConnected()) {
        qWarning() << "OpenOCD not connected. Cannot auto-detect board.";
        return false;
    }
    
    // Send "targets" command to OpenOCD to identify board
    QString response = sendCommand("targets");
    qDebug() << "OpenOCD targets output:" << response;
    
    // Detect board from response
    m_detectedBoard = BoardFactory::detectBoardFromOpenOCD(response);
    if (m_detectedBoard == "UNKNOWN") {
        qWarning() << "Failed to auto-detect board. Using fallback.";
        return false;
    }
    
    // Create board-specific HAL
    m_boardHAL = BoardFactory::createHAL(m_detectedBoard);
    if (!m_boardHAL) {
        qWarning() << "Failed to create HAL for board:" << m_detectedBoard;
        return false;
    }
    
    qDebug() << "Board detected and initialized:" << m_detectedBoard;
    qDebug() << "Manufacturer:" << m_boardHAL->getManufacturer();
    qDebug() << "Quirks:" << m_boardHAL->getQuirksAndLimitations();
    
    return true;
}

bool OpenOCDManager::setBoard(const QString& boardName) {
    if (!BoardFactory::isSupported(boardName)) {
        if (!BoardRegistry::instance().board(boardName).id.isEmpty()) {
            BoardDef bd = BoardRegistry::instance().board(boardName);
            if (bd.family.startsWith("ESP32", Qt::CaseInsensitive)) {
                m_boardHAL = BoardFactory::createHAL("ESP32");
            } else {
                m_boardHAL = BoardFactory::createHAL("STM32F030R8");
            }
        } else {
            qWarning() << "Board not supported:" << boardName;
            return false;
        }
    } else {
        m_boardHAL = BoardFactory::createHAL(boardName);
    }
    
    m_detectedBoard = boardName;
    
    BoardPreset active = BoardRegistry::instance().activePreset();
    if (!active.name.isEmpty()) {
        qDebug() << "Board preset set to:" << active.name;
    } else {
        qDebug() << "Board manually set to:" << m_detectedBoard;
    }
    return true;
}

QString OpenOCDManager::sendCommand(const QString& cmd) {
    QString trimmed = cmd.trimmed();
    qDebug() << "[OpenOCDManager] Sending command:" << trimmed;

    // 1. High-Level GPIO Control Commands
    if (trimmed.compare("LED_ON", Qt::CaseInsensitive) == 0) {
        m_ledState = true;
        // STM32 GPIOA BSRR: Bit 4 (PA4) and Bit 5 (PA5 / Nucleo LD2)
        writeMemoryWord(0x48000018, (1u << 4) | (1u << 5));
        HardwareBridge::instance().setDigitalOut("PA4", true);
        HardwareBridge::instance().setDigitalOut("PA5", true);
        emit gpioStateChanged("PA4", true);
        m_lastResponse = "OK: PA4 LED ON";
        emit responseReceived(m_lastResponse);
        return m_lastResponse;
    }

    if (trimmed.compare("LED_OFF", Qt::CaseInsensitive) == 0) {
        m_ledState = false;
        // STM32 GPIOA BSRR: Reset bits (16+4 and 16+5)
        writeMemoryWord(0x48000018, (1u << (4 + 16)) | (1u << (5 + 16)));
        HardwareBridge::instance().setDigitalOut("PA4", false);
        HardwareBridge::instance().setDigitalOut("PA5", false);
        emit gpioStateChanged("PA4", false);
        m_lastResponse = "OK: PA4 LED OFF";
        emit responseReceived(m_lastResponse);
        return m_lastResponse;
    }

    if (trimmed.compare("LED_TOGGLE", Qt::CaseInsensitive) == 0) {
        m_ledState = !m_ledState;
        if (m_ledState) {
            writeMemoryWord(0x48000018, (1u << 4) | (1u << 5));
        } else {
            writeMemoryWord(0x48000018, (1u << (4 + 16)) | (1u << (5 + 16)));
        }
        HardwareBridge::instance().setDigitalOut("PA4", m_ledState);
        HardwareBridge::instance().setDigitalOut("PA5", m_ledState);
        emit gpioStateChanged("PA4", m_ledState);
        m_lastResponse = m_ledState ? "OK: PA4 LED ON" : "OK: PA4 LED OFF";
        emit responseReceived(m_lastResponse);
        return m_lastResponse;
    }

    if (trimmed.compare("gpio_status", Qt::CaseInsensitive) == 0 ||
        trimmed.compare("LED_STATUS", Qt::CaseInsensitive) == 0) {
        return gpioStatus();
    }

    if (!isConnected()) {
        qWarning() << "Not connected to OpenOCD. Command:" << trimmed;
        return "";
    }

    if (!m_telnetSocket) {
        // Fallback: HardwareBridge OpenOCD TCL
        if (HardwareBridge::instance().isHardwareConnected() && trimmed.startsWith("mww ")) {
            auto parts = trimmed.split(' ', Qt::SkipEmptyParts);
            if (parts.size() >= 3) {
                bool ok1 = false, ok2 = false;
                quint32 addr = parts[1].toUInt(&ok1, 0);
                quint32 val = parts[2].toUInt(&ok2, 0);
                if (ok1 && ok2) {
                    writeMemoryWord(addr, val);
                    m_lastResponse = "OK";
                    return m_lastResponse;
                }
            }
        }
        m_lastResponse = "OK";
        return m_lastResponse;
    }

    // Send command with newline
    QString fullCommand = trimmed + "\n";
    qint64 written = m_telnetSocket->write(fullCommand.toUtf8());

    if (written == -1) {
        qWarning() << "Failed to send command:" << m_telnetSocket->errorString();
        return "";
    }

    qDebug() << "Sent to OpenOCD:" << trimmed.simplified();

    // Wait for response (up to 2 seconds)
    if (!m_telnetSocket->waitForReadyRead(2000)) {
        qWarning() << "Timeout waiting for OpenOCD response";
        return "";
    }

    // Read response
    QString response;
    while (m_telnetSocket->bytesAvailable() > 0) {
        response += QString::fromUtf8(m_telnetSocket->read(4096));
    }

    qDebug() << "OpenOCD response:" << response.simplified();
    m_lastResponse = response;
    emit responseReceived(response);

    return response;
}

bool OpenOCDManager::sendCommand(const QString& cmd, QString* outResponse) {
    QString resp = sendCommand(cmd);
    if (outResponse) {
        *outResponse = resp;
    }
    return !resp.isEmpty() && !resp.startsWith("ERR");
}

QString OpenOCDManager::readResponse() {
    if (!m_telnetSocket) return m_lastResponse;

    QByteArray response;
    while (m_telnetSocket->waitForReadyRead(300)) {
        response.append(m_telnetSocket->readAll());
    }

    if (!response.isEmpty()) {
        m_lastResponse = QString::fromUtf8(response).trimmed();
    }
    return m_lastResponse;
}

bool OpenOCDManager::sendRawSocket(const QString& line) {
    if (!m_telnetSocket || m_telnetSocket->state() != QAbstractSocket::ConnectedState) {
        return false;
    }
    m_telnetSocket->write(line.toUtf8());
    return m_telnetSocket->waitForBytesWritten(1000);
}

bool OpenOCDManager::writeMemoryWord(quint32 address, quint32 value) {
    // Write via HardwareBridge OpenOcdManager
    bool ok = HardwareBridge::instance().openOcdManager().writeMemoryWord(address, value);

    // If Telnet socket connected directly, also send mww
    if (m_telnetSocket && m_telnetSocket->state() == QAbstractSocket::ConnectedState) {
        QString cmd = QString("mww 0x%1 0x%2").arg(QString::number(address, 16).toUpper()).arg(QString::number(value, 16).toUpper());
        sendCommand(cmd);
    }
    return ok;
}

bool OpenOCDManager::readMemoryWord(quint32 address, quint32* outValue) {
    if (!outValue) return false;
    return HardwareBridge::instance().openOcdManager().readMemoryWord(address, outValue);
}

QString OpenOCDManager::gpioStatus() const {
    quint32 odr = 0;
    bool high = false;

    if (const_cast<OpenOCDManager*>(this)->readMemoryWord(0x48000014, &odr)) {
        high = (odr & (1u << 4)) != 0;
    } else {
        high = m_ledState;
    }

    return QString("PA4: %1 (ODR: 0x%2)")
        .arg(high ? "HIGH (ON)" : "LOW (OFF)")
        .arg(QString::number(odr, 16).toUpper().rightJustified(8, '0'));
}

bool OpenOCDManager::gpioToggle(uint8_t port, uint8_t pin) {
    if (!m_boardHAL) {
        qWarning() << "Board HAL not initialized. Cannot toggle GPIO.";
        return false;  // Fallback to simulated state
    }
    
    // Validate port/pin
    if (!m_boardHAL->isValidPin(port, pin)) {
        qWarning() << "Invalid GPIO:" << m_boardHAL->pinToString(port, pin);
        return false;
    }
    
    // Get GPIO base and register offsets from HAL
    uint32_t gpioBase = m_boardHAL->getGPIOBaseAddress(port);
    uint32_t bsrrOffset = m_boardHAL->getRegisterOffset_BSRR();
    uint32_t brrOffset = m_boardHAL->getRegisterOffset_BRR();
    
    // Read current state
    int currentState = gpioRead(port, pin);
    int newState = !currentState;
    
    // Write new state
    uint32_t bitMask = m_boardHAL->getBitMaskForPin(pin);
    uint32_t registerAddr = gpioBase + (newState ? bsrrOffset : brrOffset);
    
    QString command = QString("mww 0x%1 0x%2").arg(registerAddr, 8, 16, QChar('0')).arg(bitMask, 8, 16, QChar('0'));
    QString response = sendCommand(command);
    
    qDebug() << "GPIO Toggle:" << m_boardHAL->pinToString(port, pin) << "→" << newState
             << "(" << command << ")";
    
    if (port < 8 && pin < 16) {
        simulatedStates[port][pin] = newState;
    }

    return response.isEmpty() || response.contains("0 bytes") || response.contains("OK");
}

int OpenOCDManager::gpioRead(uint8_t port, uint8_t pin) {
    if (!m_boardHAL) {
        qWarning() << "Board HAL not initialized. Using simulated state.";
        return (port < 8 && pin < 16) ? simulatedStates[port][pin] : 0;  // Fallback
    }
    
    if (!m_boardHAL->isValidPin(port, pin)) {
        qWarning() << "Invalid GPIO:" << m_boardHAL->pinToString(port, pin);
        return -1;
    }

    if (!isConnected()) {
        return (port < 8 && pin < 16) ? simulatedStates[port][pin] : 0;
    }
    
    uint32_t gpioBase = m_boardHAL->getGPIOBaseAddress(port);
    uint32_t idrOffset = m_boardHAL->getRegisterOffset_IDR();
    uint32_t registerAddr = gpioBase + idrOffset;
    
    // Read IDR/GPIO_IN register
    QString command = QString("mdw 0x%1 1").arg(registerAddr, 8, 16, QChar('0'));
    QString response = sendCommand(command);
    
    // Parse response: typically "0x40020010: 0x0000f040"
    QStringList parts = response.split(':');
    if (parts.size() >= 2) {
        QString valuePart = parts[1].trimmed().split(' ')[0];
        bool ok;
        uint32_t regValue = valuePart.toUInt(&ok, 16);
        
        if (ok) {
            uint32_t bitMask = m_boardHAL->getBitMaskForPin(pin);
            int state = (regValue & bitMask) ? 1 : 0;
            if (port < 8 && pin < 16) {
                simulatedStates[port][pin] = state;
            }
            return state;
        }
    }
    
    if (port < 8 && pin < 16) {
        return simulatedStates[port][pin];
    }
    qWarning() << "Failed to read GPIO register. Response:" << response;
    return -1;
}
