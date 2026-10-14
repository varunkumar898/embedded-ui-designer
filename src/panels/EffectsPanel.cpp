#include "EffectsPanel.h"
#include "CanvasScene.h"
#include "ColorPickerDialog.h"
#include "theme/Theme.h"
#include <QUndoStack>
#include <QScrollArea>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <algorithm>

EffectsPanel::EffectsPanel(CanvasScene* scene, QUndoStack* undoStack, QWidget* parent)
    : QWidget(parent)
    , m_scene(scene)
    , m_undoStack(undoStack)
{
    setupUi();
}

void EffectsPanel::setupUi() {
    setObjectName("EffectsPanel");
    setStyleSheet(QString(
        "#EffectsPanel { background-color: %1; color: %2; }"
        "QGroupBox { font-size: 11px; font-weight: 600; color: %3; border: 1px solid %4; border-radius: %5px; margin-top: 8px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }"
        "QLabel { color: %6; font-size: 11px; }"
        "QToolButton, QPushButton { background: %7; color: %2; border: 1px solid %4; border-radius: %8px; padding: 4px 8px; font-size: 11px; font-weight: 500; }"
        "QToolButton:hover, QPushButton:hover { background: %9; border-color: %10; }"
        "QSlider::groove:horizontal { height: 4px; background: %9; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: %11; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: %2; border: 1px solid %11; width: 12px; margin-top: -4px; margin-bottom: -4px; border-radius: 6px; }"
        "QListWidget { background-color: %12; border: 1px solid %4; border-radius: %5px; color: %2; }"
        "QListWidget::item { padding: 6px; border-radius: %8px; }"
        "QListWidget::item:selected { background-color: %13; color: %2; }"
        "QListWidget::item:hover { background-color: %9; }"
    ).arg(Theme::BgSurface, Theme::TextPrimary, Theme::TextSecondary, Theme::BorderSubtle)
     .arg(Theme::RadiusMd)
     .arg(Theme::TextSecondary, Theme::BgRaised)
     .arg(Theme::RadiusSm)
     .arg(Theme::BgOverlay, Theme::BorderStrong, Theme::Accent, Theme::BgBase, Theme::AccentSoft));

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(6, 6, 6, 6);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(createTextEffectsPage());    // 0
    m_stack->addWidget(createImageEffectsPage());   // 1
    m_stack->addWidget(createGenericEffectsPage()); // 2

    rootLayout->addWidget(m_stack);
}

