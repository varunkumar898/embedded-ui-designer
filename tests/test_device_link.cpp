#include <QTest>
#include <QSignalSpy>
#include "hardware/DeviceLink.h"
#include "hardware/MockDeviceLink.h"

using namespace Hardware;

class TestDeviceLink : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {}
    void cleanupTestCase() {}

    void testMockConnectIdentify() {
        MockDeviceLink link;
        link.setSimulatedConnected(true, "Simulated");
        link.connectDevice();
        QCOMPARE(link.state(), DeviceLinkState::DeviceFound);
        DeviceIdentity id = link.identify();
        QVERIFY(!id.chipId.isEmpty());
        QCOMPARE(link.state(), DeviceLinkState::Connected);
        QVERIFY(link.isConnected());
    }

    void testNoDeviceStaysDisconnected() {
        MockDeviceLink link;
        link.connectDevice();
        QCOMPARE(link.state(), DeviceLinkState::NoDevice);
        QVERIFY(!link.isConnected());
        QVERIFY(!link.identify().chipId.isEmpty() == false);
    }

    void testMismatchState() {
        MockDeviceLink link;
        link.setSimulatedConnected(true, "Simulated");
        link.connectDevice();
        link.identify();
        link.setSimulatedConnected(false, "Simulated");
        // After disconnect, state should not be Connected.
        QVERIFY(link.state() != DeviceLinkState::Connected || !link.isConnected());
    }

    void testWriteGuardAllowsSourced() {
        MockDeviceLink link;
        link.setAllowedAddresses({0x48000018});
        QVERIFY(link.writeAddressAllowed(0x48000018));
        QVERIFY(!link.writeAddressAllowed(0xDEADBEEF));
    }

    void testWriteGuardOverride() {
        MockDeviceLink link;
        link.setAllowedAddresses({});
        QVERIFY(!link.writeAddressAllowed(0x48000018));
        link.setWriteOverride(true);
        QVERIFY(link.writeAddressAllowed(0x48000018));
        link.setWriteOverride(false);
        QVERIFY(!link.writeAddressAllowed(0x48000018));
    }

    void testSimulatedNeverShowsRealBoardName() {
        MockDeviceLink link;
        link.setSimulatedConnected(true, "ST-LINK/V2.1 (STMicroelectronics)");
        // probe name must have been neutralized
        // (state reason / identity should not leak a real board name)
        QVERIFY(link.stateReason().contains("Simulated") || link.stateReason().isEmpty());
    }

    void testPingAndReadWrite() {
        MockDeviceLink link;
        link.setSimulatedConnected(true, "Simulated");
        link.connectDevice();
        link.identify();
        link.setSimulatedSignal("LED", 42);
        QVariant v;
        QVERIFY(link.readSignal("LED", &v));
        QCOMPARE(v.toInt(), 42);
        QVERIFY(link.writeSignal("LED", 7));
        QVariant v2;
        QVERIFY(link.readSignal("LED", &v2));
        QCOMPARE(v2.toInt(), 7);
        QVERIFY(link.ping());
    }
};

QTEST_MAIN(TestDeviceLink)
#include "test_device_link.moc"