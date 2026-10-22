#include "OcdTelnetManager.h"
#include "HardwareBridge.h"
#include "WriteGuard.h"
#include "BoardFactory.h"
#include "BoardRegistry.h"

#include <QTcpSocket>
#include <QEventLoop>
#include <QRegularExpression>
#include <QThread>
#include <QDateTime>

static int simulatedStates[8][16] = {}; // 8 ports, 16 pins each

namespace {
// R1.e: the QTcpSocket belongs to the thread OcdTelnetManager was created
// on. Foreign-thread callers (e.g. OpenOCDWorker) are marshalled onto that
// thread with a blocking queued connection instead of using the socket
// runtime from an unrelated thread (undefined behavior in Qt).
template <typename F>
auto runOnOwner(OcdTelnetManager* self, F&& fn) -> decltype(fn()) {
    if (QThread::currentThread() == self->thread()) {
        return fn();
    }
    using R = decltype(fn());
    if constexpr (std::is_void_v<R>) {
        QMetaObject::invokeMethod(self, std::forward<F>(fn), Qt::BlockingQueuedConnection);
    } else {
        R out{};
        QMetaObject::invokeMethod(self, [&]() { out = fn(); }, Qt::BlockingQueuedConnection);
        return out;
    }
}
} // namespace

OcdTelnetManager::OcdTelnetManager(QObject* parent)
    : QObject(parent)
{
    // Default fallback HAL from saved preset if available
    BoardPreset active = BoardRegistry::instance().activePreset();
    QString boardToSet = !active.boardId.isEmpty() ? active.boardId : "STM32F030R8";
    setBoard(boardToSet);
}

OcdTelnetManager::~OcdTelnetManager() {
    disconnect();
}