QWidget* EffectsPanel::createTextEffectsPage() {
    QScrollArea* sa = new QScrollArea(this);
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);

    QWidget* container = new QWidget(sa);
    QVBoxLayout* l = new QVBoxLayout(container);
    l->setContentsMargins(4, 4, 4, 4);
    l->setSpacing(8);

    // 1. Text Effects Preset Grid
    QGroupBox* presetsGroup = new QGroupBox("Style Effects", container);
    QVBoxLayout* pgl = new QVBoxLayout(presetsGroup);

    m_textPresetsList = new QListWidget(presetsGroup);
    m_textPresetsList->setViewMode(QListView::IconMode);
    m_textPresetsList->setResizeMode(QListWidget::Adjust);
    m_textPresetsList->setGridSize(QSize(70, 55));
    m_textPresetsList->setIconSize(QSize(28, 28));
    m_textPresetsList->setFixedHeight(125);

    const QStringList presets = {"None", "Shadow", "Glow", "Echo", "Outline", "Background", "Hollow", "Neon", "Splice"};
    for (const QString& p : presets) {
        new QListWidgetItem(p, m_textPresetsList);
    }
    m_textPresetsList->setCurrentRow(0);
    connect(m_textPresetsList, &QListWidget::currentRowChanged, this, &EffectsPanel::onTextPresetSelected);
    pgl->addWidget(m_textPresetsList);
    l->addWidget(presetsGroup);

    // 2. Effect Parameters
    QGroupBox* paramsGroup = new QGroupBox("Effect Controls", container);
    QGridLayout* pgl2 = new QGridLayout(paramsGroup);
    pgl2->setSpacing(6);

    pgl2->addWidget(new QLabel("Color:"), 0, 0);
    m_effectColorBtn = new QToolButton(paramsGroup);
    m_effectColorBtn->setText("Pick Color ■");
    connect(m_effectColorBtn, &QToolButton::clicked, this, &EffectsPanel::onEffectColorClicked);
    pgl2->addWidget(m_effectColorBtn, 0, 1);

    pgl2->addWidget(new QLabel("Offset X:"), 1, 0);
    m_offsetXSlider = new QSlider(Qt::Horizontal, paramsGroup);
    m_offsetXSlider->setRange(-20, 20);
    m_offsetXSlider->setValue(3);
    connect(m_offsetXSlider, &QSlider::valueChanged, this, &EffectsPanel::onEffectOffsetChanged);
    pgl2->addWidget(m_offsetXSlider, 1, 1);

    pgl2->addWidget(new QLabel("Offset Y:"), 2, 0);
    m_offsetYSlider = new QSlider(Qt::Horizontal, paramsGroup);
    m_offsetYSlider->setRange(-20, 20);
    m_offsetYSlider->setValue(3);
    connect(m_offsetYSlider, &QSlider::valueChanged, this, &EffectsPanel::onEffectOffsetChanged);
    pgl2->addWidget(m_offsetYSlider, 2, 1);

    pgl2->addWidget(new QLabel("Thickness:"), 3, 0);
    m_thicknessSlider = new QSlider(Qt::Horizontal, paramsGroup);
    m_thicknessSlider->setRange(1, 15);
    m_thicknessSlider->setValue(2);
    connect(m_thicknessSlider, &QSlider::valueChanged, this, &EffectsPanel::onEffectThicknessChanged);
    pgl2->addWidget(m_thicknessSlider, 3, 1);

    pgl2->addWidget(new QLabel("Opacity:"), 4, 0);
    m_effectOpacitySlider = new QSlider(Qt::Horizontal, paramsGroup);
    m_effectOpacitySlider->setRange(0, 100);
    m_effectOpacitySlider->setValue(100);
    connect(m_effectOpacitySlider, &QSlider::valueChanged, this, &EffectsPanel::onEffectOpacityChanged);
    pgl2->addWidget(m_effectOpacitySlider, 4, 1);

    l->addWidget(paramsGroup);

    // 3. Text Curve Section
    QGroupBox* curveGroup = new QGroupBox("Curve / Arc Bending", container);
    QVBoxLayout* cgl = new QVBoxLayout(curveGroup);

    QHBoxLayout* cRow1 = new QHBoxLayout();
    m_curveSlider = new QSlider(Qt::Horizontal, curveGroup);
    m_curveSlider->setRange(-100, 100);
    m_curveSlider->setValue(0);
    connect(m_curveSlider, &QSlider::valueChanged, this, &EffectsPanel::onCurveSliderChanged);
    cRow1->addWidget(m_curveSlider);

    m_curveValLabel = new QLabel("0%", curveGroup);
    m_curveValLabel->setFixedWidth(36);
    cRow1->addWidget(m_curveValLabel);
    cgl->addLayout(cRow1);

    QHBoxLayout* cPresets = new QHBoxLayout();
    QPushButton* arcUpBtn = new QPushButton("Arc Up (+50%)", curveGroup);
    connect(arcUpBtn, &QPushButton::clicked, this, [this]() { onCurvePresetClicked(50); });
    cPresets->addWidget(arcUpBtn);

    QPushButton* arcDownBtn = new QPushButton("Arc Down (-50%)", curveGroup);
    connect(arcDownBtn, &QPushButton::clicked, this, [this]() { onCurvePresetClicked(-50); });
    cPresets->addWidget(arcDownBtn);

    QPushButton* circBtn = new QPushButton("Circle (100%)", curveGroup);
    connect(circBtn, &QPushButton::clicked, this, [this]() { onCurvePresetClicked(100); });
    cPresets->addWidget(circBtn);

    QPushButton* resetCurveBtn = new QPushButton("Straight (0%)", curveGroup);
    connect(resetCurveBtn, &QPushButton::clicked, this, [this]() { onCurvePresetClicked(0); });
    cPresets->addWidget(resetCurveBtn);

    cgl->addLayout(cPresets);
    l->addWidget(curveGroup);

    l->addStretch();
    sa->setWidget(container);
    return sa;
}

