#pragma once

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QJsonObject>
#include "UIComponent.h"

class PropertiesPanel : public QWidget {
    Q_OBJECT

public:
    explicit PropertiesPanel(QWidget* parent = nullptr);

    void setTargetComponent(UIComponent* comp);
    UIComponent* targetComponent() const { return m_targetComponent; }
    void refreshValues();

    void setUndoStack(class QUndoStack* stack) { m_undoStack = stack; }
    class QUndoStack* undoStack() const { return m_undoStack; }

    void commitPropertyChange(const QString& desc);

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
    QWidget* m_contentWidget = nullptr;

    QLabel* m_typeBadge = nullptr;
    QLineEdit* m_idEdit = nullptr;

    // Geometry
    QSpinBox* m_spinX = nullptr;
    QSpinBox* m_spinY = nullptr;
    QSpinBox* m_spinW = nullptr;
    QSpinBox* m_spinH = nullptr;

    // Specific Fields Container
    QGroupBox* m_specificGroup = nullptr;
    QVBoxLayout* m_specificLayout = nullptr;

    // Dynamic controls
    QLineEdit* m_textEdit = nullptr;
    QPushButton* m_colorBtn1 = nullptr;
    QPushButton* m_colorBtn2 = nullptr;
    QSpinBox* m_spinRadius = nullptr;
    QSpinBox* m_spinStrokeW = nullptr;
    QSpinBox* m_spinPixelSize = nullptr;
    QCheckBox* m_chkBold = nullptr;
    QCheckBox* m_chkItalic = nullptr;
    QLineEdit* m_handlerEdit = nullptr;
    QDoubleSpinBox* m_spinProgressValue = nullptr;
    QLineEdit* m_imagePathEdit = nullptr;
    QPushButton* m_browseImageBtn = nullptr;
    QComboBox* m_comboImageFormat = nullptr;
    class QUndoStack* m_undoStack = nullptr;

    void setupUi();
    void rebuildSpecificEditors();
    void updateColorButton(QPushButton* btn, const QColor& color);
};
