#include "ContextualTopToolbar.h"
#include "theme/Theme.h"
#include "CanvasView.h"
#include "CanvasScene.h"
#include "Project.h"
#include "LabelComponent.h"
#include "ImageComponent.h"
#include "RectangleComponent.h"
#include "CircleComponent.h"
#include "ButtonComponent.h"
#include "ProgressBarComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "HardwareBridge.h"
#include "hardware/BoardRegistry.h"
#include "GroupComponent.h"
#include "ColorPickerDialog.h"
#include "PropertyChangeCommand.h"
#include "GroupCommand.h"
#include <QUndoStack>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMenu>
#include <algorithm>

ContextualTopToolbar::ContextualTopToolbar(CanvasView* view, CanvasScene* scene, QUndoStack* undoStack, Project* project, QWidget* parent)
    : QWidget(parent)
    , m_view(view)
    , m_scene(scene)
    , m_undoStack(undoStack)
    , m_project(project)
{
    setupUi();
}

void ContextualTopToolbar::setupUi() {
    setFixedHeight(42);
    setObjectName("ContextualTopToolbar");

    setStyleSheet(QString(
        "#ContextualTopToolbar {"
        "  background-color: %1;"
        "  border-bottom: 1px solid %2;"
        "  padding: 0px 8px;"
        "}"
        "QToolButton, QPushButton {"
        "  background: %3;"
        "  color: %4;"
        "  border: 1px solid %2;"
        "  border-radius: %5px;"
        "  padding: 4px 8px;"
        "  font-size: 11px;"
        "  font-weight: 500;"
        "}"
        "QToolButton:hover, QPushButton:hover {"
        "  background: %6;"
        "  border-color: %7;"
        "  color: %4;"
        "}"
        "QToolButton:checked {"
        "  background: %8;"
        "  border-color: %9;"
        "  color: %4;"
        "}"
        "QComboBox, QSpinBox, QLineEdit {"
        "  background: %10;"
        "  color: %4;"
        "  border: 1px solid %2;"
        "  border-radius: %5px;"
        "  padding: 3px 6px;"
        "  font-size: 11px;"
        "}"
        "QLabel {"
        "  color: %11;"
        "  font-size: 11px;"
        "  font-weight: 500;"
        "}"
        "QSlider::groove:horizontal {"
        "  height: 4px;"
        "  background: %6;"
        "  border-radius: 2px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: %8;"
        "  border-radius: 2px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: %4;"
        "  border: 1px solid %8;"
        "  width: 12px;"
        "  margin-top: -4px;"
        "  margin-bottom: -4px;"
        "  border-radius: 6px;"
        "}")
        .arg(Theme::BgSurface, Theme::BorderSubtle, Theme::BgRaised, Theme::TextPrimary)
        .arg(Theme::RadiusSm)
        .arg(Theme::BgOverlay, Theme::BorderStrong, Theme::Accent, Theme::AccentHover, Theme::BgBase, Theme::TextSecondary)
    );

    QHBoxLayout* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(8, 2, 8, 2);
    rootLayout->setSpacing(6);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(createNoSelectionPage()); // 0
    m_stack->addWidget(createTextPage());        // 1
    m_stack->addWidget(createImagePage());       // 2
    m_stack->addWidget(createShapePage());       // 3
    m_stack->addWidget(createButtonPage());      // 4
    m_stack->addWidget(createProgressPage());    // 5
    m_stack->addWidget(createSliderPage());      // 6
    m_stack->addWidget(createSwitchPage());      // 7
    m_stack->addWidget(createGaugePage());       // 8
    m_stack->addWidget(createMultiSelectPage());  // 9

    rootLayout->addWidget(m_stack);
}

QWidget* ContextualTopToolbar::createNoSelectionPage() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);

    QToolButton* addTextBtn = new QToolButton(w);
    addTextBtn->setText("🔤 Add Text");
    connect(addTextBtn, &QToolButton::clicked, this, &ContextualTopToolbar::addTextRequested);
    l->addWidget(addTextBtn);

    QToolButton* addShapeBtn = new QToolButton(w);
    addShapeBtn->setText("⬛ Add Shape");
    connect(addShapeBtn, &QToolButton::clicked, this, &ContextualTopToolbar::addShapeRequested);
    l->addWidget(addShapeBtn);

    QToolButton* uploadBtn = new QToolButton(w);
    uploadBtn->setText("☁ Upload Image");
    connect(uploadBtn, &QToolButton::clicked, this, &ContextualTopToolbar::openUploadsPanelRequested);
    l->addWidget(uploadBtn);

    l->addSpacing(10);
    QFrame* sep = new QFrame(w);
    sep->setFrameShape(QFrame::VLine);
    sep->setStyleSheet(QString("color: %1;").arg(Theme::BorderSubtle));
    l->addWidget(sep);
    l->addSpacing(10);

    QToolButton* undoBtn = new QToolButton(w);
    undoBtn->setText("↶ Undo");
    connect(undoBtn, &QToolButton::clicked, this, [this]() {
        if (m_undoStack && m_undoStack->canUndo()) m_undoStack->undo();
    });
    l->addWidget(undoBtn);

    QToolButton* redoBtn = new QToolButton(w);
    redoBtn->setText("↷ Redo");
    connect(redoBtn, &QToolButton::clicked, this, [this]() {
        if (m_undoStack && m_undoStack->canRedo()) m_undoStack->redo();
    });
    l->addWidget(redoBtn);

    l->addStretch();
    return w;
}

