#include "SimulationControlPanel.h"
#include <QHeaderView>
#include <QScrollArea>
#include <QGroupBox>
#include <QDateTime>

namespace Simulator {

SimulationControlPanel::SimulationControlPanel(SimulationRuntime* runtime, QWidget* parent)
    : QWidget(parent)
    , m_runtime(runtime)
{
    setupUi();

    if (m_runtime) {
        connect(m_runtime, &SimulationRuntime::dataSourceValueChanged, this, &SimulationControlPanel::refreshValues);
        connect(m_runtime, &SimulationRuntime::logEntryAdded, this, &SimulationControlPanel::onLogEntryAdded);
    }

    rebuildDataSourceControls();
}

void SimulationControlPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(6);

    setStyleSheet(
        "QWidget { background-color: #0f172a; color: #f8fafc; font-family: sans-serif; }"
        "QTabWidget::pane { border: 1px solid #334155; background-color: #0f172a; }"
        "QTabBar::tab { background-color: #1e293b; color: #94a3b8; padding: 6px 12px; margin-right: 2px; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
        "QTabBar::tab:selected { background-color: #334155; color: #38bdf8; font-weight: bold; }"
        "QGroupBox { font-weight: bold; border: 1px solid #334155; border-radius: 6px; margin-top: 6px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: #38bdf8; }"
        "QPushButton { background-color: #1e293b; border: 1px solid #475569; border-radius: 4px; padding: 5px 10px; color: #f8fafc; }"
        "QPushButton:hover { background-color: #334155; border-color: #38bdf8; }"
        "QSlider::groove:horizontal { height: 4px; background: #334155; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #38bdf8; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #f8fafc; border: 1px solid #0284c7; width: 14px; margin-top: -5px; margin-bottom: -5px; border-radius: 7px; }"
        "QSpinBox, QDoubleSpinBox, QLineEdit, QComboBox { background-color: #1e293b; border: 1px solid #334155; border-radius: 4px; padding: 3px 6px; color: #f8fafc; }"
        "QTableWidget { background-color: #0f172a; gridline-color: #1e293b; border: 1px solid #334155; color: #e2e8f0; }"
        "QHeaderView::section { background-color: #1e293b; color: #94a3b8; padding: 4px; border: 1px solid #334155; }"
    );

    // Preset selection header
    auto* presetBox = new QGroupBox("Automotive / Dashboard Presets", this);
    auto* presetLayout = new QHBoxLayout(presetBox);
    auto* presetCombo = new QComboBox(presetBox);
    presetCombo->setObjectName("simPresetCombo");
    presetCombo->addItem("-- Select Simulation Preset --");
    if (m_runtime) {
        for (const QString& p : m_runtime->availablePresets()) {
            presetCombo->addItem(p);
        }
    }
    connect(presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SimulationControlPanel::onPresetChanged);
    presetLayout->addWidget(presetCombo);
    mainLayout->addWidget(presetBox);

    // Tab Widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createDataSourcesTab(), "Data Sources");
    m_tabWidget->addTab(createHardwareTab(), "Virtual HAL");
    m_tabWidget->addTab(createLogTab(), "Data Monitor");
    mainLayout->addWidget(m_tabWidget, 1);
}

QWidget* SimulationControlPanel::createDataSourcesTab() {
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { border: none; background: transparent; }");

    auto* container = new QWidget(scroll);
    m_sourcesLayout = new QVBoxLayout(container);
    m_sourcesLayout->setContentsMargins(6, 6, 6, 6);
    m_sourcesLayout->setSpacing(8);

    scroll->setWidget(container);
    return scroll;
}

QWidget* SimulationControlPanel::createHardwareTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(10);

    auto* grpStatus = new QGroupBox("Hardware Safety & Connection", widget);
    auto* statusLayout = new QVBoxLayout(grpStatus);

    auto* lblSafe = new QLabel("Safety Isolation: Physical hardware access is LOCKED in Simulation mode.", grpStatus);
    lblSafe->setWordWrap(true);
    lblSafe->setStyleSheet("color: #38bdf8; font-size: 11px;");
    statusLayout->addWidget(lblSafe);

    m_btnConnection = new QPushButton("Virtual Connection: ONLINE", grpStatus);
    m_btnConnection->setCheckable(true);
    m_btnConnection->setChecked(true);
    m_btnConnection->setStyleSheet("background-color: #065f46; color: #a7f3d0; font-weight: bold;");
    connect(m_btnConnection, &QPushButton::toggled, this, &SimulationControlPanel::onToggleConnection);
    statusLayout->addWidget(m_btnConnection);

    m_chkErrorInjection = new QCheckBox("Inject Hardware Faults (ADC Noise & IO Timeouts)", grpStatus);
    connect(m_chkErrorInjection, &QCheckBox::toggled, this, &SimulationControlPanel::onToggleErrorInjection);
    statusLayout->addWidget(m_chkErrorInjection);

    auto* btnReset = new QPushButton("Reset Virtual Hardware", grpStatus);
    connect(btnReset, &QPushButton::clicked, this, &SimulationControlPanel::onResetHardware);
    statusLayout->addWidget(btnReset);

    layout->addWidget(grpStatus);
    layout->addStretch();
    return widget;
}

