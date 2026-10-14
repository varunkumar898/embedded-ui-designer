#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QListWidget>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QToolButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include "UIComponent.h"
#include "LabelComponent.h"
#include "ImageComponent.h"

class CanvasScene;
class QUndoStack;

class EffectsPanel : public QWidget {
    Q_OBJECT

public:
    explicit EffectsPanel(CanvasScene* scene, QUndoStack* undoStack, QWidget* parent = nullptr);
    ~EffectsPanel() override = default;

    void updateSelection(const QList<UIComponent*>& selected);

private slots:
    // Text Effects slots
    void onTextPresetSelected(int index);
    void onEffectColorClicked();
    void onEffectSecondaryColorClicked();
    void onEffectOffsetChanged();
    void onEffectBlurChanged(int val);
    void onEffectThicknessChanged(int val);
    void onEffectOpacityChanged(int val);
    void onEchoCountChanged(int val);
    void onBgColorClicked();
    void onBgPaddingChanged(int val);
    void onBgRadiusChanged(int val);
    void onCurveSliderChanged(int val);
    void onCurvePresetClicked(int val);

    // Image Adjustment slots
    void onImageBrightnessChanged(int val);
    void onImageContrastChanged(int val);
    void onImageSaturationChanged(int val);
    void onImageGrayscaleToggled(bool checked);
    void onImageResetAdjustments();
    void onImageCropChanged();
    void onImageResetCrop();

private:
    void setupUi();
    QWidget* createTextEffectsPage();
    QWidget* createImageEffectsPage();
    QWidget* createGenericEffectsPage();

    void syncFromSelected();

    CanvasScene* m_scene = nullptr;
    QUndoStack* m_undoStack = nullptr;
    UIComponent* m_selectedComp = nullptr;
    bool m_updatingUi = false;

    QStackedWidget* m_stack = nullptr;

    // Text Effect Widgets
    QListWidget* m_textPresetsList = nullptr;
    QSlider* m_offsetXSlider = nullptr;
    QSlider* m_offsetYSlider = nullptr;
    QSlider* m_blurSlider = nullptr;
    QSlider* m_thicknessSlider = nullptr;
    QSlider* m_effectOpacitySlider = nullptr;
    QSlider* m_echoCountSlider = nullptr;
    QToolButton* m_effectColorBtn = nullptr;
    QToolButton* m_effectSecColorBtn = nullptr;
    QToolButton* m_bgColorBtn = nullptr;
    QSpinBox* m_bgPaddingSpin = nullptr;
    QSpinBox* m_bgRadiusSpin = nullptr;

    QSlider* m_curveSlider = nullptr;
    QLabel* m_curveValLabel = nullptr;

    // Image Effect Widgets
    QSlider* m_brightSlider = nullptr;
    QLabel* m_brightVal = nullptr;
    QSlider* m_contrastSlider = nullptr;
    QLabel* m_contrastVal = nullptr;
    QSlider* m_satSlider = nullptr;
    QLabel* m_satVal = nullptr;
    QCheckBox* m_grayCheck = nullptr;

    QSlider* m_cropLeftSlider = nullptr;
    QSlider* m_cropTopSlider = nullptr;
    QSlider* m_cropRightSlider = nullptr;
    QSlider* m_cropBottomSlider = nullptr;
};