QWidget* ContextualTopToolbar::createTextPage() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(5);

    // Font family
    m_fontCombo = new QComboBox(w);
    m_fontCombo->addItems(LabelComponent::availableFonts());
    m_fontCombo->setFixedWidth(110);
    connect(m_fontCombo, &QComboBox::currentTextChanged, this, &ContextualTopToolbar::onFontFamilyChanged);
    l->addWidget(m_fontCombo);

    // Font size
    m_fontSizeSpin = new QSpinBox(w);
    m_fontSizeSpin->setRange(6, 144);
    m_fontSizeSpin->setValue(16);
    m_fontSizeSpin->setFixedWidth(55);
    m_fontSizeSpin->setSuffix(" px");
    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ContextualTopToolbar::onFontSizeChanged);
    l->addWidget(m_fontSizeSpin);

    // Style toggles
    m_boldBtn = new QToolButton(w);
    m_boldBtn->setText("B");
    m_boldBtn->setCheckable(true);
    m_boldBtn->setStyleSheet("font-weight: bold; width: 22px;");
    connect(m_boldBtn, &QToolButton::toggled, this, &ContextualTopToolbar::onBoldToggled);
    l->addWidget(m_boldBtn);

    m_italicBtn = new QToolButton(w);
    m_italicBtn->setText("I");
    m_italicBtn->setCheckable(true);
    m_italicBtn->setStyleSheet("font-style: italic; width: 22px;");
    connect(m_italicBtn, &QToolButton::toggled, this, &ContextualTopToolbar::onItalicToggled);
    l->addWidget(m_italicBtn);

    m_underlineBtn = new QToolButton(w);
    m_underlineBtn->setText("U");
    m_underlineBtn->setCheckable(true);
    m_underlineBtn->setStyleSheet("text-decoration: underline; width: 22px;");
    connect(m_underlineBtn, &QToolButton::toggled, this, &ContextualTopToolbar::onUnderlineToggled);
    l->addWidget(m_underlineBtn);

    m_strikeBtn = new QToolButton(w);
    m_strikeBtn->setText("S");
    m_strikeBtn->setCheckable(true);
    m_strikeBtn->setStyleSheet("text-decoration: line-through; width: 22px;");
    connect(m_strikeBtn, &QToolButton::toggled, this, &ContextualTopToolbar::onStrikeThroughToggled);
    l->addWidget(m_strikeBtn);

    // Alignment
    m_alignLeftBtn = new QToolButton(w);
    m_alignLeftBtn->setText("⫷");
    m_alignLeftBtn->setToolTip("Align Left");
    connect(m_alignLeftBtn, &QToolButton::clicked, this, [this]() { onAlignmentClicked(Qt::AlignLeft); });
    l->addWidget(m_alignLeftBtn);

    m_alignCenterBtn = new QToolButton(w);
    m_alignCenterBtn->setText("≡");
    m_alignCenterBtn->setToolTip("Align Center");
    connect(m_alignCenterBtn, &QToolButton::clicked, this, [this]() { onAlignmentClicked(Qt::AlignHCenter); });
    l->addWidget(m_alignCenterBtn);

    m_alignRightBtn = new QToolButton(w);
    m_alignRightBtn->setText("⫸");
    m_alignRightBtn->setToolTip("Align Right");
    connect(m_alignRightBtn, &QToolButton::clicked, this, [this]() { onAlignmentClicked(Qt::AlignRight); });
    l->addWidget(m_alignRightBtn);

    // Text color
    m_textColorBtn = new QToolButton(w);
    m_textColorBtn->setText("Color ■");
    m_textColorBtn->setToolTip("Text Color");
    connect(m_textColorBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onTextColorClicked);
    l->addWidget(m_textColorBtn);

    // Curvature slider
    l->addWidget(new QLabel("Curve:", w));
    m_curveSlider = new QSlider(Qt::Horizontal, w);
    m_curveSlider->setRange(-100, 100);
    m_curveSlider->setValue(0);
    m_curveSlider->setFixedWidth(70);
    connect(m_curveSlider, &QSlider::valueChanged, this, &ContextualTopToolbar::onTextCurvatureChanged);
    l->addWidget(m_curveSlider);

    m_curveLabel = new QLabel("0%", w);
    m_curveLabel->setFixedWidth(32);
    l->addWidget(m_curveLabel);

    // Effects button
    m_textEffectsBtn = new QToolButton(w);
    m_textEffectsBtn->setText("Effects ✨");
    m_textEffectsBtn->setToolTip("Open Text Effects Panel");
    connect(m_textEffectsBtn, &QToolButton::clicked, this, &ContextualTopToolbar::openEffectsPanelRequested);
    l->addWidget(m_textEffectsBtn);

    // Opacity
    l->addWidget(new QLabel("Opacity:", w));
    m_textOpacitySlider = new QSlider(Qt::Horizontal, w);
    m_textOpacitySlider->setRange(0, 100);
    m_textOpacitySlider->setValue(100);
    m_textOpacitySlider->setFixedWidth(65);
    connect(m_textOpacitySlider, &QSlider::valueChanged, this, &ContextualTopToolbar::onComponentOpacityChanged);
    l->addWidget(m_textOpacitySlider);

    m_textOpacityLabel = new QLabel("100%", w);
    m_textOpacityLabel->setFixedWidth(34);
    l->addWidget(m_textOpacityLabel);

    // Position & Transform
    QToolButton* posBtn = new QToolButton(w);
    posBtn->setText("Position 📐");
    connect(posBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onPositionMenuClicked);
    l->addWidget(posBtn);

    l->addStretch();
    return w;
}

