#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QTcpSocket>
#include <QTimer>
#include <QHash>
#include <QPair>

struct DiscoveredProbe {
    bool detected = false;
    QString name;
    QString vendorId;
    QString productId;
    QString serialNumber;
    QString suggestedInterface;
};

/**
 * @brief Manages OpenOCD daemon lifecycle, USB debug probe auto-discovery,
 * and live register read/write over OpenOCD TCL/Telnet RPC.
 */
class OpenOcdManager : public QObject {
    Q_OBJECT

public:
    explicit OpenOcdManager(QObject* parent = nullptr);
    ~OpenOcdManager() override;

    bool isConnected() const { return m_isConnected; }
    QString connectedProbeName() const { return m_connectedProbeName; }
    DiscoveredProbe lastDiscoveredProbe() const { return m_lastProbe; }

    // Background USB probe polling
    void startProbePolling(int intervalMs = 1000);
    void stopProbePolling();
    DiscoveredProbe scanUsbProbes();

    // Target Connection and Lifecycle
    bool connectToTarget(const QString& interfaceCfg, const QString& targetCfg, int tclPort = 6666, int telnetPort = 4444);
    void disconnectTarget();

    // Live Register / Memory Access via OpenOCD
    bool writeMemoryWord(quint32 address, quint32 value);
    bool readMemoryWord(quint32 address, quint32* outValue);

    // Simulation overrides (for headless testing & probe plug/unplug simulation)
    void setSimulatedConnected(bool connected, const QString& probeName = "ST-LINK/V2.1 (Simulated)");
    quint32 simulatedMemoryWord(quint32 address) const;
    void setSimulatedMemoryWord(quint32 address, quint32 value);
    QList<QPair<quint32, quint32>> memoryWriteHistory() const { return m_writeHistory; }
    void clearMemoryWriteHistory() { m_writeHistory.clear(); }

signals:
    void connectionStatusChanged(bool connected, const QString& probeName);
    void probeDiscovered(const DiscoveredProbe& probe);
    void probeRemoved();
    void logMessage(const QString& msg);

public slots:
    void onPollTimer();

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onSocketConnected();
    void onSocketDisconnected();

private:
    bool launchOpenOcdProcess(const QString& interfaceCfg, const QString& targetCfg, int tclPort, int telnetPort);
    bool executeTclCommand(const QString& cmd, QString* outResponse = nullptr);

    QTimer m_pollTimer;
    QProcess* m_process = nullptr;
    QTcpSocket* m_socket = nullptr;

    bool m_isConnected = false;
    bool m_simulatedMode = false;
    QString m_connectedProbeName;
    DiscoveredProbe m_lastProbe;

    QString m_activeInterface;
    QString m_activeTarget;
    int m_tclPort = 6666;
    int m_telnetPort = 4444;

    QHash<quint32, quint32> m_simulatedMemory;
    QList<QPair<quint32, quint32>> m_writeHistory;
};
