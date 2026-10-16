#pragma once

#include <QWidget>
#include <QString>

#include "models/BoardDisplayConfig.h"
#include "analyzer/FramebufferFit.h"

class Project;
class QComboBox;
class QSpinBox;
class QLabel;
class QTableWidget;

/**
 * @brief Task 5 - "Board & Display" dock.
 *
 * Three sections:
 *  1. Target: MCU combo (CapabilityDatabase devices), board combo filtered to
 *     the selected MCU (or "Custom / none"), read-only capability summary and
 *     the memory-region table with framebuffer suitability.
 *  2. Display: resolution (kept in sync with the canvas), pixel format limited
 *     to what the generators/runtime support, interface / controller / touch
 *     driver limited to implemented drivers. No rotation (the project has none)
 *     and no pin choices (pins are never hardcoded here).
 *  3. Buffering: full / partial single / partial double, editable buffer height,
 *     live computed framebuffer size with the visible formula and a fit
 *     indicator evaluated against framebuffer-capable RAM regions only.
 *
 * Every edit is persisted additively through Project (BoardDisplayConfig /
 * DisplayConfig / HardwareConfig) and triggers `settingsChanged`, which
 * MainWindow wires to an immediate analyzer + diagnostics refresh.
 */
class BoardDisplayPanel : public QWidget {
    Q_OBJECT

public:
    explicit BoardDisplayPanel(QWidget* parent = nullptr);
    ~BoardDisplayPanel() override = default;

    void setProject(Project* project);
    Project* project() const { return m_project; }

    BoardDisplayConfig currentConfig() const { return m_config; }
    FramebufferFit::Result currentFit() const { return m_lastFit; }

    // Task 6: effective target backend of the project (registry id, e.g.
    // "lvgl" / "qt-for-mcus"), derived exactly like the export dispatch.
    QString currentBackendId() const;

    // Text mirrors of the live preview widgets (stable API for acceptance tests).
    QString fitStatusText() const;
    QString fitHintText() const;
    QString formulaText() const;
    QString sizeText() const;

    // Programmatic setters: update the widgets, persist through the project and
    // emit settingsChanged exactly like a user edit would.
    void setFramebufferMode(BoardDisplayConfig::FramebufferMode mode);
    void setPartialBufferLines(int lines);
    void setInterfaceId(const QString& id);
    void setControllerId(const QString& id);
    void setTouchDriverId(const QString& id);
    void setDisplaySize(int width, int height);
    void setPixelFormat(const QString& colorFormat, int colorDepth);
    void setTargetDevice(const QString& deviceName);
    // Task 6: selects the target backend; the planned entry is ignored.
    void setTargetBackend(const QString& backendId);

signals:
    void settingsChanged();

public slots:
    // Re-reads hardware / display / board-display state from the project.
    void refreshFromProject();

private slots:
    void onBackendChanged(int index);
    void onMcuChanged(int index);
    void onBoardChanged(int index);
    void onResolutionChanged();
    void onPixelFormatChanged(int index);
    void onInterfaceChanged(int index);
    void onControllerChanged(int index);
    void onTouchChanged(int index);
    void onModeChanged(int index);
    void onBufferLinesChanged(int lines);

private:
    void setupUi();
    void rebuildBackendCombo();
    void populateMcuCombo();
    void rebuildBoardCombo();
    void updateTargetSummary();
    void rebuildRegionsTable();
    void updateDisplayControls();
    void updateBufferingControls();
    void updateComputedPreview();

    void applyConfig(const BoardDisplayConfig& config);
    DeviceProfile currentDevice() const;
    QString capabilityDeviceFor(const QString& rawMcu) const;
    static bool boardMatchesDevice(const QString& boardMcu, const QString& deviceId);
    void syncInterfaceCombo(QComboBox* combo, const QStringList& options,
                            const QString& current, const QString& placeholder);

    Project* m_project = nullptr;
    BoardDisplayConfig m_config;
    FramebufferFit::Result m_lastFit;
    bool m_updating = false;

    // Target section
    QComboBox* m_backendCombo = nullptr;
    QComboBox* m_mcuCombo = nullptr;
    QComboBox* m_boardCombo = nullptr;
    QLabel* m_targetSummary = nullptr;
    QTableWidget* m_regionsTable = nullptr;

    // Display section
    QSpinBox* m_widthSpin = nullptr;
    QSpinBox* m_heightSpin = nullptr;
    QComboBox* m_pixelFormatCombo = nullptr;
    QComboBox* m_interfaceCombo = nullptr;
    QComboBox* m_controllerCombo = nullptr;
    QComboBox* m_touchCombo = nullptr;

    // Buffering section
    QComboBox* m_modeCombo = nullptr;
    QSpinBox* m_bufferLinesSpin = nullptr;
    QLabel* m_sizeLabel = nullptr;
    QLabel* m_formulaLabel = nullptr;
    QLabel* m_fitLabel = nullptr;
    QLabel* m_fitHintLabel = nullptr;
};
