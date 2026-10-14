#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <vector>
#include <map>
#include "BoardDatabase.h"

struct UsbDevice {
    int busNumber = 0;
    int deviceNumber = 0;
    QString vendorId;      // e.g., "0403"
    QString productId;     // e.g., "6010"
    QString manufacturer;
    QString description;
    QString serialNumber;
};

struct DetectedBoard {
    QString boardName;     // e.g., "ESP32 DEVKITV1", "STM32 NUCLEO-F030R8"
    QString boardType;     // e.g., "esp32", "stm32f0", "rp2040"
    QString usbAdapter;    // e.g., "CP2104", "ST-Link", "FTDI"
    QString openOcdConfig; // e.g., "target/esp32.cfg"
    UsbDevice usbDevice;
};

struct DetectedBoardInfo {
    bool detected = false;
    QString boardId;
    QString boardName;
    QString mcu;
    QString adapterType;
    QString vid;
    QString pid;
    QString serial;
    QString openocdConfig;
    QString suggestedInterface;
    BoardSpec spec;

    QString summary() const {
        if (!detected) return "No Board Detected";
        return QString("%1 (%2) via %3 [%4:%5]")
            .arg(boardName, mcu, adapterType, vid, pid);
    }
};

/**
 * @brief BoardDetector auto-detects connected microcontrollers and debug adapters
 * (ST-Link, FTDI, CP2104/CP2102, CH340, J-Link, CMSIS-DAP/Picoprobe, ESP USB-JTAG).
 * Matches against BoardDatabase to select the correct OpenOCD config without ANR timeouts.
 */
class BoardDetector : public QObject {
    Q_OBJECT

public:
    explicit BoardDetector(QObject* parent = nullptr);
    ~BoardDetector() override = default;

    // Auto-detect connected board and adapter
    // Returns null DetectedBoard if not found
    DetectedBoard detectBoard();

    // Get all connected USB devices
    std::vector<UsbDevice> getConnectedDevices();

    // Get OpenOCD config for adapter type
    QString getOpenOcdConfigForAdapter(const QString& adapterType, const QString& boardType);

    // Known USB adapter VID/PID mappings
    static const std::map<QString, QString> ADAPTER_VIDS;  // "ST-Link" -> "0483"

    // Parse lsusb output (Linux)
    std::vector<UsbDevice> parseLsusbOutput(const QString& output);

    // Platform-specific USB detection
    std::vector<UsbDevice> detectUsbDevices_Linux();
    std::vector<UsbDevice> detectUsbDevices_Windows();
    std::vector<UsbDevice> detectUsbDevices_Mac();

    // Identify board type and adapter from USB device
    QString identifyBoardType(const UsbDevice& device);
    QString identifyAdapterType(const UsbDevice& device);

    // Legacy and event polling methods
    DetectedBoardInfo detectConnectedBoard();
    DetectedBoardInfo lastDetectedBoard() const { return m_lastDetected; }

    void startPolling(int intervalMs = 1500);
    void stopPolling();

    // Direct adapter resolution for OpenOCD
    QString resolveOpenOCDConfig(const QString& boardIdOrName);

    // Simulated test injection
    void setSimulatedBoard(const QString& boardId);
    void clearSimulatedBoard();

signals:
    void boardDetected(const DetectedBoardInfo& info);
    void boardRemoved();

private:
    void onPollTimer();
    DetectedBoardInfo scanUsbSysfs();
    DetectedBoardInfo scanLsusb();

    QTimer m_pollTimer;
    DetectedBoardInfo m_lastDetected;
    bool m_simulated = false;
    DetectedBoardInfo m_simulatedInfo;
};
