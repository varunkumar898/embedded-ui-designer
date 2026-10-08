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

    // 4. Configure Switch on Canvas bound to PA0
    std::cout << "[Step 4] Adding SwitchComponent bound to GPIO PA0...\n";
    auto* canvasScene = window.canvasScene();
    if (!canvasScene) {
        std::cerr << "FAIL: canvasScene not found!\n";
        return 6;
    }

    auto* sw = new SwitchComponent("switch_pa0");
    sw->setCompPos(100, 100);
    sw->setCompSize(70, 36);
    canvasScene->addUIComponent(sw);
    bridge.bindComponent(sw, "PA0", PinMode::DigitalOut);
    QApplication::processEvents();

    // 5. Test Live GPIO Output Toggle HIGH (3.3V)
    std::cout << "[Step 5] Toggling Switch to ON (HIGH - 3.3V)...\n";
    sw->setChecked(true);
    QApplication::processEvents();

    quint32 valHigh = 0;
    bridge.openOcdManager().readMemoryWord(0x48000014, &valHigh);
    std::cout << "  GPIOA ODR readback (HIGH): 0x" << std::hex << valHigh << std::dec << "\n";
    std::cout << "  PA0 bit value: " << (valHigh & 0x1) << " (Expected 1)\n";

    if ((valHigh & 0x1) != 1) {
        std::cerr << "FAIL: GPIOA PA0 ODR bit 0 was not set HIGH!\n";
        return 7;
    }

    // 6. Test Live GPIO Output Toggle LOW (0V)
    std::cout << "[Step 6] Toggling Switch to OFF (LOW - 0V)...\n";
    sw->setChecked(false);
    QApplication::processEvents();

    quint32 valLow = 0;
    bridge.openOcdManager().readMemoryWord(0x48000014, &valLow);
    std::cout << "  GPIOA ODR readback (LOW): 0x" << std::hex << valLow << std::dec << "\n";
    std::cout << "  PA0 bit value: " << (valLow & 0x1) << " (Expected 0)\n";

    if ((valLow & 0x1) != 0) {
        std::cerr << "FAIL: GPIOA PA0 ODR bit 0 was not reset LOW!\n";
        return 8;
    }

    window.grab().save("screenshots/live_hardware_connected_switch_pa0.png");
    std::cout << "  Screenshot saved: screenshots/live_hardware_connected_switch_pa0.png\n";

    // 7. Disconnect cleanly
    std::cout << "[Step 7] Disconnecting hardware...\n";
    connectBtn->click();
    QApplication::processEvents();
    std::cout << "  Hardware connected after disconnect: " << (bridge.isHardwareConnected() ? "YES" : "NO") << "\n";

    std::cout << "=======================================================\n";
    std::cout << "SUCCESS: All board auto-detection & GPIO live tests passed!\n";
    std::cout << "=======================================================\n";
    return 0;
}
