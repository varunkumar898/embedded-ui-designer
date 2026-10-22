#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QPointer>

class UIComponent;
class ProgressBarComponent;
class LabelComponent;
class SliderComponent;

enum class PinMode {
    None,
    DigitalIn,
    DigitalOut,
    PwmOutput,
    AnalogIn,
    SpiRawTransfer,
    InputPullUp,
    InputPullDown,
    OpenDrainOut,
    AlternateFunction,
    InterruptInput,
    I2cBus,
    UartTxRx,
    CanBus
};

QString pinModeToString(PinMode mode);
PinMode stringToPinMode(const QString& str);

enum class ConnectionState {
    Disconnected,
    DetectingProbe,
    Connecting,
    Connected,
    ConnectionFailed,
    DeviceRemoved,
    SimulationMode
};

QString connectionStateToString(ConnectionState state);

struct AdcMapping {
    bool available = false;
    QString peripheral = "ADC1";
    int channel = 0;
    int resolutionBits = 12; // 0 to (2^bits - 1)
    quint32 dataReg = 0x40012440;
    quint32 triggerReg = 0x40012408;
};

struct PwmMapping {
    bool available = false;
    QString timer = "TIM3";
    int channel = 1;
    int alternateFunction = 1;
    quint32 ccrReg = 0x40000434;
};

struct SpiMapping {
    bool available = false;
    QString peripheral = "SPI1";
    QString sckPin;
    QString misoPin;
    QString mosiPin;
    int alternateFunction = 0;
    quint32 cr1Reg = 0x40013000;
    quint32 drReg = 0x4001300C;
    quint32 srReg = 0x40013008;
};

struct PinConflict {
    bool hasConflict = false;
    QString pin;
    QString reason;
    QString conflictingTask;
};

struct PinProfile {
    QString pinName;
    QList<PinMode> supportedModes;
    PinMode activeMode = PinMode::None; // Legacy / Active Task mode
    PinMode taskAMode = PinMode::None;
    PinMode taskBMode = PinMode::None;
    QString taskAPeripheral;
    QString taskBPeripheral;
    QString taskAComponentId;
    QString taskBComponentId;
    QString portGroup; // "Port A", "Port B", "GPIO", etc.
    QString description;
    bool isReserved = false;
    QString reservedReason;
    QStringList supportedPeripherals;
    AdcMapping adc;
    PwmMapping pwm;
    SpiMapping spi;
};

#include "OpenOcdManager.h"

struct BoardProfile {
    QString id;
    QString name;
    QString mcuFamily;
    QString connectionType = "openocd"; // "openocd", "serial", "linux", "mock", "linkserver", "stcisp"
    quint32 gpioBase = 0x48000000;
    quint32 bsrrOffset = 0x18;
    QString openocdInterface = "interface/stlink.cfg";
    QString openocdTarget = "target/stm32f0x.cfg";
    QMap<QString, PinProfile> pins;
    QStringList spiBusses;
    QStringList portGroups;

    // R1.b: adapter/transport identity sourced from the board catalog
    // (config/boards/board-specs/*.json "usb_adapter"). A serial transport is
    // selected ONLY when a device with these USB ids is attached - never by
    // "some tty exists".
    QString adapterKind;             // "stlink", "esptool", "linux-gpio", "mock", "picotool", ...
    QString adapterVid;              // normalized 4-hex, e.g. "0483"
    QString adapterPid;              // e.g. "374b"
    QStringList adapterAltVidPids;   // extra accepted "vid:pid" pairs
};

class HardwareBridge : public QObject {
    Q_OBJECT

public:
    static HardwareBridge& instance();

    // Board Profile Management
    QList<BoardProfile> availableBoards() const;
    BoardProfile currentBoard() const;
    bool setBoard(const QString& boardId);
    bool addBoard(const BoardProfile& profile);
    bool hasBoard(const QString& boardId) const;

