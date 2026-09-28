#include "FlashController.h"
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>

FlashController::FlashController(DeviceManager* deviceManager, QObject* parent)
    : QObject(parent)
    , m_deviceManager(deviceManager)
    , m_process(new QProcess(this))
{
    setupProcess();
    m_openOcdPath = findOpenOcdExecutable();

    if (m_deviceManager) {
        setDeviceManager(m_deviceManager);
    }
}

FlashController::~FlashController() {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(500);
    }
}

void FlashController::setupProcess() {
    if (!m_process) {
        m_process = new QProcess(this);
    }

    // Route standard output and standard error signals to consoleOutputUpdate
    connect(m_process, &QProcess::readyReadStandardOutput, this, &FlashController::onProcessReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError, this, &FlashController::onProcessReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &FlashController::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &FlashController::onProcessErrorOccurred);
}

void FlashController::setDeviceManager(DeviceManager* deviceManager) {
    if (m_deviceManager && m_deviceManager != deviceManager) {
        disconnect(m_deviceManager, nullptr, this, nullptr);
    }

    m_deviceManager = deviceManager;
    if (m_deviceManager) {
        // Synchronize initial port
        setSelectedPort(m_deviceManager->selectedPort());

        // Listen for live port selection changes from DeviceManager
        connect(m_deviceManager, &DeviceManager::selectedPortChanged, this, [this](const QString& port) {
            setSelectedPort(port);
            emit consoleOutputUpdate(QString("[FlashController] Target port changed: %1\n").arg(port));
        });
    }
}

void FlashController::setSelectedPort(const QString& port) {
    if (m_selectedPort != port) {
        m_selectedPort = port;
        emit selectedPortChanged(m_selectedPort);
    }
}

void FlashController::setOpenOcdPath(const QString& path) {
    if (m_openOcdPath != path) {
        m_openOcdPath = path;
        emit openOcdPathChanged(m_openOcdPath);
    }
}

QString FlashController::findOpenOcdExecutable() const {
    QString exe = QStandardPaths::findExecutable("openocd");
    if (!exe.isEmpty()) return exe;

#ifdef Q_OS_WIN
    exe = QStandardPaths::findExecutable("openocd.exe");
    if (!exe.isEmpty()) return exe;
#endif

    return "openocd";
}

bool FlashController::flashSTM32(const QString& binaryPath) {
    if (m_isFlashing) {
        emit consoleOutputUpdate("[FlashController] Error: Another flashing session is currently in progress.\n");
        return false;
    }

    if (binaryPath.isEmpty()) {
        emit consoleOutputUpdate("[FlashController] Error: Binary file path cannot be empty.\n");
        return false;
    }

    QString normalizedBin = QDir::toNativeSeparators(binaryPath);
    emit consoleOutputUpdate(QString("[FlashController] Initiating STM32 flash via OpenOCD & ST-Link...\n"));
    emit consoleOutputUpdate(QString("[FlashController] Binary: %1\n").arg(normalizedBin));

    // Prompt 3 Requirement 2: OpenOCD with standard ST-Link and STM32 target parameters
    QStringList args;
    args << "-f" << "interface/stlink.cfg";
    args << "-f" << "target/stm32f4x.cfg";
    args << "-c" << QString("program {%1} verify reset exit 0x08000000").arg(normalizedBin);

    return executeOpenOcd(args);
}

bool FlashController::flashRISCV(const QString& binaryPath, const QString& adapterConfig) {
    if (m_isFlashing) {
        emit consoleOutputUpdate("[FlashController] Error: Another flashing session is currently in progress.\n");
        return false;
    }

    if (binaryPath.isEmpty()) {
        emit consoleOutputUpdate("[FlashController] Error: Binary file path cannot be empty.\n");
        return false;
    }

    QString normalizedBin = QDir::toNativeSeparators(binaryPath);

    // Default to standard generic FTDI JTAG adapter if none specified
    QString adapter = adapterConfig.trimmed().isEmpty() ? "interface/ftdi/minimodule.cfg" : adapterConfig.trimmed();

    emit consoleOutputUpdate(QString("[FlashController] Initiating RISC-V flash via OpenOCD & FTDI adapter...\n"));
    emit consoleOutputUpdate(QString("[FlashController] Adapter: %1\n").arg(adapter));
    emit consoleOutputUpdate(QString("[FlashController] Binary: %1\n").arg(normalizedBin));

    // Prompt 3 Requirement 3: OpenOCD targeting generic FTDI adapters and RISC-V cores
    QStringList args;
    args << "-f" << adapter;
    args << "-f" << "target/riscv.cfg";
    args << "-c" << QString("program {%1} verify reset exit").arg(normalizedBin);

    return executeOpenOcd(args);
}