QWidget* ContextualTopToolbar::createImagePage() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);

    m_imgReplaceBtn = new QToolButton(w);
    m_imgReplaceBtn->setText("🔄 Replace Image");
    connect(m_imgReplaceBtn, &QToolButton::clicked, this, &ContextualTopToolbar::replaceImageRequested);
    l->addWidget(m_imgReplaceBtn);

    m_imgCropBtn = new QToolButton(w);
    m_imgCropBtn->setText("✂ Crop");
    connect(m_imgCropBtn, &QToolButton::clicked, this, &ContextualTopToolbar::openEffectsPanelRequested);
    l->addWidget(m_imgCropBtn);

    m_imgFlipHBtn = new QToolButton(w);
    m_imgFlipHBtn->setText("⇄ Flip H");
    connect(m_imgFlipHBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onFlipHClicked);
    l->addWidget(m_imgFlipHBtn);

    m_imgFlipVBtn = new QToolButton(w);
    m_imgFlipVBtn->setText("⇅ Flip V");
    connect(m_imgFlipVBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onFlipVClicked);
    l->addWidget(m_imgFlipVBtn);

    m_imgScalingCombo = new QComboBox(w);
    m_imgScalingCombo->addItems({"KeepAspectRatio", "Stretch", "Center"});
    connect(m_imgScalingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ContextualTopToolbar::onImageScalingChanged);
    l->addWidget(m_imgScalingCombo);

    m_imgEffectsBtn = new QToolButton(w);
    m_imgEffectsBtn->setText("Adjustments 🎨");
    connect(m_imgEffectsBtn, &QToolButton::clicked, this, &ContextualTopToolbar::openEffectsPanelRequested);
    l->addWidget(m_imgEffectsBtn);

    l->addWidget(new QLabel("Opacity:", w));
    m_imgOpacitySlider = new QSlider(Qt::Horizontal, w);
    m_imgOpacitySlider->setRange(0, 100);
    m_imgOpacitySlider->setValue(100);
    m_imgOpacitySlider->setFixedWidth(65);
    connect(m_imgOpacitySlider, &QSlider::valueChanged, this, &ContextualTopToolbar::onComponentOpacityChanged);
    l->addWidget(m_imgOpacitySlider);

    m_imgOpacityLabel = new QLabel("100%", w);
    l->addWidget(m_imgOpacityLabel);

    QToolButton* posBtn = new QToolButton(w);
    posBtn->setText("Position 📐");
    connect(posBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onPositionMenuClicked);
    l->addWidget(posBtn);

    l->addStretch();
    return w;
}

QWidget* ContextualTopToolbar::createShapePage() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);

    m_shapeFillBtn = new QToolButton(w);
    m_shapeFillBtn->setText("Fill ■");
    connect(m_shapeFillBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onShapeFillColorClicked);
    l->addWidget(m_shapeFillBtn);

    m_shapeBorderBtn = new QToolButton(w);
    m_shapeBorderBtn->setText("Border ▢");
    connect(m_shapeBorderBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onShapeBorderColorClicked);
    l->addWidget(m_shapeBorderBtn);

    l->addWidget(new QLabel("Border W:", w));
    m_shapeBorderWidthSpin = new QSpinBox(w);
    m_shapeBorderWidthSpin->setRange(0, 30);
    m_shapeBorderWidthSpin->setFixedWidth(50);
    connect(m_shapeBorderWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ContextualTopToolbar::onShapeBorderWidthChanged);
    l->addWidget(m_shapeBorderWidthSpin);

    l->addWidget(new QLabel("Radius:", w));
    m_shapeRadiusSpin = new QSpinBox(w);
    m_shapeRadiusSpin->setRange(0, 100);
    m_shapeRadiusSpin->setFixedWidth(50);
    connect(m_shapeRadiusSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ContextualTopToolbar::onShapeRadiusChanged);
    l->addWidget(m_shapeRadiusSpin);

    m_shapeFlipHBtn = new QToolButton(w);
    m_shapeFlipHBtn->setText("⇄ Flip H");
    connect(m_shapeFlipHBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onFlipHClicked);
    l->addWidget(m_shapeFlipHBtn);

    m_shapeFlipVBtn = new QToolButton(w);
    m_shapeFlipVBtn->setText("⇅ Flip V");
    connect(m_shapeFlipVBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onFlipVClicked);
    l->addWidget(m_shapeFlipVBtn);

    l->addWidget(new QLabel("Opacity:", w));
    m_shapeOpacitySlider = new QSlider(Qt::Horizontal, w);
    m_shapeOpacitySlider->setRange(0, 100);
    m_shapeOpacitySlider->setValue(100);
    m_shapeOpacitySlider->setFixedWidth(65);
    connect(m_shapeOpacitySlider, &QSlider::valueChanged, this, &ContextualTopToolbar::onComponentOpacityChanged);
    l->addWidget(m_shapeOpacitySlider);

    QToolButton* posBtn = new QToolButton(w);
    posBtn->setText("Position 📐");
    connect(posBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onPositionMenuClicked);
    l->addWidget(posBtn);

    l->addStretch();
    return w;
}

