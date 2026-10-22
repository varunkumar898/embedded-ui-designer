#include "BoardDetector.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#ifdef HAVE_QT_SERIALPORT
#include <QSerialPortInfo>
#endif

const std::map<QString, QString> BoardDetector::ADAPTER_VIDS = {
    {"ST-Link", "0483"},
    {"FTDI", "0403"},
    {"CP2104", "10c4"},
    {"J-Link", "1366"},
    {"CMSIS-DAP", "c251"}
};

BoardDetector::BoardDetector(QObject* parent)
    : QObject(parent)
{
    connect(&m_pollTimer, &QTimer::timeout, this, &BoardDetector::onPollTimer);
}

void BoardDetector::startPolling(int intervalMs) {
    m_pollTimer.setInterval(intervalMs);
    if (!m_pollTimer.isActive()) {
        m_pollTimer.start();
    }
    onPollTimer();
}

void BoardDetector::stopPolling() {
    m_pollTimer.stop();
}

void BoardDetector::setSimulatedBoard(const QString& boardId) {
    m_simulated = true;
    BoardSpec spec = BoardDatabase::instance().findBoardById(boardId);
    if (!spec.isValid()) {
        spec = BoardDatabase::instance().findBoardByName(boardId);
    }
    m_simulatedInfo.detected = true;
    m_simulatedInfo.boardId = spec.id;
    m_simulatedInfo.boardName = spec.name;
    m_simulatedInfo.mcu = spec.mcu;
    m_simulatedInfo.adapterType = spec.usbAdapterType;
    m_simulatedInfo.vid = spec.vid;
    m_simulatedInfo.pid = spec.pid;
    m_simulatedInfo.openocdConfig = spec.openocdConfigFile;
    m_simulatedInfo.suggestedInterface = spec.openocdInterface;
    m_simulatedInfo.spec = spec;

    m_lastDetected = m_simulatedInfo;
    emit boardDetected(m_simulatedInfo);
}

void BoardDetector::clearSimulatedBoard() {
    m_simulated = false;
    m_simulatedInfo = DetectedBoardInfo();
    m_lastDetected = DetectedBoardInfo();
    emit boardRemoved();
}

DetectedBoard BoardDetector::detectBoard() {
    if (m_simulated && m_simulatedInfo.detected) {
        DetectedBoard b;
        b.boardName = m_simulatedInfo.boardName;
        b.boardType = m_simulatedInfo.boardId.contains("esp32") ? "esp32" : (m_simulatedInfo.boardId.contains("pico") ? "rp2040" : "stm32");
        b.usbAdapter = m_simulatedInfo.adapterType;
        b.openOcdConfig = m_simulatedInfo.openocdConfig.isEmpty() ? getOpenOcdConfigForAdapter(b.usbAdapter, b.boardType) : m_simulatedInfo.openocdConfig;
        b.usbDevice.vendorId = m_simulatedInfo.vid;
        b.usbDevice.productId = m_simulatedInfo.pid;
        b.usbDevice.description = m_simulatedInfo.boardName;
        return b;
    }

    auto devices = getConnectedDevices();
    for (const auto& device : devices) {
        QString boardType = identifyBoardType(device);
        QString adapterType = identifyAdapterType(device);

        if (!boardType.isEmpty()) {
            DetectedBoard board;
            board.boardName = boardType;
            board.boardType = boardType;
            board.usbAdapter = adapterType;
            board.openOcdConfig = getOpenOcdConfigForAdapter(adapterType, boardType);
            board.usbDevice = device;
            return board;
        }
    }

    DetectedBoard nullBoard;
    nullBoard.boardName = "";
    return nullBoard;
}

std::vector<UsbDevice> BoardDetector::getConnectedDevices() {
#ifdef Q_OS_LINUX
    return detectUsbDevices_Linux();
#elif defined(Q_OS_WIN)
    return detectUsbDevices_Windows();
#elif defined(Q_OS_MAC)
    return detectUsbDevices_Mac();
#else
    return detectUsbDevices_Linux();
#endif
}

