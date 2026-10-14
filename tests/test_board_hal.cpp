#include <QCoreApplication>
#include <QDebug>
#include <cassert>
#include <iostream>

#include "hardware/BoardHAL.h"
#include "hardware/STM32HAL.h"
#include "hardware/ESP32HAL.h"
#include "hardware/BoardFactory.h"
#include "hardware/OpenOCDManager.h"
#include "hardware/OpenOCDWorker.h"
#include <QFile>
#include <QThread>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    std::cout << "=======================================================\n";
    std::cout << "TEST: Multi-Board Hardware Abstraction Layer (HAL)\n";
    std::cout << "=======================================================\n";

    // -------------------------------------------------------------------------
    // Test 1: STM32F030R8 HAL Verification
    // -------------------------------------------------------------------------
    std::cout << "[Test 1] Testing STM32F030R8 HAL...\n";
    STM32HAL stm32;
    assert(stm32.getBoardName() == "STM32F030R8");
    assert(stm32.getManufacturer() == "STMicroelectronics");
    assert(stm32.getRegisterOffset_BSRR() == 0x18);
    assert(stm32.getRegisterOffset_BRR() == 0x1C);
    assert(stm32.getRegisterOffset_IDR() == 0x10);

    // Base addresses (0x48000000 + port * 0x400)
    assert(stm32.getGPIOBaseAddress(0) == 0x48000000); // PA
    assert(stm32.getGPIOBaseAddress(1) == 0x48000400); // PB
    assert(stm32.getGPIOBaseAddress(5) == 0x48001400); // PF

    // Ports
    assert(stm32.isValidPort(0));
    assert(stm32.isValidPort(5));
    assert(!stm32.isValidPort(6)); // PG not supported on F030R8

    // Pins
    assert(stm32.isValidPin(0, 4));
    assert(stm32.isValidPin(1, 12));
    assert(!stm32.isValidPin(0, 16)); // >15 invalid

    // Quirks / Hardware Limitations: SWD PA13/PA14 must be rejected
    assert(!stm32.isValidPin(0, 13)); // SWDIO
    assert(!stm32.isValidPin(0, 14)); // SWCLK
    assert(stm32.isValidPin(1, 13));  // PB13 is valid

    // Bitmask
    assert(stm32.getBitMaskForPin(4) == (1U << 4));
    assert(stm32.getBitMaskForPin(12) == (1U << 12));

    // String conversion
    assert(stm32.pinToString(0, 4) == "PA4");
    assert(stm32.pinToString(1, 12) == "PB12");
    assert(stm32.pinToString(0, 13) == "INVALID");

    std::cout << "  STM32F030R8 HAL verified successfully.\n";

    // -------------------------------------------------------------------------
    // Test 2: ESP32 HAL Verification
    // -------------------------------------------------------------------------
    std::cout << "[Test 2] Testing ESP32 HAL...\n";
    ESP32HAL esp32;
    assert(esp32.getBoardName() == "ESP32");
    assert(esp32.getManufacturer() == "Espressif");
    assert(esp32.getRegisterOffset_BSRR() == 0x08); // GPIO_OUT_W1TS
    assert(esp32.getRegisterOffset_BRR() == 0x0C);  // GPIO_OUT_W1TC
    assert(esp32.getRegisterOffset_IDR() == 0x3C);  // GPIO_IN
    assert(esp32.getGPIOBaseAddress(0) == 0x3FF44000);
    assert(esp32.getGPIOBaseAddress(1) == 0x3FF44000);

    // Virtual ports
    assert(esp32.isValidPort(0));
    assert(esp32.isValidPort(1));
    assert(esp32.isValidPort(2));
    assert(!esp32.isValidPort(3));

    // Pins & Hardware Limitations:
    // Port 0: GPIO0-15. GPIO6-11 (SPI Flash) must be rejected
    assert(esp32.isValidPin(0, 0));
    assert(esp32.isValidPin(0, 4));
    assert(!esp32.isValidPin(0, 6));  // Flash
    assert(!esp32.isValidPin(0, 11)); // Flash
    assert(esp32.isValidPin(0, 12));

    // Port 1: GPIO16-31. GPIO28-31 (pins 12-15) must be rejected
    assert(esp32.isValidPin(1, 2));   // GPIO18
    assert(!esp32.isValidPin(1, 12)); // GPIO28 reserved

    // Port 2: GPIO32-39. pins 0-7 valid
    assert(esp32.isValidPin(2, 2));   // GPIO34
    assert(!esp32.isValidPin(2, 8));

    // String conversion
    assert(esp32.pinToString(1, 2) == "GPIO18");
    assert(esp32.pinToString(0, 4) == "GPIO4");
    assert(esp32.pinToString(0, 6) == "INVALID");

    std::cout << "  ESP32 HAL verified successfully.\n";

    // -------------------------------------------------------------------------
    // Test 3: Auto-Detection & BoardFactory
    // -------------------------------------------------------------------------
    std::cout << "[Test 3] Testing Auto-Detection & BoardFactory...\n";
    assert(BoardFactory::detectBoardFromOpenOCD("stm32f0x.cpu") == "STM32F030R8");
    assert(BoardFactory::detectBoardFromOpenOCD("Info : stm32f0x.cpu: hardware has 4 breakpoints") == "STM32F030R8");
    assert(BoardFactory::detectBoardFromOpenOCD("esp32.cpu") == "ESP32");
    assert(BoardFactory::detectBoardFromOpenOCD("esp32s3.cpu") == "ESP32-S3");
    assert(BoardFactory::detectBoardFromOpenOCD("rp2040.core0") == "RP2040");
    assert(BoardFactory::detectBoardFromOpenOCD("unknown_target_string") == "UNKNOWN");

    assert(BoardFactory::isSupported("STM32F030R8"));
    assert(BoardFactory::isSupported("ESP32"));
    assert(BoardFactory::isSupported("ESP32-S3"));
    assert(!BoardFactory::isSupported("UNKNOWN"));

    auto stm32FromFactory = BoardFactory::createHAL("STM32F030R8");
    assert(stm32FromFactory != nullptr);
    assert(stm32FromFactory->getBoardName() == "STM32F030R8");

    auto esp32FromFactory = BoardFactory::createHAL("ESP32");
    assert(esp32FromFactory != nullptr);
    assert(esp32FromFactory->getBoardName() == "ESP32");

    auto unknownHal = BoardFactory::createHAL("UNKNOWN");
    assert(unknownHal == nullptr);

    std::cout << "  BoardFactory and auto-detection verified successfully.\n";

    // -------------------------------------------------------------------------
    // Test 4: Pin String Parsing
    // -------------------------------------------------------------------------
    std::cout << "[Test 4] Testing Pin String Parsing...\n";
    uint8_t port = 0, pin = 0;

    // STM32 parsing
    assert(stm32.parsePinString("PA4", port, pin) && port == 0 && pin == 4);
    assert(stm32.parsePinString("PB12", port, pin) && port == 1 && pin == 12);
    assert(!stm32.parsePinString("PA13", port, pin)); // Rejected (SWD)
    assert(!stm32.parsePinString("PA14", port, pin)); // Rejected (SWD)
    assert(!stm32.parsePinString("PG1", port, pin));  // Invalid port
    assert(!stm32.parsePinString("PA16", port, pin)); // Invalid pin
    assert(!stm32.parsePinString("INVALID", port, pin));

    // ESP32 parsing
    assert(esp32.parsePinString("GPIO18", port, pin) && port == 1 && pin == 2);
    assert(esp32.parsePinString("18", port, pin) && port == 1 && pin == 2);
    assert(esp32.parsePinString("GPIO4", port, pin) && port == 0 && pin == 4);
    assert(esp32.parsePinString("4", port, pin) && port == 0 && pin == 4);
    assert(!esp32.parsePinString("GPIO6", port, pin));  // Rejected (Flash)
    assert(!esp32.parsePinString("GPIO11", port, pin)); // Rejected (Flash)
    assert(!esp32.parsePinString("GPIO28", port, pin)); // Rejected (Reserved)
    assert(!esp32.parsePinString("GPIO45", port, pin)); // Invalid pin number
    assert(!esp32.parsePinString("INVALID", port, pin));

    std::cout << "  Pin string parsing verified successfully.\n";

    // -------------------------------------------------------------------------
    // Test 5: OpenOCDManager HAL Integration & Rapid Toggling
    // -------------------------------------------------------------------------
    std::cout << "[Test 5] Testing OpenOCDManager HAL Integration & Rapid Toggling...\n";
    OpenOCDManager openocd;
    assert(openocd.getBoardHAL() != nullptr);
    assert(openocd.getDetectedBoardName() == "STM32F030R8");

    // Test rapid toggles (50+) on STM32
    for (int i = 0; i < 60; ++i) {
        bool ok = openocd.gpioToggle(0, 4); // PA4
        assert(ok);
        int state = openocd.gpioRead(0, 4);
        assert(state == (i % 2 == 0 ? 1 : 0));
    }
    std::cout << "  STM32 60 rapid toggles succeeded without error.\n";

    // Switch to ESP32 board
    bool setEspOk = openocd.setBoard("ESP32");
    assert(setEspOk);
    assert(openocd.getBoardHAL() != nullptr);
    assert(openocd.getBoardHAL()->getBoardName() == "ESP32");

    // Test rapid toggles (50+) on ESP32 (GPIO18 -> port 1, pin 2)
    for (int i = 0; i < 60; ++i) {
        bool ok = openocd.gpioToggle(1, 2); // GPIO18
        assert(ok);
        int state = openocd.gpioRead(1, 2);
        assert(state == (i % 2 == 0 ? 1 : 0));
    }
    std::cout << "  ESP32 60 rapid toggles succeeded without error.\n";

    // Verify invalid pin rejection in OpenOCDManager
    assert(!openocd.gpioToggle(0, 6)); // GPIO6 on ESP32 is Flash reserved
    assert(openocd.gpioRead(0, 6) == -1);

    // -------------------------------------------------------------------------
    // Test 6: OpenOCD Config Paths & Embedded Resources Verification
    // -------------------------------------------------------------------------
    std::cout << "[Test 6] Testing OpenOCD Config Paths & Embedded Resources...\n";
    assert(BoardFactory::getOpenOCDConfigPath("ESP32-DEVKITV1") == ":/config/openocd/esp32-devkitv1.cfg");
    assert(BoardFactory::getOpenOCDConfigPath("ESP32") == ":/config/openocd/esp32-devkitv1.cfg");
    assert(BoardFactory::getOpenOCDConfigPath("ESP32-DEVKITV1-CP210X") == ":/config/openocd/esp32-devkitv1-cp210x.cfg");
    assert(BoardFactory::getOpenOCDConfigPath("ESP32-DEVKITV1-USB") == ":/config/openocd/esp32-devkitv1-usb.cfg");
    assert(BoardFactory::getOpenOCDConfigPath("STM32F030R8") == ":/config/openocd/stm32f030r8.cfg");
    assert(BoardFactory::getOpenOCDConfigPath("UNKNOWN") == "");

    QFile espCfg(":/config/openocd/esp32-devkitv1.cfg");
    assert(espCfg.open(QIODevice::ReadOnly));
    QString espContent = QString::fromUtf8(espCfg.readAll());
    assert(espContent.contains("ESP32 DEVKIT V1"));
    espCfg.close();

    QFile espCp210xCfg(":/config/openocd/esp32-devkitv1-cp210x.cfg");
    assert(espCp210xCfg.open(QIODevice::ReadOnly));
    QString cp210xContent = QString::fromUtf8(espCp210xCfg.readAll());
    assert(cp210xContent.contains("CP210x"));
    espCp210xCfg.close();

    QFile espUsbCfg(":/config/openocd/esp32-devkitv1-usb.cfg");
    assert(espUsbCfg.open(QIODevice::ReadOnly));
    QString espUsbContent = QString::fromUtf8(espUsbCfg.readAll());
    assert(espUsbContent.contains("SiLabs CP2104") || espUsbContent.contains("ESP32 JTAG"));
    espUsbCfg.close();

    QFile stmCfg(":/config/openocd/stm32f030r8.cfg");
    assert(stmCfg.open(QIODevice::ReadOnly));
    QString stmContent = QString::fromUtf8(stmCfg.readAll());
    assert(stmContent.contains("stm32f0x.cfg"));
    stmCfg.close();
    std::cout << "  OpenOCD config files verified in Qt resource system.\n";

    // -------------------------------------------------------------------------
    // Test 7: OpenOCDWorker Background Threading, Timeout & Cancellation (No ANR)
    // -------------------------------------------------------------------------
    std::cout << "[Test 7] Testing OpenOCDWorker Background Thread, Timeout & Cancel...\n";
    {
        // 7a. Test user cancellation
        OpenOCDManager testMgr;
        OpenOCDWorker worker(&testMgr);
        bool failedEmitted = false;
        QString failureReason;
        QObject::connect(&worker, &OpenOCDWorker::connectionFailed, [&](const QString& reason) {
            failedEmitted = true;
            failureReason = reason;
        });

        worker.connectToOpenOCD("127.0.0.1", 4444, 2);
        worker.cancelConnection();
        QCoreApplication::processEvents();

        assert(failedEmitted);
        assert(failureReason.contains("cancelled", Qt::CaseInsensitive));
        std::cout << "  Cancellation handled promptly.\n";
    }

    {
        // 7b. Test timeout on unused port without hanging the thread
        OpenOCDManager testMgr;
        QThread workerThread;
        OpenOCDWorker worker(&testMgr);
        worker.moveToThread(&workerThread);
        workerThread.start();

        bool timeoutFailed = false;
        QString timeoutMsg;
        int progressCount = 0;
        QObject::connect(&worker, &OpenOCDWorker::connectionFailed, [&](const QString& reason) {
            timeoutFailed = true;
            timeoutMsg = reason;
        });
        QObject::connect(&worker, &OpenOCDWorker::progressUpdated, [&](const QString&, int) {
            progressCount++;
        });

        // Use 1 second timeout on port 65432 (nothing listening)
        QMetaObject::invokeMethod(&worker, [&]() {
            worker.connectToOpenOCD("127.0.0.1", 65432, 1);
        }, Qt::QueuedConnection);

        // Wait up to 3 seconds for timeout
        auto startWait = std::chrono::steady_clock::now();
        while (!timeoutFailed && 
               std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startWait).count() < 3) {
            QCoreApplication::processEvents();
            QThread::msleep(50);
        }

        assert(timeoutFailed);
        assert(timeoutMsg.contains("timeout", Qt::CaseInsensitive));
        assert(progressCount > 0);

        workerThread.quit();
        workerThread.wait(1000);
        std::cout << "  Connection timeout handled in background without freezing main loop.\n";
    }

    std::cout << "=======================================================\n";
    std::cout << "ALL MULTI-BOARD HAL & WORKER TESTS PASSED! (100%)\n";
    std::cout << "=======================================================\n";
    return 0;
}