QWidget* ContextualTopToolbar::createButtonPage() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);

    l->addWidget(new QLabel("Text:", w));
    m_btnTextEdit = new QLineEdit(w);
    m_btnTextEdit->setFixedWidth(100);
    connect(m_btnTextEdit, &QLineEdit::textChanged, this, &ContextualTopToolbar::onButtonTextChanged);
    l->addWidget(m_btnTextEdit);

    m_btnBgBtn = new QToolButton(w);
    m_btnBgBtn->setText("Bg Color ■");
    connect(m_btnBgBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onButtonBgColorClicked);
    l->addWidget(m_btnBgBtn);

    m_btnTextColBtn = new QToolButton(w);
    m_btnTextColBtn->setText("Text Color ■");
    connect(m_btnTextColBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onButtonTextColorClicked);
    l->addWidget(m_btnTextColBtn);

    l->addWidget(new QLabel("Radius:", w));
    m_btnRadiusSpin = new QSpinBox(w);
    m_btnRadiusSpin->setRange(0, 50);
    m_btnRadiusSpin->setFixedWidth(50);
    connect(m_btnRadiusSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int r) {
        if (auto b = dynamic_cast<ButtonComponent*>(m_primaryComp)) b->setCornerRadius(r);
    });
    l->addWidget(m_btnRadiusSpin);

    l->addWidget(new QLabel("Opacity:", w));
    m_btnOpacitySlider = new QSlider(Qt::Horizontal, w);
    m_btnOpacitySlider->setRange(0, 100);
    m_btnOpacitySlider->setValue(100);
    m_btnOpacitySlider->setFixedWidth(65);
    connect(m_btnOpacitySlider, &QSlider::valueChanged, this, &ContextualTopToolbar::onComponentOpacityChanged);
    l->addWidget(m_btnOpacitySlider);

    QToolButton* posBtn = new QToolButton(w);
    posBtn->setText("Position 📐");
    connect(posBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onPositionMenuClicked);
    l->addWidget(posBtn);

    l->addStretch();
    return w;
}

QWidget* ContextualTopToolbar::createProgressPage() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);

    l->addWidget(new QLabel("Value:", w));
    m_progValSpin = new QSpinBox(w);
    m_progValSpin->setRange(0, 100);
    m_progValSpin->setFixedWidth(55);
    connect(m_progValSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ContextualTopToolbar::onProgressValueChanged);
    l->addWidget(m_progValSpin);

    m_progFillBtn = new QToolButton(w);
    m_progFillBtn->setText("Fill Color ■");
    connect(m_progFillBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onProgressFillColorClicked);
    l->addWidget(m_progFillBtn);

    m_progTrackBtn = new QToolButton(w);
    m_progTrackBtn->setText("Track Color ■");
    connect(m_progTrackBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onProgressTrackColorClicked);
    l->addWidget(m_progTrackBtn);

    l->addWidget(new QLabel("Radius:", w));
    m_progRadiusSpin = new QSpinBox(w);
    m_progRadiusSpin->setRange(0, 50);
    m_progRadiusSpin->setFixedWidth(50);
    connect(m_progRadiusSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int r) {
        if (auto p = dynamic_cast<ProgressBarComponent*>(m_primaryComp)) p->setCornerRadius(r);
    });
    l->addWidget(m_progRadiusSpin);

    m_progFlipHBtn = new QToolButton(w);
    m_progFlipHBtn->setText("⇄ Flip H");
    connect(m_progFlipHBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onFlipHClicked);
    l->addWidget(m_progFlipHBtn);

    m_progFlipVBtn = new QToolButton(w);
    m_progFlipVBtn->setText("⇅ Flip V");
    connect(m_progFlipVBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onFlipVClicked);
    l->addWidget(m_progFlipVBtn);

    l->addWidget(new QLabel("Opacity:", w));
    m_progOpacitySlider = new QSlider(Qt::Horizontal, w);
    m_progOpacitySlider->setRange(0, 100);
    m_progOpacitySlider->setValue(100);
    m_progOpacitySlider->setFixedWidth(65);
    connect(m_progOpacitySlider, &QSlider::valueChanged, this, &ContextualTopToolbar::onComponentOpacityChanged);
    l->addWidget(m_progOpacitySlider);

    QToolButton* posBtn = new QToolButton(w);
    posBtn->setText("Position 📐");
    connect(posBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onPositionMenuClicked);
    l->addWidget(posBtn);

    l->addStretch();
    return w;
}

QWidget* ContextualTopToolbar::createSliderPage() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);

    l->addWidget(new QLabel("Slider Config", w));
    QToolButton* posBtn = new QToolButton(w);
    posBtn->setText("Position 📐");
    connect(posBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onPositionMenuClicked);
    l->addWidget(posBtn);

    l->addStretch();
    return w;
}