std::vector<UsbDevice> BoardDetector::detectUsbDevices_Linux() {
    std::vector<UsbDevice> devices;

    QProcess process;
    process.start("lsusb");
    if (process.waitForFinished(1000)) {
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        devices = parseLsusbOutput(output);
    }

    // Also fallback / supplement from sysfs if lsusb returned empty
    if (devices.empty()) {
        QDir usbDir("/sys/bus/usb/devices");
        if (usbDir.exists()) {
            const QStringList entries = usbDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& entry : entries) {
                QString devPath = usbDir.filePath(entry);
                QFile vidFile(devPath + "/idVendor");
                QFile pidFile(devPath + "/idProduct");

                if (vidFile.exists() && pidFile.exists()) {
                    if (vidFile.open(QIODevice::ReadOnly) && pidFile.open(QIODevice::ReadOnly)) {
                        UsbDevice dev;
                        dev.vendorId = QString::fromUtf8(vidFile.readAll()).trimmed().toLower();
                        dev.productId = QString::fromUtf8(pidFile.readAll()).trimmed().toLower();
                        vidFile.close();
                        pidFile.close();

                        QFile mfgFile(devPath + "/manufacturer");
                        if (mfgFile.open(QIODevice::ReadOnly)) dev.manufacturer = QString::fromUtf8(mfgFile.readAll()).trimmed();
                        QFile prodFile(devPath + "/product");
                        if (prodFile.open(QIODevice::ReadOnly)) dev.description = QString::fromUtf8(prodFile.readAll()).trimmed();
                        QFile serFile(devPath + "/serial");
                        if (serFile.open(QIODevice::ReadOnly)) dev.serialNumber = QString::fromUtf8(serFile.readAll()).trimmed();

                        if (!dev.vendorId.isEmpty() && !dev.productId.isEmpty()) {
                            devices.push_back(dev);
                        }
                    }
                }
            }
        }
    }

    return devices;
}

std::vector<UsbDevice> BoardDetector::parseLsusbOutput(const QString& output) {
    std::vector<UsbDevice> devices;
    QStringList lines = output.split('\n');

    for (const auto& line : lines) {
        if (line.isEmpty()) continue;

        QRegularExpression rx(R"(Bus\s+(\d+)\s+Device\s+(\d+):\s+ID\s+([0-9a-fA-F]{4}):([0-9a-fA-F]{4})\s*(.*))");
        auto match = rx.match(line);
        if (match.hasMatch()) {
            UsbDevice dev;
            dev.busNumber = match.captured(1).toInt();
            dev.deviceNumber = match.captured(2).toInt();
            dev.vendorId = match.captured(3).toLower();
            dev.productId = match.captured(4).toLower();
            dev.description = match.captured(5).trimmed();

            devices.push_back(dev);
        }
    }

    return devices;
}

std::vector<UsbDevice> BoardDetector::detectUsbDevices_Windows() {
#ifdef HAVE_QT_SERIALPORT
    // Portable USB-serial enumeration: QSerialPortInfo reports VID:PID and
    // description for the adapters we own the catalog rows for (CP210x, CH340,
    // ST-Link, native USB). Works identically on Windows and macOS.
    std::vector<UsbDevice> devices;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports) {
        if (!info.hasVendorIdentifier() || !info.hasProductIdentifier()) continue;
        UsbDevice dev;
        dev.vendorId = QString::number(info.vendorIdentifier(), 16).rightJustified(4, u'0');
        dev.productId = QString::number(info.productIdentifier(), 16).rightJustified(4, u'0');
        dev.manufacturer = info.manufacturer();
        dev.description = info.description().isEmpty() ? QString("USB serial device") : info.description();
        dev.serialNumber = info.serialNumber();
        devices.push_back(dev);
    }
    return devices;
#else
    return {};
#endif
}

std::vector<UsbDevice> BoardDetector::detectUsbDevices_Mac() {
    return detectUsbDevices_Windows(); // identical portable QSerialPortInfo path
}

