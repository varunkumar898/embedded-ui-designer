#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonObject>
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
    SpiRawTransfer
};

QString pinModeToString(PinMode mode);
PinMode stringToPinMode(const QString& str);

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

struct PinProfile {
    QString pinName;
    QList<PinMode> supportedModes;
    PinMode activeMode = PinMode::None;
    AdcMapping adc;
    PwmMapping pwm;
    SpiMapping spi;
};

#include "OpenOcdManager.h"

struct BoardProfile {
    QString id;
    QString name;
    QString mcuFamily;
    quint32 gpioBase = 0x48000000;
    quint32 bsrrOffset = 0x18;
    QString openocdInterface = "interface/stlink.cfg";
    QString openocdTarget = "target/stm32f0x.cfg";
    QMap<QString, PinProfile> pins;
    QStringList spiBusses;
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
    QString currentBoardId() const { return m_currentBoardId; }

    // Pin Capability & Mode Inspection
    QStringList availablePins() const;
    PinProfile pinProfile(const QString& pin) const;
    bool isModeAvailable(const QString& pin, PinMode mode) const;
    PinMode activePinMode(const QString& pin) const;
    bool setPinMode(const QString& pin, PinMode mode);

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

    // OpenOCD & Probe Subsystem
    OpenOcdManager& openOcdManager() { return m_openOcd; }
    const OpenOcdManager& openOcdManager() const { return m_openOcd; }
    QString connectedProbeName() const;
    bool isProbeDetected() const { return m_openOcd.isProbeDetected(); }
    QString detectedBoardName() const { return m_openOcd.detectedBoardName(); }
    bool connectHardware();
    bool disconnectHardware();

    // Simulation / Test overrides (when physical OpenOCD/ST-Link probe is disconnected)
    bool isHardwareConnected() const { return m_openOcd.isConnected() || m_hardwareConnected; }
    void setHardwareConnected(bool connected);
    void setSimulatedAdcCount(const QString& pin, quint32 rawCount);
    quint32 simulatedAdcCount(const QString& pin) const;

    // Polling control
    bool isLivePolling() const { return m_pollTimer.isActive(); }
    void startLivePolling(int intervalMs = 50);
    void stopLivePolling();

signals:
    void boardChanged(const QString& boardId);
    void pinConfigChanged(const QString& pin, PinMode mode);
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

    QString m_currentBoardId = "stm32f030r8";
    QMap<QString, BoardProfile> m_boards;

    // Active component bindings
    struct ComponentBinding {
        QPointer<UIComponent> component;
        QString pin;
        PinMode mode;
    };
    QList<ComponentBinding> m_bindings;

    // Simulation cache
    QMap<QString, quint32> m_simulatedAdcValues;
    QMap<QString, double> m_pwmDutyValues;
    QMap<QString, bool> m_digitalStates;

    bool m_hardwareConnected = false;
    OpenOcdManager m_openOcd;
    QTimer m_pollTimer;
    QElapsedTimer m_pwmThrottleTimer;
    double m_lastPwmDutyWritten = -1.0;
};