QWidget* ContextualTopToolbar::createSwitchPage() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);

    l->addWidget(new QLabel("Pin:", w));
    m_switchPinCombo = new QComboBox(w);
    auto updateToolbarPins = [this]() {
        if (!m_switchPinCombo) return;
        QString current = m_switchPinCombo->currentText();
        m_switchPinCombo->clear();
        QStringList pins = BoardRegistry::instance().activeBoard().pinNames();
        if (pins.isEmpty()) pins = {"PA4", "PA5", "PB0", "PB1", "PA0", "PA1"};
        m_switchPinCombo->addItems(pins);
        int idx = m_switchPinCombo->findText(current);
        if (idx >= 0) m_switchPinCombo->setCurrentIndex(idx);
    };
    updateToolbarPins();
    connect(&BoardRegistry::instance(), &BoardRegistry::activeBoardChanged, m_switchPinCombo, [updateToolbarPins]() {
        updateToolbarPins();
    });
    l->addWidget(m_switchPinCombo);

    m_switchToggleBtn = new QPushButton("Toggle LED", w);
    m_switchToggleBtn->setCheckable(true);
    m_switchToggleBtn->setStyleSheet(QString("QPushButton:checked { background-color: %1; color: %2; }").arg(Theme::Success, Theme::TextPrimary));
    l->addWidget(m_switchToggleBtn);

    m_switchLiveStatusLbl = new QLabel("LED: OFF", w);
    m_switchLiveStatusLbl->setStyleSheet(QString("font-weight: 600; color: %1;").arg(Theme::TextMuted));
    l->addWidget(m_switchLiveStatusLbl);

    connect(m_switchToggleBtn, &QPushButton::toggled, this, [this](bool checked) {
        if (m_updatingUi) return;
        QString pin = m_switchPinCombo ? m_switchPinCombo->currentText() : "PA4";
        if (auto* sw = dynamic_cast<SwitchComponent*>(m_primaryComp)) {
            sw->setChecked(checked);
        }
        HardwareBridge::instance().setDigitalOut(pin, checked);
        m_switchLiveStatusLbl->setText(checked ? "LED: ON (3.3V)" : "LED: OFF (0V)");
        m_switchLiveStatusLbl->setStyleSheet(QString("font-weight: 600; color: %1;").arg(checked ? Theme::Success : Theme::TextMuted));
    });

    QToolButton* posBtn = new QToolButton(w);
    posBtn->setText("Position 📐");
    connect(posBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onPositionMenuClicked);
    l->addWidget(posBtn);

    l->addStretch();
    return w;
}

QWidget* ContextualTopToolbar::createGaugePage() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(6);

    l->addWidget(new QLabel("Gauge / Instrument", w));
    QToolButton* posBtn = new QToolButton(w);
    posBtn->setText("Position 📐");
    connect(posBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onPositionMenuClicked);
    l->addWidget(posBtn);

    l->addStretch();
    return w;
}

QWidget* ContextualTopToolbar::createMultiSelectPage() {
    QWidget* w = new QWidget(this);
    QHBoxLayout* l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(4);

    QLabel* countLbl = new QLabel("Multiple Items", w);
    countLbl->setStyleSheet(QString("font-weight: 600; color: %1; margin-right: 6px;").arg(Theme::AccentText));
    l->addWidget(countLbl);

    QToolButton* aLeft = new QToolButton(w); aLeft->setText("⇤"); aLeft->setToolTip("Align Left");
    connect(aLeft, &QToolButton::clicked, this, &ContextualTopToolbar::onAlignLeft);
    l->addWidget(aLeft);

    QToolButton* aHCen = new QToolButton(w); aHCen->setText("⇥⇤"); aHCen->setToolTip("Align Center");
    connect(aHCen, &QToolButton::clicked, this, &ContextualTopToolbar::onAlignHCenter);
    l->addWidget(aHCen);

    QToolButton* aRight = new QToolButton(w); aRight->setText("⇥"); aRight->setToolTip("Align Right");
    connect(aRight, &QToolButton::clicked, this, &ContextualTopToolbar::onAlignRight);
    l->addWidget(aRight);

    QToolButton* aTop = new QToolButton(w); aTop->setText("⤒"); aTop->setToolTip("Align Top");
    connect(aTop, &QToolButton::clicked, this, &ContextualTopToolbar::onAlignTop);
    l->addWidget(aTop);

    QToolButton* aVCen = new QToolButton(w); aVCen->setText("↕"); aVCen->setToolTip("Align Middle");
    connect(aVCen, &QToolButton::clicked, this, &ContextualTopToolbar::onAlignVCenter);
    l->addWidget(aVCen);

    QToolButton* aBot = new QToolButton(w); aBot->setText("⤓"); aBot->setToolTip("Align Bottom");
    connect(aBot, &QToolButton::clicked, this, &ContextualTopToolbar::onAlignBottom);
    l->addWidget(aBot);

    QToolButton* distH = new QToolButton(w); distH->setText("⋯ Dist H");
    connect(distH, &QToolButton::clicked, this, &ContextualTopToolbar::onDistributeH);
    l->addWidget(distH);

    QToolButton* distV = new QToolButton(w); distV->setText("⋮ Dist V");
    connect(distV, &QToolButton::clicked, this, &ContextualTopToolbar::onDistributeV);
    l->addWidget(distV);

    QToolButton* grpBtn = new QToolButton(w); grpBtn->setText("⊞ Group");
    grpBtn->setStyleSheet(QString("background: %1; color: %2; font-weight: 600; border-radius: %3px;").arg(Theme::Accent, Theme::BgBase).arg(Theme::RadiusSm));
    connect(grpBtn, &QToolButton::clicked, this, &ContextualTopToolbar::onGroupSelection);
    l->addWidget(grpBtn);

    l->addStretch();
    return w;
}

void ContextualTopToolbar::updateSelection(const QList<UIComponent*>& selected) {
    m_selectedComps = selected;
    m_primaryComp = selected.isEmpty() ? nullptr : selected.first();

    if (m_selectedComps.isEmpty()) {
        m_stack->setCurrentIndex(0);
        return;
    }

    if (m_selectedComps.size() > 1) {
        m_stack->setCurrentIndex(9); // Multi-select
        return;
    }

    QString type = m_primaryComp->componentType();
    if (type == "Text" || type == "Label") {
        m_stack->setCurrentIndex(1);
    } else if (type == "Image") {
        m_stack->setCurrentIndex(2);
    } else if (type == "Rectangle" || type == "Circle" || type == "Path") {
        m_stack->setCurrentIndex(3);
    } else if (type == "Button") {
        m_stack->setCurrentIndex(4);
    } else if (type == "ProgressBar" || type == "LinearProgress" || type == "CircularProgress") {
        m_stack->setCurrentIndex(5);
    } else if (type == "Slider") {
        m_stack->setCurrentIndex(6);
    } else if (type == "Switch" || type == "Checkbox") {
        m_stack->setCurrentIndex(7);
    } else if (type == "Gauge" || type == "Speedometer" || type == "Battery" || type == "Pressure" || type == "RPM" || type == "Temperature") {
        m_stack->setCurrentIndex(8);
    } else {
        m_stack->setCurrentIndex(0);
    }

    syncFromSelectedComponent();
}