    // R1.b: catalog-driven transport/adapter selection (unit-testable helpers)
    static QString normalizeUsbId(const QString& raw); // "0x0483"|"483"|"0483" -> "0483"
    // ports: list of ((vid,pid), portName). Returns the name of the FIRST port
    // whose vid:pid matches the profile's adapter ids, or "" when none does.
    static QString matchSerialPort(const BoardProfile& profile,
                                   const QList<QPair<QPair<QString,QString>,QString>>& usbSerialPorts,
                                   QString* matchedAdapter = nullptr);
    // Serial port paired by catalog match during connectHardware(). Port that
    // downstream consumers (telemetry, flashing) must use: never the first tty.
    QString lastSerialPortName() const { return m_lastSerialPortName; }
    bool isSerialPortMatched() const { return !m_lastSerialPortName.isEmpty(); }
    void setTestSerialPorts(const QList<QPair<QPair<QString,QString>,QString>>& ports);
    void clearTestSerialPorts() { m_testPortsActive = false; m_testSerialPorts.clear(); }
    bool isLiveWriteAllowed() const;
    QSet<quint32> allowedWriteAddresses() const;
    bool writeOverride() const { return m_writeOverride; }
    void setWriteOverride(bool on) { m_writeOverride = on; }
    void logWrite(quint32 address, quint32 value, bool ok, const QString& trigger);

    // Pin Capability & Mode Inspection
    QStringList availablePins() const;
    QStringList availablePinsForPort(const QString& portFilter) const;
    QStringList portGroupsForCurrentBoard() const;
    PinProfile pinProfile(const QString& pin) const;
    bool isModeAvailable(const QString& pin, PinMode mode) const;
    PinMode activePinMode(const QString& pin) const;
    bool setPinMode(const QString& pin, PinMode mode);

    // Multi-task Pin Assignment (Task A, Task B, All Tasks)
    QString activeTask() const { return m_activeTask; }
    void setActiveTask(const QString& task);
    PinMode taskPinMode(const QString& pin, const QString& task) const;
    bool setTaskPinMode(const QString& pin, const QString& task, PinMode mode);
    QString taskPeripheral(const QString& pin, const QString& task) const;
    bool setTaskPeripheral(const QString& pin, const QString& task, const QString& peripheral);
    QString taskComponentBinding(const QString& pin, const QString& task) const;
    void setTaskComponentBinding(const QString& pin, const QString& task, const QString& componentId);

    // Conflict & Validation Checking
    PinConflict checkPinConflict(const QString& pin) const;
    QList<PinConflict> allConflicts() const;
    bool validateAssignments(QStringList* outErrors = nullptr, QStringList* outWarnings = nullptr) const;

    // ADC setup and poll (Task A)
    bool setupAdc(const QString& pin);
    bool pollAdc(const QString& pin, quint32* outRawCount);

    // SPI Raw Transfer (Task A)
    bool setupSpi(const QString& spiBus);
    bool spiRawTransfer(const QString& spiBus, quint8 byteOut, quint8* outByteIn, QString* outLog = nullptr);

    // PWM Setup and Write (Task C)
    bool setupPwm(const QString& pin);
    bool writePwmDuty(const QString& pin, double dutyPercent); // 0.0 - 100.0%

    // Digital IO (Task B)
    bool setDigitalOut(const QString& pin, bool high);
    bool readDigitalIn(const QString& pin, bool* outHigh);
    bool calculateBsrrAddress(const QString& pin, quint32* outAddr, quint32* outSetMask, quint32* outResetMask) const;
    bool writeGpioBsrr(const QString& pin, bool high);

    // I2C Bus Scan & Discovery (Task C)
    bool scanI2cBus(const QString& sclPin, const QString& sdaPin, QList<quint8>* outFoundAddresses, QString* outLog = nullptr);

    // Component Binding (Task C & Task B)
    void bindComponent(UIComponent* comp, const QString& pin, PinMode mode);
    void unbindComponent(UIComponent* comp);
    void unbindPin(const QString& pin);
    QString boundPinForComponent(const UIComponent* comp) const;
    PinMode boundModeForComponent(const UIComponent* comp) const;
    UIComponent* componentBoundToPin(const QString& pin) const;