bool OcdTelnetManager::createSocketConnection() {
    // Create socket if not already created. R1.e: the socket is ALWAYS
    // parented to this manager (single ownership, owner-thread lifetime);
    // the previous bare-new branch created unparented sockets on foreign
    // threads, which the manager could not safely reach or delete.
    if (!m_telnetSocket) {
        m_telnetSocket = new QTcpSocket(this);
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

bool OcdTelnetManager::connectToTelnet(const QString& host, int port) {
    // R1.e: foreign threads (OpenOCDWorker) must not touch the socket
    // directly; marshal onto the owning thread.
    return runOnOwner(this, [&]{
        return connectToTelnetInternal(host, port);
    });
}

bool OcdTelnetManager::connectToTelnetInternal(const QString& host, int port) {
    m_telnetHost = host;
    m_telnetPort = port;

    bool ok = createSocketConnection();
    m_connected = ok;
    if (ok) {
        emit connectionChanged(true);
    }
    return ok;
}

bool OcdTelnetManager::connect(const QString& host, int port) {
    if (!connectToTelnet(host, port)) {
        qWarning() << "[OcdTelnetManager] OpenOCD connection failed on" << host << port;
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

bool OcdTelnetManager::connectToOpenOCD(const QString& config, int timeoutMs) {
    Q_UNUSED(config);
    Q_UNUSED(timeoutMs);
    return connect(m_telnetHost, m_telnetPort);
}

void OcdTelnetManager::autoDetectAndConnect() {
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

void OcdTelnetManager::disconnect() {
    // R1.e: owner-thread marshal; this also covers the destructor on the
    // owning thread and the worker-thread teardown path.
    runOnOwner(this, [&]{
        if (m_telnetSocket) {
            if (m_telnetSocket->state() != QAbstractSocket::UnconnectedState) {
                m_telnetSocket->disconnectFromHost();
                if (m_telnetSocket->state() != QAbstractSocket::UnconnectedState) {
                    m_telnetSocket->waitForDisconnected(1000);
                }
            }
            m_telnetSocket->deleteLater();
            m_telnetSocket = nullptr;
        }
        m_connected = false;
        emit connectionChanged(false);
    });
}

bool OcdTelnetManager::isConnected() const {
    if (m_telnetSocket) {
        return m_telnetSocket->state() == QAbstractSocket::ConnectedState;
    }
    return m_connected;
}

void OcdTelnetManager::setConnected(bool connected) {
    m_connected = connected;
    emit connectionChanged(connected);
}

bool OcdTelnetManager::autoDetectAndInitializeBoard() {
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

bool OcdTelnetManager::setBoard(const QString& boardName) {
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

QString OcdTelnetManager::sendCommand(const QString& cmd) {
    // R1.e: foreign threads marshal onto the owning thread (socket runs there).
    return runOnOwner(this, [&]{
        return sendCommandInternal(cmd);
    });
}

bool OcdTelnetManager::sendCommand(const QString& cmd, QString* outResponse) {
    QString resp = sendCommand(cmd);
    if (outResponse) {
        *outResponse = resp;
    }
    return !resp.isEmpty() && !resp.startsWith("ERR");
}

bool OcdTelnetManager::ledBsrrAddress(quint32* outAddress, QString* errOut) {
    // R1.e: the LED_* BSRR address comes from the current board profile
    // (GPIO base + STM32-family BSRR offset 0x18) - never a hardcoded
    // chip address. Non-STM32 profiles have no such register semantics.
    const BoardProfile bp = HardwareBridge::instance().currentBoard();
    const bool stmFamily = bp.mcuFamily.startsWith("STM32", Qt::CaseInsensitive)
                           || bp.mcuFamily.compare("STM32F0", Qt::CaseInsensitive) == 0
                           || bp.mcuFamily.contains("STM32", Qt::CaseInsensitive);
    if (!outAddress) {
        return false;
    }
    if (bp.gpioBase == 0 || !stmFamily || bp.connectionType == "serial"
        || bp.connectionType == "mock") {
        if (errOut) {
            *errOut = QString("LED_* GPIO commands require an STM32-family board "
                              "(profile %1 has base 0x%2, connection '%3').")
                          .arg(bp.name)
                          .arg(bp.gpioBase, 0, 16)
                          .arg(bp.connectionType);
        }
        return false;
    }
    *outAddress = bp.gpioBase + 0x18;
    return true;
}

QString OcdTelnetManager::sendCommandInternal(const QString& cmd) {
    QString trimmed = cmd.trimmed();
    qDebug() << "[OcdTelnetManager] Sending command:" << trimmed;

    // 1. High-Level GPIO Control Commands. R1.e: transport is the single
    // writeMemoryWord path (telnet socket when connected, else the bridge's
    // OpenOCD manager channel / simulated cache) with a profile-derived
    // address, and responses state the outcome instead of a fake ack.
    QString ledErr;
    quint32 bsrrAddress = 0;
    const bool ledCommandOk = ledBsrrAddress(&bsrrAddress, &ledErr);

    if (trimmed.compare("LED_ON", Qt::CaseInsensitive) == 0) {
        if (!ledCommandOk) {
            m_lastResponse = QString("ERR: %1").arg(ledErr);
            emit responseReceived(m_lastResponse);
            return m_lastResponse;
        }
        m_ledState = true;
        const bool wrote = writeMemoryWord(bsrrAddress, (1u << 4) | (1u << 5));
        HardwareBridge::instance().setDigitalOut("PA4", true);
        HardwareBridge::instance().setDigitalOut("PA5", true);
        emit gpioStateChanged("PA4", true);
        m_lastResponse = wrote ? "OK: PA4/PA5 set (BSRR write accepted)"
                               : "ERR: BSRR write rejected or transport unavailable";
        emit responseReceived(m_lastResponse);
        return m_lastResponse;
    }

    if (trimmed.compare("LED_OFF", Qt::CaseInsensitive) == 0) {
        if (!ledCommandOk) {
            m_lastResponse = QString("ERR: %1").arg(ledErr);
            emit responseReceived(m_lastResponse);
            return m_lastResponse;
        }
        m_ledState = false;
        const bool wrote = writeMemoryWord(bsrrAddress, (1u << (4 + 16)) | (1u << (5 + 16)));
        HardwareBridge::instance().setDigitalOut("PA4", false);
        HardwareBridge::instance().setDigitalOut("PA5", false);
        emit gpioStateChanged("PA4", false);
        m_lastResponse = wrote ? "OK: PA4/PA5 cleared (BSRR write accepted)"
                               : "ERR: BSRR write rejected or transport unavailable";
        emit responseReceived(m_lastResponse);
        return m_lastResponse;
    }

    if (trimmed.compare("LED_TOGGLE", Qt::CaseInsensitive) == 0) {
        if (!ledCommandOk) {
            m_lastResponse = QString("ERR: %1").arg(ledErr);
            emit responseReceived(m_lastResponse);
            return m_lastResponse;
        }
        m_ledState = !m_ledState;
        const quint32 bsrrValue = m_ledState ? ((1u << 4) | (1u << 5))
                                             : ((1u << (4 + 16)) | (1u << (5 + 16)));
        const bool wrote = writeMemoryWord(bsrrAddress, bsrrValue);
        HardwareBridge::instance().setDigitalOut("PA4", m_ledState);
        HardwareBridge::instance().setDigitalOut("PA5", m_ledState);
        emit gpioStateChanged("PA4", m_ledState);
        m_lastResponse = wrote ? (m_ledState ? "OK: PA4/PA5 set (BSRR write accepted)"
                                             : "OK: PA4/PA5 cleared (BSRR write accepted)")
                               : "ERR: BSRR write rejected or transport unavailable";
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

    // R1.e: wait for the prompt-terminated reply (chunked reads), not just
    // the first packet - long "targets"-style output arrives in pieces.
    const QByteArray reply = readPromptTerminated(2500);
    if (reply.isEmpty() && !m_lastReadComplete) {
        qWarning() << "Timeout waiting for OpenOCD prompt-terminated response";
        return "";
    }

    QString response = QString::fromUtf8(reply);
    qDebug() << "OpenOCD response:" << response.simplified();
    m_lastResponse = response;
    emit responseReceived(response);

    return response;
}

QString OcdTelnetManager::readResponse() {
    // R1.e: owner-thread marshal (see sendCommand).
    return runOnOwner(this, [&]{
        return readResponseInternal();
    });
}

qsizetype OcdTelnetManager::completeReplyLength(const QByteArray& buffer) {
    // An OpenOCD telnet reply ends with the prompt "> " on its own line:
    // "<reply>\r\n> " or "<reply>\n> ". A complete reply exists when the
    // buffer ends with "> " and there is newline content before it.
    if (buffer.size() < 2 || !buffer.endsWith("> ")) {
        return -1;
    }
    qsizetype promptStart = buffer.size() - 2;
    // Strip at most one preceding CRLF/LF pair before the prompt.
    qsizetype end = promptStart;
    if (end > 0 && buffer.at(end - 1) == '\n') {
        --end;
        if (end > 0 && buffer.at(end - 1) == '\r') {
            --end;
        }
    }
    return end;
}

QByteArray OcdTelnetManager::readPromptTerminated(int timeoutMs) {
    QByteArray buffer;
    m_lastReadComplete = false;
    const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + timeoutMs;
    while (true) {
        const qint64 remaining = deadline - QDateTime::currentMSecsSinceEpoch();
        if (remaining <= 0) {
            break;
        }
        if (m_telnetSocket->waitForReadyRead(static_cast<int>(qMin<qint64>(200, remaining)))) {
            buffer.append(m_telnetSocket->readAll());
        }
        const qsizetype complete = completeReplyLength(buffer);
        if (complete >= 0) {
            m_lastReadComplete = true;
            buffer.resize(static_cast<int>(complete));
            break;
        }
    }
    return buffer;
}

QString OcdTelnetManager::readResponseInternal() {
    if (!m_telnetSocket) return m_lastResponse;

    const QByteArray response = readPromptTerminated(300);
    if (!response.isEmpty()) {
        m_lastResponse = QString::fromUtf8(response).trimmed();
    }
    return m_lastResponse;
}

bool OcdTelnetManager::sendRawSocket(const QString& line) {
    if (!m_telnetSocket || m_telnetSocket->state() != QAbstractSocket::ConnectedState) {
        return false;
    }
    m_telnetSocket->write(line.toUtf8());
    return m_telnetSocket->waitForBytesWritten(1000);
}

bool OcdTelnetManager::writeMemoryWord(quint32 address, quint32 value) {
    // R1.e: foreign threads must not read the socket state directly either;
    // marshal the memory-write decision onto the owning thread.
    return runOnOwner(this, [&]{
        return writeMemoryWordInternal(address, value);
    });
}

bool OcdTelnetManager::writeMemoryWordInternal(quint32 address, quint32 value) {
    QString reason;
    if (!Hardware::WriteGuard::allow(address, HardwareBridge::instance().allowedWriteAddresses(),
                                    HardwareBridge::instance().isLiveWriteAllowed(),
                                    HardwareBridge::instance().writeOverride(), &reason)) {
        qWarning() << "[OcdTelnetManager] write rejected:" << reason;
        HardwareBridge::instance().logWrite(address, value, false, "OcdTelnetManager");
        return false;
    }
    HardwareBridge::instance().logWrite(address, value, true, "OcdTelnetManager");

    // R1.e: SINGLE transport channel. The same daemon would otherwise
    // receive the same mww twice (once over this telnet link, once over
    // the bridge's TCL link). Prefer this class's telnet socket when it
    // is connected and parse the daemon's verdict; otherwise rely on the
    // bridge's OpenOCD manager (TCL socket or simulated cache).
    if (m_telnetSocket && m_telnetSocket->state() == QAbstractSocket::ConnectedState) {
        const QString cmd = QString("mww 0x%1 0x%2")
                                .arg(QString::number(address, 16).toUpper())
                                .arg(QString::number(value, 16).toUpper());
        const QString response = sendCommand(cmd);
        // A successful mww is silent apart from the daemon's prompt; the
        // error path prints an Error/reason line in the reply.
        const bool ok = !response.contains("Error", Qt::CaseInsensitive)
                        && !response.contains("rejected", Qt::CaseInsensitive)
                        && !response.contains("Fail", Qt::CaseInsensitive);
        if (!ok) {
            qWarning() << "[OcdTelnetManager] mww failed via telnet:" << response;
        }
        return ok;
    }

    return HardwareBridge::instance().openOcdManager().writeMemoryWord(address, value);
}

bool OcdTelnetManager::readMemoryWord(quint32 address, quint32* outValue) {
    if (!outValue) return false;
    return runOnOwner(this, [&]{
        return HardwareBridge::instance().openOcdManager().readMemoryWord(address, outValue);
    });
}

QString OcdTelnetManager::gpioStatus() const {
    quint32 odr = 0;
    bool high = false;

    // R1.e: ODR address derives from the board profile (GPIO base + 0x14),
    // falling back to the F0-family address only when the profile carries
    // no GPIO base (e.g. mock boards).
    const BoardProfile bp = HardwareBridge::instance().currentBoard();
    const quint32 odrAddress = bp.gpioBase != 0
                                   ? bp.gpioBase + 0x14
                                   : 0x48000014;
    if (const_cast<OcdTelnetManager*>(this)->readMemoryWord(odrAddress, &odr)) {
        high = (odr & (1u << 4)) != 0;
    } else {
        high = m_ledState;
    }

    return QString("PA4: %1 (ODR: 0x%2)")
        .arg(high ? "HIGH (ON)" : "LOW (OFF)")
        .arg(QString::number(odr, 16).toUpper().rightJustified(8, '0'));
}

bool OcdTelnetManager::gpioToggle(uint8_t port, uint8_t pin) {
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

int OcdTelnetManager::gpioRead(uint8_t port, uint8_t pin) {
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