QString BoardDetector::identifyBoardType(const UsbDevice& device) {
    QString vid = device.vendorId.toLower();
    QString pid = device.productId.toLower();

    // Check ST-Link (STM32)
    if (vid == "0483" && (pid == "3748" || pid == "374b" || device.description.contains("ST-Link", Qt::CaseInsensitive))) {
        return "stm32";
    }

    // Check CP2104 (ESP32)
    if (vid == "10c4" && (pid == "ea60" || device.description.contains("CP210", Qt::CaseInsensitive))) {
        return "esp32";
    }

    // Check FTDI (ESP32 or other)
    if (vid == "0403" && (pid == "6010" || pid == "6011" || pid == "6001")) {
        return "esp32";
    }

    // Check J-Link
    if (vid == "1366") {
        return "stm32";
    }

    // Check RP2040
    if (vid == "2e8a" || device.description.contains("RP2040", Qt::CaseInsensitive) || device.description.contains("Pico", Qt::CaseInsensitive)) {
        return "rp2040";
    }

    // Check ESP32-S3 USB JTAG
    if (vid == "303a") {
        return "esp32s3";
    }

    // Check CMSIS-DAP
    if (vid == "c251" || vid == "0d28") {
        return "stm32";
    }

    return "";
}

QString BoardDetector::identifyAdapterType(const UsbDevice& device) {
    QString vid = device.vendorId.toLower();

    if (vid == "0483") return "ST-Link";
    if (vid == "0403") return "FTDI";
    if (vid == "10c4") return "CP2104";
    if (vid == "1366") return "J-Link";
    if (vid == "c251" || vid == "2e8a" || vid == "0d28") return "CMSIS-DAP";

    return "Unknown";
}

QString BoardDetector::getOpenOcdConfigForAdapter(const QString& adapterType, const QString& boardType) {
    if (boardType == "esp32") {
        return "target/esp32.cfg";
    }
    if (boardType == "esp32s3") {
        return "target/esp32s3.cfg";
    }
    if (boardType == "stm32") {
        return "target/stm32f0x.cfg";
    }
    if (boardType == "rp2040") {
        return "target/rp2040.cfg";
    }

    if (adapterType == "ST-Link") {
        return "interface/stlink.cfg";
    }
    if (adapterType == "FTDI") {
        return "interface/ftdi/olimex-arm-usb-ocd.cfg";
    }

    return "";
}

DetectedBoardInfo BoardDetector::detectConnectedBoard() {
    if (m_simulated) {
        return m_simulatedInfo;
    }

#if defined(Q_OS_LINUX)
    DetectedBoardInfo info = scanUsbSysfs();
    if (info.detected) {
        return info;
    }

    return scanLsusb();
#else
    // Portable path (macOS/Windows/other): drive the shared per-OS dispatcher
    // and match the adapter against the catalog the same way Linux does.
    const auto devices = getConnectedDevices();
    for (const auto& dev : devices) {
        const BoardSpec spec = BoardDatabase::instance().findBoardByUsb(
            dev.vendorId.toLower(), dev.productId.toLower());
        if (spec.isValid() && !spec.vid.isEmpty()) {
            DetectedBoardInfo info;
            info.detected = true;
            info.boardId = spec.id;
            info.boardName = spec.name;
            info.mcu = spec.mcu;
            info.adapterType = spec.usbAdapterType;
            info.vid = dev.vendorId.toLower();
            info.pid = dev.productId.toLower();
            info.serial = dev.serialNumber;
            info.openocdConfig = spec.openocdConfigFile;
            info.suggestedInterface = spec.openocdInterface;
            info.spec = spec;
            return info;
        }
        // R1.c: a VID-only match is an ADAPTER identification, never a
        // concrete board identity (any 0483 ST-Link is not "NUCLEO-F030R8").
        if (dev.vendorId.toLower() == "0483" ||
            dev.description.contains("ST-LINK", Qt::CaseInsensitive)) {
            DetectedBoardInfo adapterOnly;
            adapterOnly.detected = false; // board unknown until the target is identified
            adapterOnly.adapterType = "ST-Link";
            adapterOnly.vid = dev.vendorId.toLower();
            adapterOnly.pid = dev.productId.toLower();
            adapterOnly.serial = dev.serialNumber;
            return adapterOnly;
        }
    }
    return DetectedBoardInfo();
#endif
}

