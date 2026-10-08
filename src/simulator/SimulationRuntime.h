#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QVariant>
#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>
#include <memory>
#include "Project.h"
#include "Screen.h"
#include "UIComponent.h"
#include "DataSource.h"
#include "DataBinding.h"
#include "SimulationBackend.h"

namespace Simulator {

enum class SimulationStatus {
    Stopped,
    Running,
    Paused
};

struct SimulationLogEntry {
    qint64 timestampMs = 0;
    QString timeString;
    QString sourceId;
    QString componentId;
    QString propertyName;
    QVariant oldValue;
    QVariant newValue;
    QString details;
};

/**
 * @brief SimulationRuntime executes a Project's multi-screen UI, interactive controls,
 * simulated DataSources, bidirectional DataBindings, ComponentStates, and virtual Hardware.
 */
class SimulationRuntime : public QObject {
    Q_OBJECT

public:
    explicit SimulationRuntime(Project* project, QObject* parent = nullptr);
    ~SimulationRuntime() override;

    Project* project() const { return m_project; }
    SimulationBackend* simulationBackend() const { return m_backend.get(); }

    // Lifecycle
    SimulationStatus status() const { return m_status; }
    bool isRunning() const { return m_status == SimulationStatus::Running; }
    bool isPaused() const { return m_status == SimulationStatus::Paused; }
    bool isStopped() const { return m_status == SimulationStatus::Stopped; }

    void start();
    void pause();
    void resume();
    void stop();
    void restart();
    void step(int stepMs = 50);

    // Simulation Clock & Speed
    double speedFactor() const { return m_speedFactor; }
    void setSpeedFactor(double factor);
    qint64 simulatedElapsedMs() const { return m_simulatedElapsedMs; }

    // Multi-Screen Navigation
    QString activeScreenId() const { return m_activeScreenId; }
    Screen* activeScreen() const;
    void setActiveScreenId(const QString& screenId);

    // Simulated DataSources
    QVariant dataSourceValue(const QString& sourceId) const;
    void setDataSourceValue(const QString& sourceId, const QVariant& value);
    QMap<QString, QVariant> allDataSourceValues() const { return m_simulatedValues; }

    // Presets
    QStringList availablePresets() const;
    void applyPreset(const QString& presetName);

    // Logs & Monitoring
    const QList<SimulationLogEntry>& logEntries() const { return m_logEntries; }
    void clearLogs();

    // Safe Expression Evaluation for Calculated DataSources
    static double evaluateSafeExpression(const QString& expr, const QMap<QString, double>& variables);

    // Component Interaction Hooks
    void notifyComponentPropertyChanged(UIComponent* comp, const QString& propertyName, const QVariant& value);
    void notifyComponentInteraction(UIComponent* comp, const QString& trigger);

    // Simulated Protocol Injections (Phase 1 Option B)
    void injectCanFrame(quint32 messageId, const QByteArray& payload);
    void injectUartStream(const QString& rawFrame);
    void injectModbusRegisters(int slaveId, ModbusRegisterType regType, int startAddress, const QVector<quint16>& rawRegs);

signals:
    void statusChanged(SimulationStatus status);
    void speedFactorChanged(double factor);
    void activeScreenChanged(const QString& screenId);
    void dataSourceValueChanged(const QString& sourceId, const QVariant& value);
    void componentPropertyChanged(const QString& compId, const QString& prop, const QVariant& val);
    void logEntryAdded(const SimulationLogEntry& entry);
    void clockTicked(qint64 elapsedMs);

private slots:
    void onTickTimer();

private:
    void setupInitialState();
    void restoreProjectDesignSnapshot();
    void saveProjectDesignSnapshot();
    void propagateDataSourceBindings(const QString& sourceId, const QVariant& value);
    void updateTimerDataSources(int deltaMs);
    void updateCalculatedDataSources();
    void updateVariableWaveforms(qint64 elapsedMs);
    void applyComponentProperty(UIComponent* comp, const QString& property, const QVariant& value);
    UIComponent* findComponent(const QString& compId) const;

    Project* m_project = nullptr;
    std::shared_ptr<SimulationBackend> m_backend;
    SimulationStatus m_status = SimulationStatus::Stopped;
    double m_speedFactor = 1.0;
    qint64 m_simulatedElapsedMs = 0;
    QString m_activeScreenId;

    QTimer* m_tickTimer = nullptr;
    QMap<QString, QVariant> m_simulatedValues;
    QList<SimulationLogEntry> m_logEntries;
    bool m_inBindingUpdate = false;

    // Snapshot of original design properties to ensure project save safety
    struct ComponentSnapshot {
        QJsonObject originalJson;
    };
    QMap<QString, ComponentSnapshot> m_designSnapshot;
    QMap<QString, QVariant> m_originalDataSourceValues;
    QString m_originalActiveScreenId;
};

} // namespace Simulator
