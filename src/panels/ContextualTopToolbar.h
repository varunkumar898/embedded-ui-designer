#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QToolButton>
#include <QComboBox>
#include <QSpinBox>
#include <QSlider>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QList>
#include "UIComponent.h"

class CanvasScene;
class CanvasView;
class QUndoStack;
class Project;

class ContextualTopToolbar : public QWidget {
    Q_OBJECT

public:
    explicit ContextualTopToolbar(CanvasView* view, CanvasScene* scene, QUndoStack* undoStack, Project* project, QWidget* parent = nullptr);
    ~ContextualTopToolbar() override = default;

    void updateSelection(const QList<UIComponent*>& selected);

signals:
    void openEffectsPanelRequested();
    void openUploadsPanelRequested();
    void replaceImageRequested();
    void addTextRequested();
    void addShapeRequested();

private slots:
    // Text Actions
    void onFontFamilyChanged(const QString& family);
    void onFontSizeChanged(int size);
    void onBoldToggled(bool checked);
    void onItalicToggled(bool checked);
    void onUnderlineToggled(bool checked);
    void onStrikeThroughToggled(bool checked);
    void onCasingChanged(int index);
    void onAlignmentClicked(int alignFlag);
    void onTextColorClicked();
    void onTextBgColorClicked();
    void onTextCurvatureChanged(int val);

    // Image Actions
    void onImageScalingChanged(int index);

    // Universal Component Actions
    void onComponentOpacityChanged(int val);
    void onFlipHClicked();
    void onFlipVClicked();
    void onPositionMenuClicked();

    // Shape Actions
    void onShapeFillColorClicked();
    void onShapeBorderColorClicked();
    void onShapeBorderWidthChanged(int w);
    void onShapeRadiusChanged(int r);

    // Progress Bar Actions
    void onProgressValueChanged(int v);
    void onProgressFillColorClicked();
    void onProgressTrackColorClicked();

    // Button Actions
    void onButtonTextChanged(const QString& text);
    void onButtonBgColorClicked();
    void onButtonTextColorClicked();

    // Multi-Selection Align
    void onAlignLeft();
    void onAlignHCenter();
    void onAlignRight();
    void onAlignTop();
    void onAlignVCenter();
    void onAlignBottom();
    void onDistributeH();
    void onDistributeV();
    void onGroupSelection();

private:
    void setupUi();
    QWidget* createNoSelectionPage();
    QWidget* createTextPage();
    QWidget* createImagePage();
    QWidget* createShapePage();
    QWidget* createButtonPage();
    QWidget* createProgressPage();
    QWidget* createSliderPage();
    QWidget* createSwitchPage();
    QWidget* createGaugePage();
    QWidget* createMultiSelectPage();

    void syncFromSelectedComponent();

    CanvasView* m_view = nullptr;
    CanvasScene* m_scene = nullptr;
    QUndoStack* m_undoStack = nullptr;
    Project* m_project = nullptr;
    QList<UIComponent*> m_selectedComps;
    UIComponent* m_primaryComp = nullptr;
    bool m_updatingUi = false;

    QStackedWidget* m_stack = nullptr;

    // Text controls
    QComboBox* m_fontCombo = nullptr;
    QSpinBox* m_fontSizeSpin = nullptr;
    QToolButton* m_boldBtn = nullptr;
    QToolButton* m_italicBtn = nullptr;
    QToolButton* m_underlineBtn = nullptr;
    QToolButton* m_strikeBtn = nullptr;
    QComboBox* m_casingCombo = nullptr;
    QToolButton* m_alignLeftBtn = nullptr;
    QToolButton* m_alignCenterBtn = nullptr;
    QToolButton* m_alignRightBtn = nullptr;
    QToolButton* m_textColorBtn = nullptr;
    QToolButton* m_textBgColorBtn = nullptr;
    QSlider* m_curveSlider = nullptr;
    QLabel* m_curveLabel = nullptr;
    QToolButton* m_textEffectsBtn = nullptr;
    QSlider* m_textOpacitySlider = nullptr;
    QLabel* m_textOpacityLabel = nullptr;

    // Image controls
    QToolButton* m_imgReplaceBtn = nullptr;
    QToolButton* m_imgCropBtn = nullptr;
    QToolButton* m_imgFlipHBtn = nullptr;
    QToolButton* m_imgFlipVBtn = nullptr;
    QComboBox* m_imgScalingCombo = nullptr;
    QToolButton* m_imgEffectsBtn = nullptr;
    QSlider* m_imgOpacitySlider = nullptr;
    QLabel* m_imgOpacityLabel = nullptr;

    // Shape controls
    QToolButton* m_shapeFillBtn = nullptr;
    QToolButton* m_shapeBorderBtn = nullptr;
    QSpinBox* m_shapeBorderWidthSpin = nullptr;
    QSpinBox* m_shapeRadiusSpin = nullptr;
    QToolButton* m_shapeFlipHBtn = nullptr;
    QToolButton* m_shapeFlipVBtn = nullptr;
    QSlider* m_shapeOpacitySlider = nullptr;

    // Button controls
    QLineEdit* m_btnTextEdit = nullptr;
    QToolButton* m_btnBgBtn = nullptr;
    QToolButton* m_btnTextColBtn = nullptr;
    QSpinBox* m_btnRadiusSpin = nullptr;
    QSlider* m_btnOpacitySlider = nullptr;

    // Progress controls
    QSpinBox* m_progValSpin = nullptr;
    QToolButton* m_progFillBtn = nullptr;
    QToolButton* m_progTrackBtn = nullptr;
    QSpinBox* m_progRadiusSpin = nullptr;
    QToolButton* m_progFlipHBtn = nullptr;
    QToolButton* m_progFlipVBtn = nullptr;
    QSlider* m_progOpacitySlider = nullptr;

    // Switch & GPIO controls
    QComboBox* m_switchPinCombo = nullptr;
    QPushButton* m_switchToggleBtn = nullptr;
    QLabel* m_switchLiveStatusLbl = nullptr;
};