void ContextualTopToolbar::syncFromSelectedComponent() {
    if (!m_primaryComp) return;
    m_updatingUi = true;

    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        m_fontCombo->setCurrentText(lbl->fontFamily());
        m_fontSizeSpin->setValue(lbl->pixelSize());
        m_boldBtn->setChecked(lbl->bold());
        m_italicBtn->setChecked(lbl->italic());
        m_underlineBtn->setChecked(lbl->underline());
        m_strikeBtn->setChecked(lbl->strikeThrough());
        m_curveSlider->setValue(lbl->curvature());
        m_curveLabel->setText(QString("%1%").arg(lbl->curvature()));
        int op = static_cast<int>(std::round(lbl->opacity() * 100));
        m_textOpacitySlider->setValue(op);
        m_textOpacityLabel->setText(QString("%1%").arg(op));
    } else if (auto img = dynamic_cast<ImageComponent*>(m_primaryComp)) {
        m_imgScalingCombo->setCurrentText(img->scalingMode());
        int op = img->opacityPercent();
        m_imgOpacitySlider->setValue(op);
        m_imgOpacityLabel->setText(QString("%1%").arg(op));
    } else if (auto rect = dynamic_cast<RectangleComponent*>(m_primaryComp)) {
        m_shapeBorderWidthSpin->setValue(rect->borderWidth());
        m_shapeRadiusSpin->setValue(rect->cornerRadius());
        m_shapeOpacitySlider->setValue(static_cast<int>(rect->opacity() * 100));
    } else if (auto btn = dynamic_cast<ButtonComponent*>(m_primaryComp)) {
        m_btnTextEdit->setText(btn->text());
        m_btnRadiusSpin->setValue(btn->cornerRadius());
        m_btnOpacitySlider->setValue(static_cast<int>(btn->opacity() * 100));
    } else if (auto prog = dynamic_cast<ProgressBarComponent*>(m_primaryComp)) {
        m_progValSpin->setValue(prog->value());
        m_progRadiusSpin->setValue(prog->cornerRadius());
        m_progOpacitySlider->setValue(static_cast<int>(prog->opacity() * 100));
    } else if (auto sw = dynamic_cast<SwitchComponent*>(m_primaryComp)) {
        if (m_switchToggleBtn) {
            m_switchToggleBtn->setChecked(sw->isChecked());
        }
        if (m_switchLiveStatusLbl) {
            m_switchLiveStatusLbl->setText(sw->isChecked() ? "LED: ON (3.3V)" : "LED: OFF (0V)");
            m_switchLiveStatusLbl->setStyleSheet(QString("font-weight: 600; color: %1;").arg(sw->isChecked() ? Theme::Success : Theme::TextMuted));
        }
        if (HardwareBridge::instance().boundPinForComponent(sw).isEmpty()) {
            HardwareBridge::instance().bindComponent(sw, "PA4", PinMode::DigitalOut);
        }
    }

    m_updatingUi = false;
}

void ContextualTopToolbar::onFontFamilyChanged(const QString& family) {
    if (m_updatingUi) return;
    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        lbl->setFontFamily(family);
    }
}

void ContextualTopToolbar::onFontSizeChanged(int size) {
    if (m_updatingUi) return;
    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        lbl->setPixelSize(size);
    }
}

void ContextualTopToolbar::onBoldToggled(bool checked) {
    if (m_updatingUi) return;
    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        lbl->setBold(checked);
    }
}

void ContextualTopToolbar::onItalicToggled(bool checked) {
    if (m_updatingUi) return;
    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        lbl->setItalic(checked);
    }
}

void ContextualTopToolbar::onUnderlineToggled(bool checked) {
    if (m_updatingUi) return;
    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        lbl->setUnderline(checked);
    }
}

void ContextualTopToolbar::onStrikeThroughToggled(bool checked) {
    if (m_updatingUi) return;
    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        lbl->setStrikeThrough(checked);
    }
}

void ContextualTopToolbar::onCasingChanged(int index) {
    if (m_updatingUi) return;
    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        QString casing = "none";
        if (index == 1) casing = "uppercase";
        else if (index == 2) casing = "lowercase";
        else if (index == 3) casing = "titlecase";
        lbl->setTextCasing(casing);
    }
}

void ContextualTopToolbar::onAlignmentClicked(int alignFlag) {
    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        lbl->setAlignment(static_cast<Qt::Alignment>(alignFlag | Qt::AlignVCenter));
    }
}

void ContextualTopToolbar::onTextColorClicked() {
    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        ColorPickerDialog dlg(lbl->color(), this);
        if (dlg.exec() == QDialog::Accepted) {
            lbl->setColor(dlg.selectedColor());
        }
    }
}

void ContextualTopToolbar::onTextBgColorClicked() {
    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        ColorPickerDialog dlg(lbl->backgroundColor(), this);
        if (dlg.exec() == QDialog::Accepted) {
            lbl->setBackgroundColor(dlg.selectedColor());
        }
    }
}

