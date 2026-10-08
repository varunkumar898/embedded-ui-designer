#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QTableWidget>
#include <QLineEdit>
#include <QMap>
#include "SimulationRuntime.h"

namespace Simulator {

/**
 * @brief SimulationControlPanel provides live inspector controls to view and manipulate
 * simulated DataSources, inject hardware errors, apply automotive dashboard presets, and monitor events.
 */
class SimulationControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit SimulationControlPanel(SimulationRuntime* runtime, QWidget* parent = nullptr);
    ~SimulationControlPanel() override = default;

public slots:
    void rebuildDataSourceControls();
    void refreshValues();
    void onLogEntryAdded(const SimulationLogEntry& entry);

private slots:
    void onPresetChanged(int index);
    void onToggleConnection(bool checked);
    void onToggleErrorInjection(bool checked);
    void onResetHardware();
    void onClearLogs();

private:
    void setupUi();
    QWidget* createDataSourcesTab();
    QWidget* createHardwareTab();
    QWidget* createLogTab();

    SimulationRuntime* m_runtime = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    // Data source control mapping
    struct SourceRowWidgets {
        QWidget* container = nullptr;
        QSlider* slider = nullptr;
        QDoubleSpinBox* doubleSpin = nullptr;
        QSpinBox* intSpin = nullptr;
        QCheckBox* checkBox = nullptr;
        QLineEdit* lineEdit = nullptr;
        QLabel* ledIndicator = nullptr;
    };
    QMap<QString, SourceRowWidgets> m_sourceWidgets;
    QVBoxLayout* m_sourcesLayout = nullptr;

    // Hardware tab controls
    QPushButton* m_btnConnection = nullptr;
    QCheckBox* m_chkErrorInjection = nullptr;

    // Log tab
    QTableWidget* m_logTable = nullptr;
};

} // namespace Simulator
