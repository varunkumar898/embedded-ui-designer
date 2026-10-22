#include <QTest>
#include <QSet>
#include "hardware/HardwareBridge.h"
#include "hardware/OpenOcdManager.h"
#include "hardware/OcdTelnetManager.h"
#include "hardware/WriteGuard.h"

class TestWriteGuardPaths : public QObject {
    Q_OBJECT
private slots:
    void init() {
        HardwareBridge::instance().setWriteOverride(false);
        HardwareBridge::instance().openOcdManager().clearMemoryWriteHistory();
    }

    void cleanup() {
        HardwareBridge::instance().setWriteOverride(false);
        HardwareBridge::instance().openOcdManager().clearMemoryWriteHistory();
    }

    // 1. Default-deny: with the F030R8 profile live (default), a write to an
    //    address outside the board profile's signal table is rejected through
    //    BOTH managers: nothing reaches the simulated memory and no entry is
    //    added to the daemon's write history.
    void testDaemonWriteGuardRejectsUnsourcedAddress() {
        HardwareBridge& bridge = HardwareBridge::instance();
        OpenOcdManager& ocd = bridge.openOcdManager();

        QVERIFY(bridge.isLiveWriteAllowed());
        QVERIFY(!bridge.allowedWriteAddresses().contains(0xDEADBEEF));

        QVERIFY(!ocd.writeMemoryWord(0xDEADBEEF, 0x1234));
        QCOMPARE(ocd.memoryWriteHistory().size(), 0);

        // The telnet-side manager must reject the same write:
        OcdTelnetManager telnet;
        QVERIFY(!telnet.writeMemoryWord(0xDEADBEEF, 0x1234));
        QCOMPARE(ocd.memoryWriteHistory().size(), 0);
    }

    // 2. F030R8 simulated-hardware path: an address sourced from the active
    //    board profile IS written into simulated memory, and one write (from
    //    either manager, not both) is recorded exactly once in the history.
    void testF030R8SimulatedWriteHonorsGuard() {
        HardwareBridge& bridge = HardwareBridge::instance();
        OpenOcdManager& ocd = bridge.openOcdManager();

        const QSet<quint32> allowed = bridge.allowedWriteAddresses();
        QVERIFY(!allowed.isEmpty());
        const quint32 addr = *allowed.constBegin();

        QVERIFY(ocd.writeMemoryWord(addr, 0x77));
        QCOMPARE(ocd.simulatedMemoryWord(addr), quint32(0x77));
        QCOMPARE(ocd.memoryWriteHistory().size(), 1);
        QCOMPARE(ocd.memoryWriteHistory().constFirst().first, addr);
        QCOMPARE(ocd.memoryWriteHistory().constFirst().second, quint32(0x77));

        ocd.clearMemoryWriteHistory();
        QVERIFY(ocd.writeMemoryWord(addr, 0x88));
        QCOMPARE(ocd.simulatedMemoryWord(addr), quint32(0x88));
        QCOMPARE(ocd.memoryWriteHistory().size(), 1);
    }

    // 3. Chain enforcement: a write issued through OcdTelnetManager is
    //    guarded there AND lands in the daemon (single history entry),
    //    while a rejected one never reaches the daemon's simulated memory.
    void testTelnetManagerForwardsThroughGuard() {
        HardwareBridge& bridge = HardwareBridge::instance();
        OpenOcdManager& ocd = bridge.openOcdManager();
        OcdTelnetManager telnet;

        const QSet<quint32> allowed = bridge.allowedWriteAddresses();
        QVERIFY(!allowed.isEmpty());
        const quint32 addr = *allowed.constBegin();

        QVERIFY(telnet.writeMemoryWord(addr, 0xC0FFEE));
        QCOMPARE(ocd.memoryWriteHistory().size(), 1);
        QCOMPARE(ocd.simulatedMemoryWord(addr), quint32(0xC0FFEE));
    }

    // 4. Explicit per-session override bypasses the table/profile checks but
    //    only for the current session, and the write still goes through the
    //    guarded managers (never around them).
    void testOverrideBypass() {
        HardwareBridge& bridge = HardwareBridge::instance();
        OpenOcdManager& ocd = bridge.openOcdManager();

        // Without override this is rejected (1); with it, accepted.
        QVERIFY(!ocd.writeMemoryWord(0xDEADBEEF, 0x1234));
        QCOMPARE(ocd.memoryWriteHistory().size(), 0);

        bridge.setWriteOverride(true);
        QVERIFY(ocd.writeMemoryWord(0xDEADBEEF, 0x1234));
        QCOMPARE(ocd.simulatedMemoryWord(0xDEADBEEF), quint32(0x1234));
        QCOMPARE(ocd.memoryWriteHistory().size(), 1);
    }
};

QTEST_MAIN(TestWriteGuardPaths)
#include "test_write_guard_paths.moc"
