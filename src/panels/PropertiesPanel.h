#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QJsonObject>
#include <QList>
#include "UIComponent.h"
#include "ComponentDefinition.h"

class Project;

class PropertiesPanel : public QWidget {
    Q_OBJECT

public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

    void setProject(Project* project) { m_project = project; }
    Project* project() const { return m_project; }

    void setTargetComponent(UIComponent* comp);
    UIComponent* targetComponent() const { return m_targetComponent; }
    void refreshValues();

    /// Called when the canvas selection changes to 2+ components.
    void setSelectedComponents(const QList<UIComponent*>& comps);

    void setUndoStack(class QUndoStack* stack) { m_undoStack = stack; }
    class QUndoStack* undoStack() const { return m_undoStack; }

    int specificEditorsLayoutCount() const;

    void commitPropertyChange(const QString& desc);

    friend class TestFunctionalRunner;

signals:
    void alignLeftRequested();
    void alignRightRequested();
    void alignHCenterRequested();
    void alignTopRequested();
    void alignVCenterRequested();
    void alignBottomRequested();
    void distributeHRequested();
    void distributeVRequested();
    // Boolean path operations (2+ shapes selected)
    void booleanUnionRequested();
    void booleanSubtractRequested();
    void booleanIntersectRequested();
    void booleanXorRequested();

private slots:
    void onGeometryChanged();
    void onIdChanged(const QString& newId);
    void onSpecificPropertyChanged();

private:
    UIComponent* m_targetComponent = nullptr;
    class Project* m_project = nullptr;
    bool m_updatingFromComponent = false;
    QJsonObject m_lastSavedState;

    // UI Widgets
    QWidget* m_emptyWidget = nullptr;
    QWidget* m_multiWidget = nullptr;   ///< Shown when 2+ components are selected
    QLabel*  m_multiLabel  = nullptr;   ///< "N components selected" text
    QWidget* m_contentWidget = nullptr;
    QWidget* m_headerWidget = nullptr;
    QWidget* m_alignmentRow = nullptr;
    QWidget* m_booleanOpsRow = nullptr;

    QLabel* m_typeBadge = nullptr;
    QLineEdit* m_idEdit = nullptr;

    // Geometry
    QSpinBox* m_spinX = nullptr;
    QSpinBox* m_spinY = nullptr;
    QSpinBox* m_spinW = nullptr;
    QSpinBox* m_spinH = nullptr;
    QGroupBox* m_geometryGroup = nullptr;
    QList<QToolButton*> m_distributeButtons;

    // Specific Fields Container
    QGroupBox* m_specificGroup = nullptr;
    QVBoxLayout* m_specificLayout = nullptr;
    QWidget* m_specificContainer = nullptr;

    // Dynamic controls
    QLineEdit* m_textEdit = nullptr;
    QPushButton* m_colorBtn1 = nullptr;
    QPushButton* m_colorBtn2 = nullptr;
    QPushButton* m_colorBtn3 = nullptr;
    QPushButton* m_colorBtn4 = nullptr;
    QSpinBox* m_spinRadius = nullptr;
    QSpinBox* m_spinStrokeW = nullptr;
    QSpinBox* m_spinOpacity = nullptr;
    QSpinBox* m_spinPixelSize = nullptr;
    QCheckBox* m_chkBold = nullptr;
    QCheckBox* m_chkItalic = nullptr;
    QLineEdit* m_handlerEdit = nullptr;
    QDoubleSpinBox* m_spinProgressValue = nullptr;
    QDoubleSpinBox* m_spinProgressMin = nullptr;
    QDoubleSpinBox* m_spinProgressMax = nullptr;
    QComboBox* m_comboProgressOrientation = nullptr;
    QLineEdit* m_imagePathEdit = nullptr;
    QPushButton* m_browseImageBtn = nullptr;
    QComboBox* m_comboImageFormat = nullptr;
    QComboBox* m_comboScalingMode = nullptr;

    // Additional specific controls for full coverage
    QSpinBox* m_spinSliderVal = nullptr;
    QSpinBox* m_spinSliderMin = nullptr;
    QSpinBox* m_spinSliderMax = nullptr;
    QCheckBox* m_chkState = nullptr;
    QCheckBox* m_chkVisible = nullptr;
    QCheckBox* m_chkEnabled = nullptr;
    QLineEdit* m_placeholderEdit = nullptr;
    QCheckBox* m_chkReadOnly = nullptr;
    QCheckBox* m_chkFilled = nullptr;
    QSpinBox* m_spinPathFlatten = nullptr;

    // Variant row (shown only for CustomComponentInstance)
    QWidget*  m_variantRow   = nullptr;
    QComboBox* m_variantCombo = nullptr;

    // Alignment buttons (LabelComponent)
    QPushButton* m_btnAlignLeft = nullptr;
    QPushButton* m_btnAlignCenter = nullptr;
    QPushButton* m_btnAlignRight = nullptr;

    // Hardware Protocol Binding
    QGroupBox* m_protocolGroup = nullptr;
    QComboBox* m_protocolCombo = nullptr;
    QWidget* m_protocolNoneWidget = nullptr;
    QWidget* m_gpioWidget = nullptr;
    QComboBox* m_comboGpioPin = nullptr;
    QWidget* m_pwmWidget = nullptr;
    QComboBox* m_comboPwmPin = nullptr;
    QWidget* m_adcWidget = nullptr;
    QComboBox* m_comboAdcPin = nullptr;
    QWidget* m_spiWidget = nullptr;
    QComboBox* m_comboSpiMiso = nullptr;
    QComboBox* m_comboSpiMosi = nullptr;
    QComboBox* m_comboSpiSck = nullptr;
    QComboBox* m_comboSpiSs = nullptr;
    QWidget* m_i2cWidget = nullptr;
    QComboBox* m_comboI2cScl = nullptr;
    QComboBox* m_comboI2cSda = nullptr;
    QLineEdit* m_editI2cAddress = nullptr;
    QPushButton* m_btnScanI2c = nullptr;
    QLabel* m_lblI2cScanStatus = nullptr;
    QComboBox* m_comboDetectedI2cDevices = nullptr;
    QLineEdit* m_editI2cSensorName = nullptr;
    QPushButton* m_btnAssignSensorName = nullptr;

    class QUndoStack* m_undoStack = nullptr;

    void setupUi();
    void rebuildSpecificEditors();
    void updateColorButton(QPushButton* btn, const QColor& color);

    void updateBoardPins();
    void updateProtocolFieldsVisibility(const QString& protocol);
    void onProtocolChanged(const QString& newProtocol);
    void onProtocolPinChanged();
    void onScanI2cBusClicked();
    void onAssignSensorNameClicked();

    void showEmpty();
    void showSingle();
    void showMulti(int count);
};
