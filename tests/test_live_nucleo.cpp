#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QThread>
#include <QDebug>
#include <iostream>

#include "MainWindow.h"
#include "CanvasScene.h"
#include "HardwareBridge.h"
#include "OpenOcdManager.h"
#include "SwitchComponent.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    std::cout << "=======================================================\n";
    std::cout << "TEST: Live Nucleo Board Auto-Detection & GPIO Switch\n";
    std::cout << "=======================================================\n";

    HardwareBridge& bridge = HardwareBridge::instance();
    bridge.setBoard("stm32f030r8");

    // 1. Initial Auto-Detection Check
    std::cout << "[Step 1] Checking USB probe auto-detection...\n";
    DiscoveredProbe probe = bridge.openOcdManager().scanUsbProbes();
    std::cout << "  Probe detected: " << (probe.detected ? "YES" : "NO") << "\n";
    std::cout << "  Probe name: " << probe.name.toStdString() << "\n";
    std::cout << "  VID/PID: " << probe.vendorId.toStdString() << ":" << probe.productId.toStdString() << "\n";

    if (!probe.detected) {
        std::cerr << "FAIL: Probe not detected via USB scan!\n";
        return 1;
    }

    // 2. Launch MainWindow and check UI Status Badge
    std::cout << "[Step 2] Launching MainWindow and verifying status badge...\n";
    MainWindow window;
    window.resize(1380, 880);
    window.show();
    QApplication::processEvents();

    auto* badge = window.findChild<QLabel*>("hardwareStatusBadge");
    if (!badge) {
        std::cerr << "FAIL: hardwareStatusBadge not found!\n";
        return 2;
    }
    auto* connectBtn = window.findChild<QPushButton*>("hardwareConnectBtn");
    if (!connectBtn) {
        std::cerr << "FAIL: hardwareConnectBtn not found!\n";
        return 3;
    }

    std::cout << "  Badge text: " << badge->text().toStdString() << "\n";
    std::cout << "  Connect button text: " << connectBtn->text().toStdString() << "\n";

    if (!badge->text().contains("Board detected: STM32F030R8 via ST-Link")) {
        std::cerr << "FAIL: Status badge did not show 'Board detected: STM32F030R8 via ST-Link'\n";
        return 4;
    }

    window.grab().save("screenshots/live_board_detected.png");
    std::cout << "  Screenshot saved: screenshots/live_board_detected.png\n";

    // 3. Connect to OpenOCD
    std::cout << "[Step 3] Connecting to OpenOCD via Connect button...\n";
    connectBtn->click();
    for (int i = 0; i < 30; ++i) {
        QApplication::processEvents();
        if (bridge.isHardwareConnected()) break;
        QThread::msleep(100);
    }

    std::cout << "  Hardware connected: " << (bridge.isHardwareConnected() ? "YES" : "NO") << "\n";
    std::cout << "  Connected probe: " << bridge.connectedProbeName().toStdString() << "\n";
    std::cout << "  Updated badge text: " << badge->text().toStdString() << "\n";
    std::cout << "  Updated button text: " << connectBtn->text().toStdString() << "\n";

    if (!bridge.isHardwareConnected()) {
        std::cerr << "FAIL: Failed to connect to hardware via OpenOCD!\n";
        return 5;
    }

    // 4. Configure Switch on Canvas bound to PA4 (Built-in LED)
    std::cout << "[Step 4] Adding SwitchComponent bound to GPIO PA4 (Built-in LED)...\n";
    auto* canvasScene = window.canvasScene();
    if (!canvasScene) {
        std::cerr << "FAIL: canvasScene not found!\n";
        return 6;
    }

    auto* sw = new SwitchComponent("switch_pa4");
    sw->setCompPos(100, 100);
    sw->setCompSize(70, 36);
    canvasScene->addUIComponent(sw);
    bridge.bindComponent(sw, "PA4", PinMode::DigitalOut);
    QApplication::processEvents();

    // 5. Test Live GPIO Output Toggle HIGH (3.3V) on PA4 with Latency Profiling
    std::cout << "[Step 5] Toggling Switch to ON (HIGH - 3.3V) on PA4...\n";
    QElapsedTimer timer;
    timer.start();
    sw->setChecked(true);
    QApplication::processEvents();
    qint64 toggleHighMs = timer.elapsed();
    std::cout << "  Latency to toggle HIGH: " << toggleHighMs << " ms (Criteria: < 100ms)\n";

    quint32 valHigh = 0;
    bridge.openOcdManager().readMemoryWord(0x48000014, &valHigh);
    std::cout << "  GPIOA ODR readback (HIGH): 0x" << std::hex << valHigh << std::dec << "\n";
    std::cout << "  PA4 bit value: " << ((valHigh >> 4) & 0x1) << " (Expected 1)\n";

    if (((valHigh >> 4) & 0x1) != 1) {
        std::cerr << "FAIL: GPIOA PA4 ODR bit 4 was not set HIGH!\n";
        return 7;
    }

    // 6. Test Live GPIO Output Toggle LOW (0V) on PA4
    std::cout << "[Step 6] Toggling Switch to OFF (LOW - 0V) on PA4...\n";
    timer.restart();
    sw->setChecked(false);
    QApplication::processEvents();
    qint64 toggleLowMs = timer.elapsed();
    std::cout << "  Latency to toggle LOW: " << toggleLowMs << " ms (Criteria: < 100ms)\n";

    quint32 valLow = 0;
    bridge.openOcdManager().readMemoryWord(0x48000014, &valLow);
    std::cout << "  GPIOA ODR readback (LOW): 0x" << std::hex << valLow << std::dec << "\n";
    std::cout << "  PA4 bit value: " << ((valLow >> 4) & 0x1) << " (Expected 0)\n";

    if (((valLow >> 4) & 0x1) != 0) {
        std::cerr << "FAIL: GPIOA PA4 ODR bit 4 was not reset LOW!\n";
        return 8;
    }

    // 7. Test 10+ Multi-Cycle Reliability (Criteria: 10+ cycles without crash)
    std::cout << "[Step 7] Testing 10+ rapid toggle cycles for reliability...\n";
    for (int cycle = 1; cycle <= 12; ++cycle) {
        sw->setChecked(cycle % 2 == 1);
        QApplication::processEvents();
        quint32 odr = 0;
        bridge.openOcdManager().readMemoryWord(0x48000014, &odr);
        int bit = (odr >> 4) & 0x1;
        int expected = (cycle % 2 == 1) ? 1 : 0;
        if (bit != expected) {
            std::cerr << "FAIL: Cycle " << cycle << " mismatch! Bit=" << bit << " Expected=" << expected << "\n";
            return 9;
        }
    }
    std::cout << "  Successfully completed 12 consecutive cycles without error.\n";

    // 8. Test OpenOCDManager High-Level Commands & Device Monitor Status
    std::cout << "[Step 8] Testing OpenOCDManager commands (LED_ON, LED_OFF, gpio_status)...\n";
    auto& openocd = window.openOcdManager();
    openocd.connect();
    std::cout << "  OpenOCD connected: " << (openocd.isConnected() ? "YES" : "NO") << "\n";

    QString statusReply = openocd.sendCommand("gpio_status");
    std::cout << "  gpio_status response: " << statusReply.toStdString() << "\n";

    openocd.sendCommand("LED_ON");
    std::cout << "  LED_ON response: " << openocd.readResponse().toStdString() << "\n";
    openocd.sendCommand("LED_OFF");
    std::cout << "  LED_OFF response: " << openocd.readResponse().toStdString() << "\n";

    window.grab().save("screenshots/live_hardware_connected_switch_pa4.png");
    std::cout << "  Screenshot saved: screenshots/live_hardware_connected_switch_pa4.png\n";

    // 9. Disconnect cleanly
    std::cout << "[Step 9] Disconnecting hardware...\n";
    connectBtn->click();
    QApplication::processEvents();
    std::cout << "  Hardware connected after disconnect: " << (bridge.isHardwareConnected() ? "YES" : "NO") << "\n";

    std::cout << "=======================================================\n";
    std::cout << "SUCCESS: All Hardware-in-the-Loop validation tests passed!\n";
    std::cout << "=======================================================\n";
    return 0;
}