DetectedBoardInfo BoardDetector::scanUsbSysfs() {
    DetectedBoardInfo info;
    QDir usbDir("/sys/bus/usb/devices");
    if (!usbDir.exists()) {
        return info;
    }

    const QStringList entries = usbDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& entry : entries) {
        QString devPath = usbDir.filePath(entry);
        QFile vidFile(devPath + "/idVendor");
        QFile pidFile(devPath + "/idProduct");

        if (vidFile.exists() && pidFile.exists()) {
            if (vidFile.open(QIODevice::ReadOnly) && pidFile.open(QIODevice::ReadOnly)) {
                QString vid = QString::fromUtf8(vidFile.readAll()).trimmed().toLower();
                QString pid = QString::fromUtf8(pidFile.readAll()).trimmed().toLower();
                vidFile.close();
                pidFile.close();

                QString mfg, prod, serial;
                QFile mfgFile(devPath + "/manufacturer");
                if (mfgFile.open(QIODevice::ReadOnly)) mfg = QString::fromUtf8(mfgFile.readAll()).trimmed();
                QFile prodFile(devPath + "/product");
                if (prodFile.open(QIODevice::ReadOnly)) prod = QString::fromUtf8(prodFile.readAll()).trimmed();
                QFile serFile(devPath + "/serial");
                if (serFile.open(QIODevice::ReadOnly)) serial = QString::fromUtf8(serFile.readAll()).trimmed();

                BoardSpec spec = BoardDatabase::instance().findBoardByUsb(vid, pid);
                if (spec.isValid()) {
                    info.detected = true;
                    info.boardId = spec.id;
                    info.boardName = spec.name;
                    info.mcu = spec.mcu;
                    info.adapterType = spec.usbAdapterType;
                    info.vid = vid;
                    info.pid = pid;
                    info.serial = serial;
                    info.openocdConfig = spec.openocdConfigFile;
                    info.suggestedInterface = spec.openocdInterface;
                    info.spec = spec;
                    return info;
                }

                if (vid == "10c4") {
                    info.detected = true;
                    info.boardId = "esp32_devkitv1_cp2104";
                    info.boardName = "ESP32 DEVKIT V1 (CP2104)";
                    info.mcu = "ESP32";
                    info.adapterType = "Silicon Labs CP210x USB-to-UART";
                    info.vid = vid;
                    info.pid = pid;
                    info.serial = serial;
                    info.openocdConfig = "config/openocd/esp32-devkitv1-cp210x.cfg";
                    info.suggestedInterface = "interface/ftdi/esp32_devkitj_v1.cfg";
                    info.spec = BoardDatabase::instance().findBoardById(info.boardId);
                    return info;
                }

                if (vid == "0483") {
                    info.detected = true;
                    info.boardId = "nucleo_f030r8";
                    info.boardName = "STM32 NUCLEO-F030R8";
                    info.mcu = "STM32F030R8";
                    info.adapterType = "ST-Link/V2-1";
                    info.vid = vid;
                    info.pid = pid;
                    info.serial = serial;
                    info.openocdConfig = "config/openocd/stm32f030r8.cfg";
                    info.suggestedInterface = "interface/stlink.cfg";
                    info.spec = BoardDatabase::instance().findBoardById(info.boardId);
                    return info;
                }

                if (vid == "303a") {
                    info.detected = true;
                    info.boardId = "esp32_s3_devkitc";
                    info.boardName = "ESP32-S3-DevKitC-1";
                    info.mcu = "ESP32-S3";
                    info.adapterType = "Built-in ESP USB Serial/JTAG";
                    info.vid = vid;
                    info.pid = pid;
                    info.serial = serial;
                    info.openocdConfig = "board/esp32s3-builtin.cfg";
                    info.suggestedInterface = "interface/esp_usb_jtag.cfg";
                    info.spec = BoardDatabase::instance().findBoardById(info.boardId);
                    return info;
                }

                if (vid == "2e8a") {
                    info.detected = true;
                    info.boardId = "rpi_pico_rp2040";
                    info.boardName = "Raspberry Pi Pico (RP2040)";
                    info.mcu = "RP2040";
                    info.adapterType = "Raspberry Pi Debug Probe (CMSIS-DAP)";
                    info.vid = vid;
                    info.pid = pid;
                    info.serial = serial;
                    info.openocdConfig = "target/rp2040.cfg";
                    info.suggestedInterface = "interface/cmsis-dap.cfg";
                    info.spec = BoardDatabase::instance().findBoardById(info.boardId);
                    return info;
                }
            }
        }
    }

    return info;
}