    // Connection Controls & Multi-Platform Probe Management
    OpenOcdManager& openOcdManager() { return m_openOcd; }
    const OpenOcdManager& openOcdManager() const { return m_openOcd; }
    ConnectionState connectionState() const { return m_connectionState; }
    QString connectionStatusText() const;
    QString connectionButtonLabel() const;
    QString lastConnectionError() const { return m_lastConnectionError; }
    QString connectedProbeName() const;
    bool isProbeDetected() const;
    QString detectedBoardName() const;
    bool detectProbe();
    void rescan();
    bool connectHardware();
    bool disconnectHardware();

    // Simulation / Test overrides
    bool isHardwareConnected() const { return (m_connectionState == ConnectionState::Connected); }
    bool isSimulationMode() const { return m_simulationMode; }
    void setSimulationMode(bool enabled);
    void setHardwareConnected(bool connected);
    void setSimulatedAdcCount(const QString& pin, quint32 rawCount);
    quint32 simulatedAdcCount(const QString& pin) const;

    // Polling control
    bool isLivePolling() const { return m_pollTimer.isActive(); }
    void startLivePolling(int intervalMs = 50);
    void stopLivePolling();

    // Serialization / Deserialization
    QJsonObject saveHardwareStateToJson() const;
    bool loadHardwareStateFromJson(const QJsonObject& json);

signals:
    void boardChanged(const QString& boardId);
    void pinConfigChanged(const QString& pin, PinMode mode);
    void taskPinConfigChanged(const QString& pin, const QString& task, PinMode mode);
    void taskPeripheralChanged(const QString& pin, const QString& task, const QString& peripheral);
    void connectionStateChanged(ConnectionState state, const QString& info);
    void adcValueChanged(const QString& pin, quint32 rawCount, int resolutionBits);
    void pwmDutyChanged(const QString& pin, double dutyPercent);
    void spiTransferCompleted(const QString& bus, quint8 byteOut, quint8 byteIn, bool success);
    void digitalStateChanged(const QString& pin, bool high);
    void connectionStatusChanged(bool connected, const QString& probeName);
    void probeDiscovered(const QString& boardName);
    void probeRemoved();
    void i2cScanCompleted(const QString& sclPin, const QString& sdaPin, const QList<quint8>& addresses);

private slots:
    void onPollTimer();

private:
    explicit HardwareBridge(QObject* parent = nullptr);
    ~HardwareBridge() override = default;

    void initializeDefaultBoards();
    void updateConnectionState(ConnectionState state, const QString& info = QString());

    QString m_currentBoardId = "stm32f030r8";
    bool m_writeOverride = false;
    QString m_activeTask = "Task A"; // "Task A", "Task B", "All Tasks"
    QMap<QString, BoardProfile> m_boards;

    // Active component bindings
    struct ComponentBinding {
        QPointer<UIComponent> component;
        QString pin;
        PinMode mode;
        QString task = "Task A";
    };
    QList<ComponentBinding> m_bindings;

    // Simulation cache
    QMap<QString, quint32> m_simulatedAdcValues;
    QMap<QString, double> m_pwmDutyValues;
    QMap<QString, bool> m_digitalStates;

    ConnectionState m_connectionState = ConnectionState::Disconnected;
    bool m_simulationMode = false;
    bool m_hardwareConnected = false;
    QString m_lastConnectionError;

    OpenOcdManager m_openOcd;
    QTimer m_pollTimer;
    QElapsedTimer m_pwmThrottleTimer;
    double m_lastPwmDutyWritten = -1.0;

    // R1.b: catalog-adapter probing state (see applyCatalogAdapter / matchSerialPort)
    QList<QPair<QPair<QString,QString>,QString>> m_testSerialPorts;
    bool m_testPortsActive = false;
    QString m_lastSerialPortName;
    void applyCatalogAdapter(BoardProfile& profile);
};
