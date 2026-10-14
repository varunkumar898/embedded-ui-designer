#pragma once

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QByteArray>
#include <QDebug>
#include <QElapsedTimer>
#include <memory>

#include "BoardHAL.h"
#include "BoardFactory.h"
#include "BoardDetector.h"

/**
 * @brief Qt wrapper for OpenOCD Telnet & TCL debug session.
 * Provides real-time command sending, GPIO control (LED_ON, LED_OFF, LED_TOGGLE, gpio_status),
 * register reading/writing, and response parsing.
 */
class OpenOCDManager : public QObject {
    Q_OBJECT

public:
    explicit OpenOCDManager(QObject* parent = nullptr);
    ~OpenOCDManager() override;

    // OpenOCD Connection Management
    bool connect(const QString& host = "127.0.0.1", int port = 4444);
    bool connectToTelnet(const QString& host = "127.0.0.1", int port = 4444);
    void disconnect();
    bool isConnected() const;
    void setConnected(bool connected);

    /**
     * @brief Auto-detect board via OpenOCD and initialize HAL.
     * Called after OpenOCD connects.
     */
    bool autoDetectAndInitializeBoard();

    /**
     * @brief Get the currently loaded board HAL.
     * @return Pointer to BoardHAL (nullptr if not initialized)
     */
    BoardHAL* getBoardHAL() const { return m_boardHAL.get(); }

    /**
     * @brief Get detected board name.
     * @return e.g., "STM32F030R8", "ESP32"
     */
    QString getDetectedBoardName() const { return m_detectedBoard; }

    /**
     * @brief Manually set board (for testing or if auto-detect fails).
     * @param boardName Board name string
     * @return true if board supported
     */
    bool setBoard(const QString& boardName);

    // Auto-detect and connect to board
    void autoDetectAndConnect();
    bool connectToOpenOCD(const QString& config, int timeoutMs = 5000);
    DetectedBoard lastDetectedBoard() const { return m_lastDetectedBoard; }
    BoardDetector& boardDetector() { return m_boardDetector; }

    // Command Dispatch (Telnet / TCL / High-level GPIO)
    // Returns response string (e.g. for gpio_status or debug reply)
    QString sendCommand(const QString& cmd);
    bool sendCommand(const QString& cmd, QString* outResponse);
    QString readResponse();

    // Direct Register / Memory Access for Cortex-M
    bool writeMemoryWord(quint32 address, quint32 value);
    bool readMemoryWord(quint32 address, quint32* outValue);

    // Live GPIO State inspection & Control
    bool gpioToggle(uint8_t port, uint8_t pin);
    int gpioRead(uint8_t port, uint8_t pin);
    bool isLedOn() const { return m_ledState; }
    QString gpioStatus() const;

signals:
    void connectionChanged(bool connected);
    void connectionFailed(const QString& reason);
    void connectionSucceeded(const QString& boardName);
    void responseReceived(const QString& response);
    void gpioStateChanged(const QString& pin, bool state);
    void logMessage(const QString& msg);

private:
    void handleSocketData();
    bool sendRawSocket(const QString& line);

    QTcpSocket* m_telnetSocket = nullptr;
    QString m_telnetHost = "127.0.0.1";
    int m_telnetPort = 4444;

    /**
     * @brief Actually create the TCP socket connection.
     */
    bool createSocketConnection();

    bool m_connected = false;
    QString m_lastResponse;
    bool m_ledState = false;

    // Board-specific HAL
    std::unique_ptr<BoardHAL> m_boardHAL;
    QString m_detectedBoard;

    BoardDetector m_boardDetector;
    DetectedBoard m_lastDetectedBoard;
};