void ContextualTopToolbar::onTextCurvatureChanged(int val) {
    if (m_updatingUi) return;
    m_curveLabel->setText(QString("%1%").arg(val));
    if (auto lbl = dynamic_cast<LabelComponent*>(m_primaryComp)) {
        lbl->setCurvature(val);
    }
}

void ContextualTopToolbar::onImageScalingChanged(int index) {
    Q_UNUSED(index);
    if (m_updatingUi) return;
    if (auto img = dynamic_cast<ImageComponent*>(m_primaryComp)) {
        img->setScalingMode(m_imgScalingCombo->currentText());
    }
}

void ContextualTopToolbar::onComponentOpacityChanged(int val) {
    if (m_updatingUi) return;
    qreal op = val / 100.0;
    if (m_textOpacityLabel) m_textOpacityLabel->setText(QString("%1%").arg(val));
    if (m_imgOpacityLabel) m_imgOpacityLabel->setText(QString("%1%").arg(val));

    for (UIComponent* c : m_selectedComps) {
        if (c) {
            c->setOpacity(op);
            if (auto img = dynamic_cast<ImageComponent*>(c)) {
                img->setOpacityPercent(val);
            }
        }
    }
}

void ContextualTopToolbar::onFlipHClicked() {
    for (UIComponent* c : m_selectedComps) {
        if (c) c->setFlipHorizontal(!c->flipHorizontal());
    }
}

void ContextualTopToolbar::onFlipVClicked() {
    for (UIComponent* c : m_selectedComps) {
        if (c) c->setFlipVertical(!c->flipVertical());
    }
}

void ContextualTopToolbar::onShapeFillColorClicked() {
    if (auto rect = dynamic_cast<RectangleComponent*>(m_primaryComp)) {
        ColorPickerDialog dlg(rect->fillColor(), this);
        if (dlg.exec() == QDialog::Accepted) rect->setFillColor(dlg.selectedColor());
    }
}

void ContextualTopToolbar::onShapeBorderColorClicked() {
    if (auto rect = dynamic_cast<RectangleComponent*>(m_primaryComp)) {
        ColorPickerDialog dlg(rect->borderColor(), this);
        if (dlg.exec() == QDialog::Accepted) rect->setBorderColor(dlg.selectedColor());
    }
}

void ContextualTopToolbar::onShapeBorderWidthChanged(int w) {
    if (m_updatingUi) return;
    if (auto rect = dynamic_cast<RectangleComponent*>(m_primaryComp)) rect->setBorderWidth(w);
}

void ContextualTopToolbar::onShapeRadiusChanged(int r) {
    if (m_updatingUi) return;
    if (auto rect = dynamic_cast<RectangleComponent*>(m_primaryComp)) rect->setCornerRadius(r);
}

void ContextualTopToolbar::onProgressValueChanged(int v) {
    if (m_updatingUi) return;
    if (auto prog = dynamic_cast<ProgressBarComponent*>(m_primaryComp)) prog->setValue(v);
}

void ContextualTopToolbar::onProgressFillColorClicked() {
    if (auto prog = dynamic_cast<ProgressBarComponent*>(m_primaryComp)) {
        ColorPickerDialog dlg(prog->fillColor(), this);
        if (dlg.exec() == QDialog::Accepted) prog->setFillColor(dlg.selectedColor());
    }
}

void ContextualTopToolbar::onProgressTrackColorClicked() {
    if (auto prog = dynamic_cast<ProgressBarComponent*>(m_primaryComp)) {
        ColorPickerDialog dlg(prog->trackColor(), this);
        if (dlg.exec() == QDialog::Accepted) prog->setTrackColor(dlg.selectedColor());
    }
}

void ContextualTopToolbar::onButtonTextChanged(const QString& text) {
    if (m_updatingUi) return;
    if (auto btn = dynamic_cast<ButtonComponent*>(m_primaryComp)) btn->setText(text);
}

void ContextualTopToolbar::onButtonBgColorClicked() {
    if (auto btn = dynamic_cast<ButtonComponent*>(m_primaryComp)) {
        ColorPickerDialog dlg(btn->backgroundColor(), this);
        if (dlg.exec() == QDialog::Accepted) btn->setBackgroundColor(dlg.selectedColor());
    }
}

void ContextualTopToolbar::onButtonTextColorClicked() {
    if (auto btn = dynamic_cast<ButtonComponent*>(m_primaryComp)) {
        ColorPickerDialog dlg(btn->textColor(), this);
        if (dlg.exec() == QDialog::Accepted) btn->setTextColor(dlg.selectedColor());
    }
}

void ContextualTopToolbar::onPositionMenuClicked() {
    if (!m_primaryComp) return;

    QDialog dlg(this);
    dlg.setWindowTitle("Component Position & Geometry");
    dlg.resize(280, 220);

    QFormLayout* fl = new QFormLayout(&dlg);
    QSpinBox* xSpin = new QSpinBox(&dlg); xSpin->setRange(-10000, 10000); xSpin->setValue(static_cast<int>(m_primaryComp->pos().x()));
    QSpinBox* ySpin = new QSpinBox(&dlg); ySpin->setRange(-10000, 10000); ySpin->setValue(static_cast<int>(m_primaryComp->pos().y()));
    QSpinBox* wSpin = new QSpinBox(&dlg); wSpin->setRange(5, 10000); wSpin->setValue(static_cast<int>(m_primaryComp->compWidth()));
    QSpinBox* hSpin = new QSpinBox(&dlg); hSpin->setRange(5, 10000); hSpin->setValue(static_cast<int>(m_primaryComp->compHeight()));
    QSpinBox* rotSpin = new QSpinBox(&dlg); rotSpin->setRange(0, 359); rotSpin->setValue(static_cast<int>(m_primaryComp->rotationAngle()));

    fl->addRow("X Position (px):", xSpin);
    fl->addRow("Y Position (px):", ySpin);
    fl->addRow("Width (px):", wSpin);
    fl->addRow("Height (px):", hSpin);
    fl->addRow("Rotation (°):", rotSpin);

    QDialogButtonBox* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    fl->addRow(bb);

    if (dlg.exec() == QDialog::Accepted) {
        m_primaryComp->setCompPos(xSpin->value(), ySpin->value());
        m_primaryComp->setCompSize(wSpin->value(), hSpin->value());
        m_primaryComp->setRotationAngle(rotSpin->value());
    }
}