QWidget* EffectsPanel::createImageEffectsPage() {
    QScrollArea* sa = new QScrollArea(this);
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);

    QWidget* container = new QWidget(sa);
    QVBoxLayout* l = new QVBoxLayout(container);
    l->setContentsMargins(4, 4, 4, 4);
    l->setSpacing(8);

    // 1. Color Filters
    QGroupBox* filtersGroup = new QGroupBox("Image Adjustments", container);
    QGridLayout* fgl = new QGridLayout(filtersGroup);

    fgl->addWidget(new QLabel("Brightness:"), 0, 0);
    m_brightSlider = new QSlider(Qt::Horizontal, filtersGroup);
    m_brightSlider->setRange(-100, 100);
    m_brightSlider->setValue(0);
    connect(m_brightSlider, &QSlider::valueChanged, this, &EffectsPanel::onImageBrightnessChanged);
    fgl->addWidget(m_brightSlider, 0, 1);
    m_brightVal = new QLabel("0", filtersGroup);
    fgl->addWidget(m_brightVal, 0, 2);

    fgl->addWidget(new QLabel("Contrast:"), 1, 0);
    m_contrastSlider = new QSlider(Qt::Horizontal, filtersGroup);
    m_contrastSlider->setRange(-100, 100);
    m_contrastSlider->setValue(0);
    connect(m_contrastSlider, &QSlider::valueChanged, this, &EffectsPanel::onImageContrastChanged);
    fgl->addWidget(m_contrastSlider, 1, 1);
    m_contrastVal = new QLabel("0", filtersGroup);
    fgl->addWidget(m_contrastVal, 1, 2);

    fgl->addWidget(new QLabel("Saturation:"), 2, 0);
    m_satSlider = new QSlider(Qt::Horizontal, filtersGroup);
    m_satSlider->setRange(-100, 100);
    m_satSlider->setValue(0);
    connect(m_satSlider, &QSlider::valueChanged, this, &EffectsPanel::onImageSaturationChanged);
    fgl->addWidget(m_satSlider, 2, 1);
    m_satVal = new QLabel("0", filtersGroup);
    fgl->addWidget(m_satVal, 2, 2);

    m_grayCheck = new QCheckBox("Grayscale (Monochrome Filter)", filtersGroup);
    connect(m_grayCheck, &QCheckBox::toggled, this, &EffectsPanel::onImageGrayscaleToggled);
    fgl->addWidget(m_grayCheck, 3, 0, 1, 3);

    QPushButton* resetFiltersBtn = new QPushButton("Reset Adjustments", filtersGroup);
    connect(resetFiltersBtn, &QPushButton::clicked, this, &EffectsPanel::onImageResetAdjustments);
    fgl->addWidget(resetFiltersBtn, 4, 0, 1, 3);

    l->addWidget(filtersGroup);

    // 2. Crop Controls
    QGroupBox* cropGroup = new QGroupBox("Crop Boundaries", container);
    QGridLayout* cgl = new QGridLayout(cropGroup);

    cgl->addWidget(new QLabel("Crop Left %:"), 0, 0);
    m_cropLeftSlider = new QSlider(Qt::Horizontal, cropGroup);
    m_cropLeftSlider->setRange(0, 90);
    connect(m_cropLeftSlider, &QSlider::valueChanged, this, &EffectsPanel::onImageCropChanged);
    cgl->addWidget(m_cropLeftSlider, 0, 1);

    cgl->addWidget(new QLabel("Crop Right %:"), 1, 0);
    m_cropRightSlider = new QSlider(Qt::Horizontal, cropGroup);
    m_cropRightSlider->setRange(0, 90);
    connect(m_cropRightSlider, &QSlider::valueChanged, this, &EffectsPanel::onImageCropChanged);
    cgl->addWidget(m_cropRightSlider, 1, 1);

    cgl->addWidget(new QLabel("Crop Top %:"), 2, 0);
    m_cropTopSlider = new QSlider(Qt::Horizontal, cropGroup);
    m_cropTopSlider->setRange(0, 90);
    connect(m_cropTopSlider, &QSlider::valueChanged, this, &EffectsPanel::onImageCropChanged);
    cgl->addWidget(m_cropTopSlider, 2, 1);

    cgl->addWidget(new QLabel("Crop Bottom %:"), 3, 0);
    m_cropBottomSlider = new QSlider(Qt::Horizontal, cropGroup);
    m_cropBottomSlider->setRange(0, 90);
    connect(m_cropBottomSlider, &QSlider::valueChanged, this, &EffectsPanel::onImageCropChanged);
    cgl->addWidget(m_cropBottomSlider, 3, 1);

    QPushButton* resetCropBtn = new QPushButton("Reset Crop (Full Image)", cropGroup);
    connect(resetCropBtn, &QPushButton::clicked, this, &EffectsPanel::onImageResetCrop);
    cgl->addWidget(resetCropBtn, 4, 0, 1, 2);

    l->addWidget(cropGroup);
    l->addStretch();

    sa->setWidget(container);
    return sa;
}