DetectedBoardInfo BoardDetector::scanLsusb() {
    DetectedBoardInfo info;
#if defined(Q_OS_LINUX)
    QProcess proc;
    proc.start("lsusb");
    if (!proc.waitForFinished(800)) {
        proc.kill();
        return info;
    }

    QString out = QString::fromUtf8(proc.readAllStandardOutput());
    if (out.contains("0483:374b") || out.contains("0483:3748") || out.contains("ST-LINK", Qt::CaseInsensitive)) {
        info.detected = true;
        info.boardId = "nucleo_f030r8";
        info.boardName = "STM32 NUCLEO-F030R8";
        info.mcu = "STM32F030R8";
        info.adapterType = "ST-Link/V2-1";
        info.vid = "0483";
        info.pid = "374b";
        info.openocdConfig = "config/openocd/stm32f030r8.cfg";
        info.suggestedInterface = "interface/stlink.cfg";
        info.spec = BoardDatabase::instance().findBoardById(info.boardId);
        return info;
    }

    if (out.contains("10c4:ea60") || out.contains("CP210", Qt::CaseInsensitive)) {
        info.detected = true;
        info.boardId = "esp32_devkitv1_cp2104";
        info.boardName = "ESP32 DEVKIT V1 (CP2104)";
        info.mcu = "ESP32";
        info.adapterType = "Silicon Labs CP210x";
        info.vid = "10c4";
        info.pid = "ea60";
        info.openocdConfig = "config/openocd/esp32-devkitv1-cp210x.cfg";
        info.suggestedInterface = "interface/ftdi/esp32_devkitj_v1.cfg";
        info.spec = BoardDatabase::instance().findBoardById(info.boardId);
        return info;
    }

    if (out.contains("303a:1001") || out.contains("Espressif", Qt::CaseInsensitive)) {
        info.detected = true;
        info.boardId = "esp32_s3_devkitc";
        info.boardName = "ESP32-S3-DevKitC-1";
        info.mcu = "ESP32-S3";
        info.adapterType = "Built-in ESP USB Serial/JTAG";
        info.vid = "303a";
        info.pid = "1001";
        info.openocdConfig = "board/esp32s3-builtin.cfg";
        info.suggestedInterface = "interface/esp_usb_jtag.cfg";
        info.spec = BoardDatabase::instance().findBoardById(info.boardId);
        return info;
    }
#endif
    return info;
}

void BoardDetector::onPollTimer() {
    DetectedBoardInfo current = detectConnectedBoard();
    if (current.detected && !m_lastDetected.detected) {
        m_lastDetected = current;
        emit boardDetected(current);
    } else if (current.detected && m_lastDetected.detected) {
        if (current.boardId != m_lastDetected.boardId || current.vid != m_lastDetected.vid) {
            m_lastDetected = current;
            emit boardDetected(current);
        }
    } else if (!current.detected && m_lastDetected.detected) {
        m_lastDetected = DetectedBoardInfo();
        emit boardRemoved();
    }
}

QString BoardDetector::resolveOpenOCDConfig(const QString& boardIdOrName) {
    BoardSpec spec = BoardDatabase::instance().findBoardById(boardIdOrName);
    if (!spec.isValid()) {
        spec = BoardDatabase::instance().findBoardByName(boardIdOrName);
    }
    if (spec.isValid() && !spec.openocdConfigFile.isEmpty()) {
        return spec.openocdConfigFile;
    }
    return "config/openocd/stm32f030r8.cfg";
}
