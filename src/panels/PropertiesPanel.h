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

class PropertiesPanel : public QWidget {
    Q_OBJECT

public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

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
    QLineEdit* m_imagePathEdit = nullptr;
    QPushButton* m_browseImageBtn = nullptr;
    QComboBox* m_comboImageFormat = nullptr;

    // Additional specific controls for full coverage
    QSpinBox* m_spinSliderVal = nullptr;
    QSpinBox* m_spinSliderMin = nullptr;
    QSpinBox* m_spinSliderMax = nullptr;
    QCheckBox* m_chkState = nullptr;
    QLineEdit* m_placeholderEdit = nullptr;
    QCheckBox* m_chkReadOnly = nullptr;
    QCheckBox* m_chkFilled = nullptr;
    QSpinBox* m_spinPathFlatten = nullptr;

    // Alignment buttons (LabelComponent)
    QPushButton* m_btnAlignLeft = nullptr;
    QPushButton* m_btnAlignCenter = nullptr;
    QPushButton* m_btnAlignRight = nullptr;

    class QUndoStack* m_undoStack = nullptr;

    void setupUi();
    void rebuildSpecificEditors();
    void updateColorButton(QPushButton* btn, const QColor& color);

    void showEmpty();
    void showSingle();
    void showMulti(int count);
};