QWidget* EffectsPanel::createGenericEffectsPage() {
    QWidget* w = new QWidget(this);
    QVBoxLayout* l = new QVBoxLayout(w);
    QLabel* info = new QLabel("Select a text or image component to configure effects.", w);
    info->setWordWrap(true);
    info->setStyleSheet(QString("color: %1; font-style: italic; margin-top: 20px;").arg(Theme::TextMuted));
    l->addWidget(info);
    l->addStretch();
    return w;
}

void EffectsPanel::updateSelection(const QList<UIComponent*>& selected) {
    m_selectedComp = selected.isEmpty() ? nullptr : selected.first();

    if (!m_selectedComp) {
        m_stack->setCurrentIndex(2); // Generic
        return;
    }

    if (dynamic_cast<LabelComponent*>(m_selectedComp)) {
        m_stack->setCurrentIndex(0); // Text
    } else if (dynamic_cast<ImageComponent*>(m_selectedComp)) {
        m_stack->setCurrentIndex(1); // Image
    } else {
        m_stack->setCurrentIndex(2); // Generic
    }

    syncFromSelected();
}

void EffectsPanel::syncFromSelected() {
    if (!m_selectedComp) return;
    m_updatingUi = true;

    if (auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp)) {
        TextEffect eff = lbl->textEffect();
        int row = 0;
        switch (eff.type) {
            case TextEffectType::DropShadow: row = 1; break;
            case TextEffectType::Glow:       row = 2; break;
            case TextEffectType::Echo:       row = 3; break;
            case TextEffectType::Outline:    row = 4; break;
            case TextEffectType::Background: row = 5; break;
            case TextEffectType::Hollow:     row = 6; break;
            case TextEffectType::Neon:       row = 7; break;
            case TextEffectType::Splice:     row = 8; break;
            default:                         row = 0; break;
        }
        m_textPresetsList->setCurrentRow(row);
        m_offsetXSlider->setValue(static_cast<int>(eff.offsetX));
        m_offsetYSlider->setValue(static_cast<int>(eff.offsetY));
        m_thicknessSlider->setValue(static_cast<int>(eff.thickness));
        m_effectOpacitySlider->setValue(static_cast<int>(eff.opacity * 100));

        m_curveSlider->setValue(lbl->curvature());
        m_curveValLabel->setText(QString("%1%").arg(lbl->curvature()));
    } else if (auto img = dynamic_cast<ImageComponent*>(m_selectedComp)) {
        m_brightSlider->setValue(img->brightness());
        m_brightVal->setText(QString::number(img->brightness()));
        m_contrastSlider->setValue(img->contrast());
        m_contrastVal->setText(QString::number(img->contrast()));
        m_satSlider->setValue(img->saturation());
        m_satVal->setText(QString::number(img->saturation()));
        m_grayCheck->setChecked(img->grayscale());

        QRectF cr = img->cropRect();
        m_cropLeftSlider->setValue(static_cast<int>(cr.left() * 100));
        m_cropTopSlider->setValue(static_cast<int>(cr.top() * 100));
        m_cropRightSlider->setValue(static_cast<int>((1.0 - cr.right()) * 100));
        m_cropBottomSlider->setValue(static_cast<int>((1.0 - cr.bottom()) * 100));
    }

    m_updatingUi = false;
}