QWidget* SimulationControlPanel::createLogTab() {
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(4, 4, 4, 4);

    m_logTable = new QTableWidget(0, 4, widget);
    m_logTable->setHorizontalHeaderLabels({"Time", "DataSource", "Old Value", "New Value"});
    m_logTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_logTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_logTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_logTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_logTable->verticalHeader()->setVisible(false);
    layout->addWidget(m_logTable);

    auto* btnClear = new QPushButton("Clear Monitor Log", widget);
    connect(btnClear, &QPushButton::clicked, this, &SimulationControlPanel::onClearLogs);
    layout->addWidget(btnClear);

    return widget;
}

void SimulationControlPanel::rebuildDataSourceControls() {
    if (!m_sourcesLayout) return;

    // Clear existing
    for (auto& row : m_sourceWidgets) {
        if (row.container) {
            m_sourcesLayout->removeWidget(row.container);
            row.container->deleteLater();
        }
    }
    m_sourceWidgets.clear();

    if (!m_runtime || !m_runtime->project()) return;

    for (const auto& ds : m_runtime->project()->dataSources()) {
        QString id = ds.id();
        SourceRowWidgets row;
        row.container = new QGroupBox(QString("%1 (%2)").arg(ds.name(), dataSourceTypeToString(ds.type())), this);
        auto* rowLayout = new QHBoxLayout(row.container);
        rowLayout->setContentsMargins(6, 6, 6, 6);

        if (ds.dataType() == DataType::Boolean || ds.type() == DataSourceType::Gpio) {
            row.checkBox = new QCheckBox("Active / High", row.container);
            row.checkBox->setChecked(m_runtime->dataSourceValue(id).toBool());

            row.ledIndicator = new QLabel(row.container);
            row.ledIndicator->setFixedSize(14, 14);
            row.ledIndicator->setStyleSheet(row.checkBox->isChecked() ? "background: #22c55e; border-radius: 7px;" : "background: #475569; border-radius: 7px;");

            connect(row.checkBox, &QCheckBox::toggled, this, [this, id](bool checked) {
                if (m_runtime) m_runtime->setDataSourceValue(id, checked);
            });

            rowLayout->addWidget(row.checkBox);
            rowLayout->addWidget(row.ledIndicator);
        } else if (ds.dataType() == DataType::Float || ds.dataType() == DataType::Integer || ds.dataType() == DataType::UnsignedInteger || ds.type() == DataSourceType::Sensor || ds.type() == DataSourceType::Adc || ds.type() == DataSourceType::Pwm) {
            double minV = ds.metadata().value("min").toDouble(0.0);
            double maxV = ds.metadata().value("max").toDouble(100.0);
            if (maxV <= minV) maxV = minV + 100.0;

            row.slider = new QSlider(Qt::Horizontal, row.container);
            row.slider->setRange(0, 1000);
            double curVal = m_runtime->dataSourceValue(id).toDouble();
            int sliderPos = static_cast<int>(((curVal - minV) / (maxV - minV)) * 1000.0);
            row.slider->setValue(sliderPos);

            row.doubleSpin = new QDoubleSpinBox(row.container);
            row.doubleSpin->setRange(minV, maxV);
            row.doubleSpin->setValue(curVal);
            row.doubleSpin->setSingleStep((maxV - minV) / 50.0);

            connect(row.slider, &QSlider::valueChanged, this, [this, id, minV, maxV, row](int pos) {
                double val = minV + (static_cast<double>(pos) / 1000.0) * (maxV - minV);
                if (row.doubleSpin && std::abs(row.doubleSpin->value() - val) > 0.01) {
                    row.doubleSpin->blockSignals(true);
                    row.doubleSpin->setValue(val);
                    row.doubleSpin->blockSignals(false);
                }
                if (m_runtime) m_runtime->setDataSourceValue(id, val);
            });

            connect(row.doubleSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, id, minV, maxV, row](double val) {
                int pos = static_cast<int>(((val - minV) / (maxV - minV)) * 1000.0);
                if (row.slider && row.slider->value() != pos) {
                    row.slider->blockSignals(true);
                    row.slider->setValue(pos);
                    row.slider->blockSignals(false);
                }
                if (m_runtime) m_runtime->setDataSourceValue(id, val);
            });

            rowLayout->addWidget(row.slider, 2);
            rowLayout->addWidget(row.doubleSpin, 1);
        } else {
            row.lineEdit = new QLineEdit(row.container);
            row.lineEdit->setText(m_runtime->dataSourceValue(id).toString());
            connect(row.lineEdit, &QLineEdit::textChanged, this, [this, id](const QString& text) {
                if (m_runtime) m_runtime->setDataSourceValue(id, text);
            });
            rowLayout->addWidget(row.lineEdit);
        }

        m_sourcesLayout->addWidget(row.container);
        m_sourceWidgets[id] = row;
    }

    m_sourcesLayout->addStretch();
}

