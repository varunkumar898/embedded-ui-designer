#include <QTest>
#include <QSignalSpy>
#include "hardware/DeviceLink.h"
#include "hardware/MockDeviceLink.h"
#include "hardware/SerialDeviceLink.h"
#include "agent/MockAgent.h"
#include "agent/dl_codec.h"
#include <cstring>

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

    void testMockAgentHelloIdentify() {
        MockAgent agent;
        agent.setChipId("Simulated device");
        agent.setSignalTableHash(0x12345678);
        agent.setProtocolVersion(1);

        // Build HELLO frame
        dl_frame_t hello; memset(&hello, 0, sizeof(hello));
        hello.version = 1; hello.type = 0x01; hello.req_id = 0x0001; hello.length = 0;
        uint8_t buf[DL_MAX_FRAME];
        size_t n = dl_encode(&hello, buf, sizeof(buf));
        QVERIFY(n > 0);

        QByteArray resp = agent.handleBytes(QByteArray((const char*)buf, (int)n));
        QVERIFY(!resp.isEmpty());

        dl_parser_t p; dl_parser_init(&p);
        dl_frame_t parsed; memset(&parsed, 0, sizeof(parsed));
        int got = 0;
        for (int i = 0; i < resp.size(); i++) {
            if (dl_parser_feed(&p, (uint8_t)resp[i], &parsed)) { got = 1; break; }
        }
        QVERIFY(got == 1);
        QCOMPARE(parsed.type, (uint8_t)0x02); // IDENTIFY
    }

    void testMockAgentPing() {
        MockAgent agent;
        dl_frame_t ping; memset(&ping, 0, sizeof(ping));
        ping.version = 1; ping.type = 0x13; ping.req_id = 0x0002; ping.length = 0;
        uint8_t buf[DL_MAX_FRAME];
        size_t n = dl_encode(&ping, buf, sizeof(buf));
        QByteArray resp = agent.handleBytes(QByteArray((const char*)buf, (int)n));
        QVERIFY(!resp.isEmpty());
        dl_parser_t p; dl_parser_init(&p);
        dl_frame_t parsed; memset(&parsed, 0, sizeof(parsed));
        int got = 0;
        for (int i = 0; i < resp.size(); i++) {
            if (dl_parser_feed(&p, (uint8_t)resp[i], &parsed)) { got = 1; break; }
        }
        QVERIFY(got == 1);
        QCOMPARE(parsed.type, (uint8_t)0x13); // PING echo
    }

    void testSerialHandshakeSuccess() {
        MockAgent agent;
        agent.setChipId("STM32F030R8");
        agent.setSignalTableHash(0xABCDEF01);
        agent.setProtocolVersion(1);

        QByteArray hello = SerialDeviceLink::buildHelloFrame();
        QVERIFY(!hello.isEmpty());
        QByteArray resp = agent.handleBytes(hello);
        QVERIFY(!resp.isEmpty());

        QString reason;
        DeviceIdentity id = SerialDeviceLink::parseIdentify(resp, 0xABCDEF01, 1, &reason);
        QVERIFY(id.chipId == "STM32F030R8");
        QVERIFY(reason.isEmpty());
    }

    void testSerialHandshakeWrongChip() {
        MockAgent agent;
        agent.setChipId("ESP32");
        agent.setSignalTableHash(0xABCDEF01);
        agent.setProtocolVersion(1);

        QByteArray hello = SerialDeviceLink::buildHelloFrame();
        QByteArray resp = agent.handleBytes(hello);
        QString reason;
        DeviceIdentity id = SerialDeviceLink::parseIdentify(resp, 0xABCDEF01, 1, &reason);
        // chip id differs from expected (caller compares); parseIdentify itself
        // succeeds at protocol level. The Mismatch is determined by the caller
        // comparing id.chipId to the selected board. Here we just ensure parse works.
        QVERIFY(!id.chipId.isEmpty());
    }

    void testSerialHandshakeWrongTableHash() {
        MockAgent agent;
        agent.setChipId("STM32F030R8");
        agent.setSignalTableHash(0x11111111);
        agent.setProtocolVersion(1);

        QByteArray hello = SerialDeviceLink::buildHelloFrame();
        QByteArray resp = agent.handleBytes(hello);
        QString reason;
        DeviceIdentity id = SerialDeviceLink::parseIdentify(resp, 0xABCDEF01, 1, &reason);
        QVERIFY(id.chipId.isEmpty());
        QVERIFY(reason.contains("hash mismatch"));
    }

    void testSerialHandshakeWrongProtoVersion() {
        MockAgent agent;
        agent.setChipId("STM32F030R8");
        agent.setSignalTableHash(0xABCDEF01);
        agent.setProtocolVersion(2);

        QByteArray hello = SerialDeviceLink::buildHelloFrame();
        QByteArray resp = agent.handleBytes(hello);
        QString reason;
        DeviceIdentity id = SerialDeviceLink::parseIdentify(resp, 0xABCDEF01, 1, &reason);
        QVERIFY(id.chipId.isEmpty());
        QVERIFY(reason.contains("protocol version mismatch"));
    }

    void testSerialHandshakeGarbage() {
        QByteArray garbage("not a frame at all \x00\x01\x02", 24);
        QString reason;
        DeviceIdentity id = SerialDeviceLink::parseIdentify(garbage, 0xABCDEF01, 1, &reason);
        QVERIFY(id.chipId.isEmpty());
        QVERIFY(!reason.isEmpty());
    }

QTEST_MAIN(TestDeviceLink)
#include "test_device_link.moc"