void EffectsPanel::onTextPresetSelected(int index) {
    if (m_updatingUi) return;
    auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp);
    if (!lbl) return;

    TextEffect eff = lbl->textEffect();
    switch (index) {
        case 1: eff.type = TextEffectType::DropShadow; eff.color = QColor(0, 0, 0, 160); break;
        case 2: eff.type = TextEffectType::Glow;       eff.color = QColor(0, 150, 255, 180); break;
        case 3: eff.type = TextEffectType::Echo;       eff.color = QColor(100, 100, 100, 140); eff.count = 2; break;
        case 4: eff.type = TextEffectType::Outline;    eff.color = Qt::black; eff.thickness = 2.0; break;
        case 5: eff.type = TextEffectType::Background; eff.bgColor = QColor(250, 204, 21); break;
        case 6: eff.type = TextEffectType::Hollow;     eff.thickness = 2.0; break;
        case 7: eff.type = TextEffectType::Neon;       eff.color = QColor(236, 72, 153); break;
        case 8: eff.type = TextEffectType::Splice;     eff.color = Qt::black; eff.thickness = 2.0; break;
        default: eff.type = TextEffectType::None; break;
    }
    lbl->setTextEffect(eff);
}

void EffectsPanel::onEffectColorClicked() {
    auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp);
    if (!lbl) return;
    TextEffect eff = lbl->textEffect();
    ColorPickerDialog dlg(eff.color, this);
    if (dlg.exec() == QDialog::Accepted) {
        eff.color = dlg.selectedColor();
        lbl->setTextEffect(eff);
    }
}

void EffectsPanel::onEffectSecondaryColorClicked() {
    auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp);
    if (!lbl) return;
    TextEffect eff = lbl->textEffect();
    ColorPickerDialog dlg(eff.secondaryColor, this);
    if (dlg.exec() == QDialog::Accepted) {
        eff.secondaryColor = dlg.selectedColor();
        lbl->setTextEffect(eff);
    }
}

void EffectsPanel::onEffectOffsetChanged() {
    if (m_updatingUi) return;
    auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp);
    if (!lbl) return;
    TextEffect eff = lbl->textEffect();
    eff.offsetX = m_offsetXSlider->value();
    eff.offsetY = m_offsetYSlider->value();
    lbl->setTextEffect(eff);
}

void EffectsPanel::onEffectBlurChanged(int val) {
    if (m_updatingUi) return;
    auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp);
    if (!lbl) return;
    TextEffect eff = lbl->textEffect();
    eff.blur = val;
    lbl->setTextEffect(eff);
}

void EffectsPanel::onEffectThicknessChanged(int val) {
    if (m_updatingUi) return;
    auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp);
    if (!lbl) return;
    TextEffect eff = lbl->textEffect();
    eff.thickness = val;
    lbl->setTextEffect(eff);
}

void EffectsPanel::onEffectOpacityChanged(int val) {
    if (m_updatingUi) return;
    auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp);
    if (!lbl) return;
    TextEffect eff = lbl->textEffect();
    eff.opacity = val / 100.0;
    lbl->setTextEffect(eff);
}