void SimulationControlPanel::refreshValues() {
    if (!m_runtime) return;

    for (auto it = m_sourceWidgets.begin(); it != m_sourceWidgets.end(); ++it) {
        QString id = it.key();
        SourceRowWidgets& row = it.value();
        QVariant val = m_runtime->dataSourceValue(id);

        if (row.checkBox) {
            bool b = val.toBool();
            if (row.checkBox->isChecked() != b) {
                row.checkBox->blockSignals(true);
                row.checkBox->setChecked(b);
                row.checkBox->blockSignals(false);
            }
            if (row.ledIndicator) {
                row.ledIndicator->setStyleSheet(b ? "background: #22c55e; border-radius: 7px;" : "background: #475569; border-radius: 7px;");
            }
        }
        if (row.doubleSpin) {
            double d = val.toDouble();
            if (std::abs(row.doubleSpin->value() - d) > 0.001) {
                row.doubleSpin->blockSignals(true);
                row.doubleSpin->setValue(d);
                row.doubleSpin->blockSignals(false);

                if (row.slider) {
                    double minV = row.doubleSpin->minimum();
                    double maxV = row.doubleSpin->maximum();
                    int pos = static_cast<int>(((d - minV) / (maxV - minV)) * 1000.0);
                    row.slider->blockSignals(true);
                    row.slider->setValue(pos);
                    row.slider->blockSignals(false);
                }
            }
        }
    }
}

void SimulationControlPanel::onPresetChanged(int index) {
    auto* combo = qobject_cast<QComboBox*>(sender());
    if (!combo || index <= 0 || !m_runtime) return;

    QString preset = combo->itemText(index);
    m_runtime->applyPreset(preset);
    refreshValues();
}

void SimulationControlPanel::onToggleConnection(bool checked) {
    if (m_runtime && m_runtime->simulationBackend()) {
        m_runtime->simulationBackend()->setSimulatedConnected(checked);
        m_btnConnection->setText(checked ? "Virtual Connection: ONLINE" : "Virtual Connection: OFFLINE (Simulated Disconnect)");
        m_btnConnection->setStyleSheet(checked ? "background-color: #065f46; color: #a7f3d0; font-weight: bold;"
                                               : "background-color: #7f1d1d; color: #fecaca; font-weight: bold;");
    }
}

void SimulationControlPanel::onToggleErrorInjection(bool checked) {
    if (m_runtime && m_runtime->simulationBackend()) {
        m_runtime->simulationBackend()->setSimulateErrors(checked);
    }
}

void SimulationControlPanel::onResetHardware() {
    if (m_runtime && m_runtime->simulationBackend()) {
        m_runtime->simulationBackend()->resetAllSimulationData();
        refreshValues();
    }
}

void SimulationControlPanel::onLogEntryAdded(const SimulationLogEntry& entry) {
    if (!m_logTable) return;

    int row = m_logTable->rowCount();
    m_logTable->insertRow(row);
    m_logTable->setItem(row, 0, new QTableWidgetItem(entry.timeString));
    m_logTable->setItem(row, 1, new QTableWidgetItem(entry.sourceId));
    m_logTable->setItem(row, 2, new QTableWidgetItem(entry.oldValue.toString()));
    m_logTable->setItem(row, 3, new QTableWidgetItem(entry.newValue.toString()));

    if (row > 300) {
        m_logTable->removeRow(0);
    }
    m_logTable->scrollToBottom();
}

void SimulationControlPanel::onClearLogs() {
    if (m_logTable) {
        m_logTable->setRowCount(0);
    }
    if (m_runtime) {
        m_runtime->clearLogs();
    }
}

} // namespace Simulator
