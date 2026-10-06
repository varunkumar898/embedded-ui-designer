#pragma once

#include <QDialog>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "HardwareModel.h"
#include "PinMuxEngine.h"
#include "PinoutViewWidget.h"
#include "DeviceDatabase.h"

class HardwareWizardDialog : public QDialog {
    Q_OBJECT

public:
    explicit HardwareWizardDialog(QWidget* parent = nullptr);
    ~HardwareWizardDialog() override = default;

    Hardware::HardwareConfig hardwareConfiguration() const;
    QString projectName() const;
    int displayWidth() const;
    int displayHeight() const;
    bool isRound() const;
    int colorDepth() const;

private slots:
    void onNextStep();
    void onPrevStep();
    void onFinish();

    // Step 1: Device / Board / Custom selection
    void onFilterChanged();
    void onAllTableRowSelected();
    void onDeviceSelected();
    void onBoardSelected();
    void onSaveCustomHardware();
    void onImportCustomHardware();
    void onAddCustomPin();
    void onRemoveCustomPin();

    // Step 2: Pinout configuration
    void onPinTableRowSelected();
    void onPinModeChanged(int row, const QString& mode);
    void onPinAfChanged(int row, const QString& af);
    void onPinViewModeChanged(int index);
    void onPinoutWidgetPinClicked(const QString& pinName);

    // Step 3: Peripheral configuration
    void onPeripheralSelected();
    void onPeripheralEnabledToggled(bool enabled);
    void onPeripheralSignalPinChanged(const QString& signal, const QString& pin);
    void onAutoAssignPeripheralPins();

    // Step 4 & 5: Clocks & Toolchain
    void onFrameworkChanged(const QString& framework);

private:
    void setupUi();
    void applyDarkEngineeringTheme();

    // Page Builders
    QWidget* createHardwareSelectionPage();
    QWidget* createAllSelectorTab();
    QWidget* createMcuSelectorTab();
    QWidget* createBoardSelectorTab();
    QWidget* createCustomConfigTab();

    QWidget* createPinoutPage();
    QWidget* createPeripheralPage();
    QWidget* createClockMemoryPage();
    QWidget* createToolchainPage();
    QWidget* createReviewPage();

    // Refreshers
    void populateAllTable();
    void populateDeviceTable();
    void populateBoardTable();
    void populatePinTable();
    void populatePeripheralList();
    void updateSelectedTargetBanner();
    void updateReviewSummary();
    void syncEngineFromTarget();
    void resetFilters();

    // Navigation & State
    QStackedWidget* m_stepsStack = nullptr;
    QLabel* m_stepIndicatorLabel = nullptr;
    QPushButton* m_btnBack = nullptr;
    QPushButton* m_btnNext = nullptr;
    QPushButton* m_btnCancel = nullptr;
    QPushButton* m_btnFinish = nullptr;

    // Step 1 Widgets
    QTabWidget* m_hwTabs = nullptr;
    QLineEdit* m_searchHardwareEdit = nullptr;
    QComboBox* m_typeFilterCombo = nullptr;
    QComboBox* m_vendorFilterCombo = nullptr;
    QComboBox* m_familyFilterCombo = nullptr;
    QComboBox* m_archFilterCombo = nullptr;
    QComboBox* m_coreFilterCombo = nullptr;
    QComboBox* m_packageFilterCombo = nullptr;
    QPushButton* m_btnResetFilters = nullptr;

    QTableWidget* m_allTable = nullptr;
    QTableWidget* m_deviceTable = nullptr;
    QTableWidget* m_boardTable = nullptr;

    // Custom HW Editor
    QLineEdit* m_customVendorEdit = nullptr;
    QLineEdit* m_customFamilyEdit = nullptr;
    QLineEdit* m_customPartEdit = nullptr;
    QLineEdit* m_customArchEdit = nullptr;
    QLineEdit* m_customCoreEdit = nullptr;
    QLineEdit* m_customPackageEdit = nullptr;
    QSpinBox* m_customFlashSpin = nullptr;
    QSpinBox* m_customRamSpin = nullptr;
    QSpinBox* m_customClockSpin = nullptr;
    QTableWidget* m_customPinsTable = nullptr;

    QLabel* m_targetBanner = nullptr;

    // Step 2 Widgets
    QComboBox* m_pinViewModeCombo = nullptr;
    QStackedWidget* m_pinViewStack = nullptr;
    QTableWidget* m_pinTable = nullptr;
    Hardware::PinoutViewWidget* m_pinoutWidget = nullptr;
    QTextEdit* m_pinDetailsLabel = nullptr;

    // Step 3 Widgets
    QTableWidget* m_peripheralListTable = nullptr;
    QGroupBox* m_periDetailsGroup = nullptr;
    QCheckBox* m_periEnabledCheck = nullptr;
    QWidget* m_periPinsContainer = nullptr;
    QVBoxLayout* m_periPinsLayout = nullptr;
    QMap<QString, QComboBox*> m_periSignalCombos;
    QSpinBox* m_periParamBaudOrFreq = nullptr;
    QLabel* m_lblParamBaudOrFreq = nullptr;
    QPushButton* m_btnAutoAssign = nullptr;

    // Step 4 Widgets
    QSpinBox* m_sysClockSpin = nullptr;
    QComboBox* m_oscSourceCombo = nullptr;
    QSpinBox* m_hclkSpin = nullptr;
    QSpinBox* m_flashSizeSpin = nullptr;
    QSpinBox* m_ramSizeSpin = nullptr;

    // Step 5 Widgets
    QLineEdit* m_projectNameEdit = nullptr;
    QSpinBox* m_dispWidthSpin = nullptr;
    QSpinBox* m_dispHeightSpin = nullptr;
    QComboBox* m_frameworkCombo = nullptr;
    QComboBox* m_toolchainCombo = nullptr;
    QComboBox* m_ideCombo = nullptr;

    // Step 6 Widgets
    QTextEdit* m_reviewSummaryEdit = nullptr;
    QLabel* m_validationStatusLabel = nullptr;

    // State
    Hardware::DeviceDatabase& m_db;
    Hardware::PinMuxEngine m_engine;
    Hardware::DeviceDefinition m_selectedDevice;
    Hardware::BoardDefinition m_selectedBoard;
    QString m_selectedTargetType = "device"; // "device", "board", "custom"
};