void EffectsPanel::onEchoCountChanged(int val) {
    if (m_updatingUi) return;
    auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp);
    if (!lbl) return;
    TextEffect eff = lbl->textEffect();
    eff.count = val;
    lbl->setTextEffect(eff);
}

void EffectsPanel::onBgColorClicked() {
    auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp);
    if (!lbl) return;
    TextEffect eff = lbl->textEffect();
    ColorPickerDialog dlg(eff.bgColor, this);
    if (dlg.exec() == QDialog::Accepted) {
        eff.bgColor = dlg.selectedColor();
        lbl->setTextEffect(eff);
    }
}

void EffectsPanel::onBgPaddingChanged(int val) {
    if (m_updatingUi) return;
    auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp);
    if (!lbl) return;
    TextEffect eff = lbl->textEffect();
    eff.bgPadding = val;
    lbl->setTextEffect(eff);
}

void EffectsPanel::onBgRadiusChanged(int val) {
    if (m_updatingUi) return;
    auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp);
    if (!lbl) return;
    TextEffect eff = lbl->textEffect();
    eff.bgCornerRadius = val;
    lbl->setTextEffect(eff);
}

void EffectsPanel::onCurveSliderChanged(int val) {
    if (m_updatingUi) return;
    m_curveValLabel->setText(QString("%1%").arg(val));
    if (auto lbl = dynamic_cast<LabelComponent*>(m_selectedComp)) {
        lbl->setCurvature(val);
    }
}

void EffectsPanel::onCurvePresetClicked(int val) {
    m_curveSlider->setValue(val);
}

void EffectsPanel::onImageBrightnessChanged(int val) {
    if (m_updatingUi) return;
    m_brightVal->setText(QString::number(val));
    if (auto img = dynamic_cast<ImageComponent*>(m_selectedComp)) {
        img->setBrightness(val);
    }
}

void EffectsPanel::onImageContrastChanged(int val) {
    if (m_updatingUi) return;
    m_contrastVal->setText(QString::number(val));
    if (auto img = dynamic_cast<ImageComponent*>(m_selectedComp)) {
        img->setContrast(val);
    }
}

void EffectsPanel::onImageSaturationChanged(int val) {
    if (m_updatingUi) return;
    m_satVal->setText(QString::number(val));
    if (auto img = dynamic_cast<ImageComponent*>(m_selectedComp)) {
        img->setSaturation(val);
    }
}

void EffectsPanel::onImageGrayscaleToggled(bool checked) {
    if (m_updatingUi) return;
    if (auto img = dynamic_cast<ImageComponent*>(m_selectedComp)) {
        img->setGrayscale(checked);
    }
}

void EffectsPanel::onImageResetAdjustments() {
    m_brightSlider->setValue(0);
    m_contrastSlider->setValue(0);
    m_satSlider->setValue(0);
    m_grayCheck->setChecked(false);
    if (auto img = dynamic_cast<ImageComponent*>(m_selectedComp)) {
        img->setBrightness(0);
        img->setContrast(0);
        img->setSaturation(0);
        img->setGrayscale(false);
    }
}

void EffectsPanel::onImageCropChanged() {
    if (m_updatingUi) return;
    auto img = dynamic_cast<ImageComponent*>(m_selectedComp);
    if (!img) return;

    double left = m_cropLeftSlider->value() / 100.0;
    double top = m_cropTopSlider->value() / 100.0;
    double right = 1.0 - (m_cropRightSlider->value() / 100.0);
    double bottom = 1.0 - (m_cropBottomSlider->value() / 100.0);

    if (right > left && bottom > top) {
        img->setCropRect(QRectF(left, top, right - left, bottom - top));
    }
}

void EffectsPanel::onImageResetCrop() {
    m_cropLeftSlider->setValue(0);
    m_cropTopSlider->setValue(0);
    m_cropRightSlider->setValue(0);
    m_cropBottomSlider->setValue(0);
    if (auto img = dynamic_cast<ImageComponent*>(m_selectedComp)) {
        img->setCropRect(QRectF(0, 0, 1.0, 1.0));
    }
}