void ContextualTopToolbar::onAlignLeft() {
    if (m_selectedComps.size() < 2) return;
    qreal minX = std::numeric_limits<qreal>::max();
    for (UIComponent* c : m_selectedComps) minX = std::min(minX, c->pos().x());
    for (UIComponent* c : m_selectedComps) c->setCompPos(minX, c->pos().y());
}

void ContextualTopToolbar::onAlignHCenter() {
    if (m_selectedComps.size() < 2) return;
    qreal minX = std::numeric_limits<qreal>::max(), maxX = std::numeric_limits<qreal>::lowest();
    for (UIComponent* c : m_selectedComps) {
        minX = std::min(minX, c->pos().x());
        maxX = std::max(maxX, c->pos().x() + c->compWidth());
    }
    qreal midX = (minX + maxX) / 2.0;
    for (UIComponent* c : m_selectedComps) c->setCompPos(midX - c->compWidth() / 2.0, c->pos().y());
}

void ContextualTopToolbar::onAlignRight() {
    if (m_selectedComps.size() < 2) return;
    qreal maxX = std::numeric_limits<qreal>::lowest();
    for (UIComponent* c : m_selectedComps) maxX = std::max(maxX, c->pos().x() + c->compWidth());
    for (UIComponent* c : m_selectedComps) c->setCompPos(maxX - c->compWidth(), c->pos().y());
}

void ContextualTopToolbar::onAlignTop() {
    if (m_selectedComps.size() < 2) return;
    qreal minY = std::numeric_limits<qreal>::max();
    for (UIComponent* c : m_selectedComps) minY = std::min(minY, c->pos().y());
    for (UIComponent* c : m_selectedComps) c->setCompPos(c->pos().x(), minY);
}

void ContextualTopToolbar::onAlignVCenter() {
    if (m_selectedComps.size() < 2) return;
    qreal minY = std::numeric_limits<qreal>::max(), maxY = std::numeric_limits<qreal>::lowest();
    for (UIComponent* c : m_selectedComps) {
        minY = std::min(minY, c->pos().y());
        maxY = std::max(maxY, c->pos().y() + c->compHeight());
    }
    qreal midY = (minY + maxY) / 2.0;
    for (UIComponent* c : m_selectedComps) c->setCompPos(c->pos().x(), midY - c->compHeight() / 2.0);
}

void ContextualTopToolbar::onAlignBottom() {
    if (m_selectedComps.size() < 2) return;
    qreal maxY = std::numeric_limits<qreal>::lowest();
    for (UIComponent* c : m_selectedComps) maxY = std::max(maxY, c->pos().y() + c->compHeight());
    for (UIComponent* c : m_selectedComps) c->setCompPos(c->pos().x(), maxY - c->compHeight());
}

void ContextualTopToolbar::onDistributeH() {
    if (m_selectedComps.size() < 3) return;
    auto sorted = m_selectedComps;
    std::sort(sorted.begin(), sorted.end(), [](UIComponent* a, UIComponent* b) { return a->pos().x() < b->pos().x(); });
    qreal minX = sorted.first()->pos().x();
    qreal maxX = sorted.last()->pos().x() + sorted.last()->compWidth();
    qreal totalCompW = 0.0;
    for (UIComponent* c : sorted) totalCompW += c->compWidth();
    qreal gap = (maxX - minX - totalCompW) / (sorted.size() - 1);
    qreal curX = minX;
    for (UIComponent* c : sorted) {
        c->setCompPos(curX, c->pos().y());
        curX += c->compWidth() + gap;
    }
}

void ContextualTopToolbar::onDistributeV() {
    if (m_selectedComps.size() < 3) return;
    auto sorted = m_selectedComps;
    std::sort(sorted.begin(), sorted.end(), [](UIComponent* a, UIComponent* b) { return a->pos().y() < b->pos().y(); });
    qreal minY = sorted.first()->pos().y();
    qreal maxY = sorted.last()->pos().y() + sorted.last()->compHeight();
    qreal totalCompH = 0.0;
    for (UIComponent* c : sorted) totalCompH += c->compHeight();
    qreal gap = (maxY - minY - totalCompH) / (sorted.size() - 1);
    qreal curY = minY;
    for (UIComponent* c : sorted) {
        c->setCompPos(c->pos().x(), curY);
        curY += c->compHeight() + gap;
    }
}

void ContextualTopToolbar::onGroupSelection() {
    if (m_selectedComps.size() <= 1 || !m_scene) return;
    if (m_undoStack) {
        m_undoStack->push(new GroupCommand(m_scene, m_selectedComps));
    } else {
        GroupCommand cmd(m_scene, m_selectedComps);
        cmd.redo();
    }
}