bool FlashController::flashESP32(const QString& binaryPath) {
    if (m_isFlashing) {
        emit consoleOutputUpdate("[FlashController] Error: Another flashing session is currently in progress.\n");
        return false;
    }

    if (binaryPath.isEmpty()) {
        emit consoleOutputUpdate("[FlashController] Error: Binary file path cannot be empty.\n");
        return false;
    }

    QString normalizedBin = QDir::toNativeSeparators(binaryPath);
    emit consoleOutputUpdate(QString("[FlashController] Initiating ESP32 flash via esptool.py...\n"));
    emit consoleOutputUpdate(QString("[FlashController] Target Port: %1\n").arg(m_selectedPort.isEmpty() ? "Auto-detect" : m_selectedPort));
    emit consoleOutputUpdate(QString("[FlashController] Binary: %1\n").arg(normalizedBin));

    QString program = "esptool.py";
    QStringList args;
    if (!m_selectedPort.isEmpty()) {
        args << "-p" << m_selectedPort;
    }
    args << "-b" << "921600" << "write_flash" << "0x0" << normalizedBin;

    emit consoleOutputUpdate(QString("[FlashController] Executing command: %1 %2\n")
        .arg(program, args.join(" ")));

    m_isFlashing = true;
    emit isFlashingChanged(true);
    emit flashingStarted();

    m_process->start(program, args);

    if (!m_process->waitForStarted(2000)) {
        m_isFlashing = false;
        emit isFlashingChanged(false);
        emit consoleOutputUpdate(QString("[FlashController] Failed to start '%1': %2\n")
            .arg(program, m_process->errorString()));
        emit flashingFinished(false, -1);
        return false;
    }

    return true;
}

bool FlashController::flashDevice(const QString& boardType, const QString& binaryPath) {
    QString bt = boardType.toLower();
    if (bt.contains("esp32")) {
        return flashESP32(binaryPath);
    } else if (bt.contains("stm32")) {
        return flashSTM32(binaryPath);
    } else if (bt.contains("riscv") || bt.contains("risc-v") || bt.contains("ch32") || bt.contains("gd32v")) {
        return flashRISCV(binaryPath);
    } else {
        // Fallback default STM32
        return flashSTM32(binaryPath);
    }
}

bool FlashController::executeOpenOcd(const QStringList& arguments) {
    QString program = m_openOcdPath.isEmpty() ? "openocd" : m_openOcdPath;

    emit consoleOutputUpdate(QString("[FlashController] Executing command: %1 %2\n")
        .arg(program, arguments.join(" ")));

    m_isFlashing = true;
    emit isFlashingChanged(true);
    emit flashingStarted();

    // Asynchronous execution without blocking the Qt GUI thread
    m_process->start(program, arguments);

    if (!m_process->waitForStarted(2000)) {
        m_isFlashing = false;
        emit isFlashingChanged(false);
        emit consoleOutputUpdate(QString("[FlashController] Failed to start '%1': %2\n")
            .arg(program, m_process->errorString()));
        emit flashingFinished(false, -1);
        return false;
    }

    return true;
}

void FlashController::abort() {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        emit consoleOutputUpdate("[FlashController] Aborting flashing process...\n");
        m_process->terminate();
        if (!m_process->waitForFinished(1000)) {
            m_process->kill();
        }
    }
}

// Prompt 3 Requirement 4: Route readyReadStandardOutput and readyReadStandardError to consoleOutputUpdate(QString)
void FlashController::onProcessReadyReadStandardOutput() {
    if (!m_process) return;
    QByteArray data = m_process->readAllStandardOutput();
    if (!data.isEmpty()) {
        QString text = QString::fromLocal8Bit(data);
        emit consoleOutputUpdate(text);
    }
}

void FlashController::onProcessReadyReadStandardError() {
    if (!m_process) return;
    QByteArray data = m_process->readAllStandardError();
    if (!data.isEmpty()) {
        QString text = QString::fromLocal8Bit(data);
        emit consoleOutputUpdate(text);
    }
}

void FlashController::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_isFlashing = false;
    m_lastExitCode = exitCode;
    emit isFlashingChanged(false);

    bool success = (exitStatus == QProcess::NormalExit && exitCode == 0);
    if (success) {
        emit consoleOutputUpdate(QString("\n[FlashController] SUCCESS: Flashing completed with exit code %1.\n").arg(exitCode));
    } else {
        emit consoleOutputUpdate(QString("\n[FlashController] FAILED: Process finished with exit code %1 (status %2).\n")
            .arg(exitCode).arg(exitStatus == QProcess::CrashExit ? "Crashed" : "Normal"));
    }

    emit flashingFinished(success, exitCode);
}

void FlashController::onProcessErrorOccurred(QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) {
        m_isFlashing = false;
        emit isFlashingChanged(false);
        emit consoleOutputUpdate(QString("[FlashController] Error: Failed to launch OpenOCD binary. Ensure it is in PATH.\n"));
        emit flashingFinished(false, -1);
    }
}
