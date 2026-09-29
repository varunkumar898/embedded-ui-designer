#include "PropertiesPanel.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include "ImageComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include "PropertyChangeCommand.h"
#include <QUndoStack>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QColorDialog>
#include <QScrollArea>
#include <QFileDialog>

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    setTargetComponent(nullptr);
}

void PropertiesPanel::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // Empty state
    m_emptyWidget = new QWidget(this);
    QVBoxLayout* emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setContentsMargins(20, 40, 20, 20);
    QLabel* emptyLabel = new QLabel("No Component Selected\n\nClick an element on the canvas to inspect and edit its properties.", m_emptyWidget);
    emptyLabel->setWordWrap(true);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: #717C8F; font-size: 13px; line-height: 1.4;");
    emptyLayout->addWidget(emptyLabel);
    rootLayout->addWidget(m_emptyWidget);

    // Content container inside scroll area
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background-color: transparent;");

    m_contentWidget = new QWidget(scrollArea);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_contentWidget);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // Header: Type badge & ID
    QHBoxLayout* headerLayout = new QHBoxLayout();
    m_typeBadge = new QLabel("COMPONENT", m_contentWidget);
    m_typeBadge->setStyleSheet("background-color: #2196F3; color: white; padding: 4px 8px; border-radius: 4px; font-weight: bold; font-size: 11px;");
    m_idEdit = new QLineEdit(m_contentWidget);
    m_idEdit->setStyleSheet("background-color: #252830; color: #FFFFFF; border: 1px solid #3B404E; border-radius: 4px; padding: 4px 8px; font-weight: bold;");
    headerLayout->addWidget(m_typeBadge);
    headerLayout->addWidget(m_idEdit, 1);
    mainLayout->addLayout(headerLayout);

    connect(m_idEdit, &QLineEdit::textChanged, this, &PropertiesPanel::onIdChanged);
    connect(m_idEdit, &QLineEdit::editingFinished, this, [this]() {
        commitPropertyChange("Rename Component ID");
    });

    // Geometry Group
    QGroupBox* geomGroup = new QGroupBox("Transform & Geometry", m_contentWidget);
    geomGroup->setStyleSheet("QGroupBox { color: #9AA5B8; font-size: 11px; font-weight: bold; border: 1px solid #3B404E; border-radius: 6px; margin-top: 10px; padding-top: 14px; } QGroupBox::title { subcontrol-origin: margin; left: 8px; }");
    QGridLayout* geomLayout = new QGridLayout(geomGroup);
    geomLayout->setSpacing(8);

    auto makeSpin = [this](int minVal, int maxVal) {
        QSpinBox* spin = new QSpinBox(m_contentWidget);
        spin->setRange(minVal, maxVal);
        spin->setStyleSheet("QSpinBox { background-color: #252830; color: #FFFFFF; border: 1px solid #3B404E; border-radius: 4px; padding: 2px 4px; }");
        return spin;
    };

    m_spinX = makeSpin(-2000, 2000);
    m_spinY = makeSpin(-2000, 2000);
    m_spinW = makeSpin(10, 2000);
    m_spinH = makeSpin(10, 2000);

    geomLayout->addWidget(new QLabel("X:", geomGroup), 0, 0);
    geomLayout->addWidget(m_spinX, 0, 1);
    geomLayout->addWidget(new QLabel("Y:", geomGroup), 0, 2);
    geomLayout->addWidget(m_spinY, 0, 3);
    geomLayout->addWidget(new QLabel("W:", geomGroup), 1, 0);
    geomLayout->addWidget(m_spinW, 1, 1);
    geomLayout->addWidget(new QLabel("H:", geomGroup), 1, 2);
    geomLayout->addWidget(m_spinH, 1, 3);

    mainLayout->addWidget(geomGroup);

    connect(m_spinX, QOverload<int>::of(&QSpinBox::valueChanged), this, &PropertiesPanel::onGeometryChanged);
    connect(m_spinY, QOverload<int>::of(&QSpinBox::valueChanged), this, &PropertiesPanel::onGeometryChanged);
    connect(m_spinW, QOverload<int>::of(&QSpinBox::valueChanged), this, &PropertiesPanel::onGeometryChanged);
    connect(m_spinH, QOverload<int>::of(&QSpinBox::valueChanged), this, &PropertiesPanel::onGeometryChanged);

    connect(m_spinX, &QSpinBox::editingFinished, this, [this]() { commitPropertyChange("Change Position X"); });
    connect(m_spinY, &QSpinBox::editingFinished, this, [this]() { commitPropertyChange("Change Position Y"); });
    connect(m_spinW, &QSpinBox::editingFinished, this, [this]() { commitPropertyChange("Change Width"); });
    connect(m_spinH, &QSpinBox::editingFinished, this, [this]() { commitPropertyChange("Change Height"); });

    // Specific Properties Group
    m_specificGroup = new QGroupBox("Component Properties", m_contentWidget);
    m_specificGroup->setStyleSheet("QGroupBox { color: #9AA5B8; font-size: 11px; font-weight: bold; border: 1px solid #3B404E; border-radius: 6px; margin-top: 10px; padding-top: 14px; } QGroupBox::title { subcontrol-origin: margin; left: 8px; }");
    m_specificLayout = new QVBoxLayout(m_specificGroup);
    m_specificLayout->setSpacing(8);
    mainLayout->addWidget(m_specificGroup);

    mainLayout->addStretch(1);
    scrollArea->setWidget(m_contentWidget);
    rootLayout->addWidget(scrollArea);
}

void PropertiesPanel::setTargetComponent(UIComponent* comp) {
    m_targetComponent = comp;
    m_lastSavedState = comp ? comp->toJson() : QJsonObject();

    if (!m_targetComponent) {
        m_emptyWidget->setVisible(true);
        m_contentWidget->setVisible(false);
        return;
    }

    m_emptyWidget->setVisible(false);
    m_contentWidget->setVisible(true);

    m_typeBadge->setText(m_targetComponent->componentType().toUpper());
    rebuildSpecificEditors();
    refreshValues();
}

void PropertiesPanel::commitPropertyChange(const QString& desc) {
    if (!m_targetComponent || m_updatingFromComponent) return;
    QJsonObject currentState = m_targetComponent->toJson();
    if (currentState != m_lastSavedState) {
        if (m_undoStack) {
            m_undoStack->push(new PropertyChangeCommand(m_targetComponent, m_lastSavedState, currentState, desc));
        }
        m_lastSavedState = currentState;
    }
}

void PropertiesPanel::refreshValues() {
    if (!m_targetComponent) return;

    m_updatingFromComponent = true;

    m_idEdit->setText(m_targetComponent->componentId());
    m_spinX->setValue(static_cast<int>(m_targetComponent->compX()));
    m_spinY->setValue(static_cast<int>(m_targetComponent->compY()));
    m_spinW->setValue(static_cast<int>(m_targetComponent->compWidth()));
    m_spinH->setValue(static_cast<int>(m_targetComponent->compHeight()));

    if (auto btn = dynamic_cast<ButtonComponent*>(m_targetComponent)) {
        if (m_textEdit) m_textEdit->setText(btn->text());
        if (m_colorBtn1) updateColorButton(m_colorBtn1, btn->backgroundColor());
        if (m_colorBtn2) updateColorButton(m_colorBtn2, btn->textColor());
        if (m_spinRadius) m_spinRadius->setValue(btn->cornerRadius());
        if (m_handlerEdit) m_handlerEdit->setText(btn->onClickedHandler());
    } else if (auto lbl = dynamic_cast<LabelComponent*>(m_targetComponent)) {
        if (m_textEdit) m_textEdit->setText(lbl->text());
        if (m_colorBtn1) updateColorButton(m_colorBtn1, lbl->color());
        if (m_spinPixelSize) m_spinPixelSize->setValue(lbl->pixelSize());
        if (m_chkBold) m_chkBold->setChecked(lbl->bold());
        if (m_chkItalic) m_chkItalic->setChecked(lbl->italic());
    } else if (auto rect = dynamic_cast<RectangleComponent*>(m_targetComponent)) {
        if (m_colorBtn1) updateColorButton(m_colorBtn1, rect->fillColor());
        if (m_colorBtn2) updateColorButton(m_colorBtn2, rect->strokeColor());
        if (m_spinStrokeW) m_spinStrokeW->setValue(rect->strokeWidth());
        if (m_spinRadius) m_spinRadius->setValue(rect->cornerRadius());
    } else if (auto prog = dynamic_cast<ProgressBarComponent*>(m_targetComponent)) {
        if (m_spinProgressValue) m_spinProgressValue->setValue(prog->value());
        if (m_colorBtn1) updateColorButton(m_colorBtn1, prog->barColor());
        if (m_colorBtn2) updateColorButton(m_colorBtn2, prog->trackColor());
        if (m_spinRadius) m_spinRadius->setValue(prog->cornerRadius());
    } else if (auto img = dynamic_cast<ImageComponent*>(m_targetComponent)) {
        if (m_imagePathEdit) m_imagePathEdit->setText(img->imagePath());
        if (m_comboImageFormat) m_comboImageFormat->setCurrentText(img->format());
    }

    m_lastSavedState = m_targetComponent->toJson();
    m_updatingFromComponent = false;
}

void PropertiesPanel::updateColorButton(QPushButton* btn, const QColor& color) {
    if (!btn) return;
    btn->setText(color.name().toUpper());
    btn->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; font-weight: bold; border: 1px solid #3B404E; border-radius: 4px; padding: 4px 8px; }"
    ).arg(color.name(), (color.lightness() > 140 ? "#000000" : "#FFFFFF")));
}

void PropertiesPanel::rebuildSpecificEditors() {
    QLayoutItem* item;
    while ((item = m_specificLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    m_textEdit = nullptr;
    m_colorBtn1 = nullptr;
    m_colorBtn2 = nullptr;
    m_spinRadius = nullptr;
    m_spinStrokeW = nullptr;
    m_spinPixelSize = nullptr;
    m_chkBold = nullptr;
    m_chkItalic = nullptr;
    m_handlerEdit = nullptr;
    m_spinProgressValue = nullptr;
    m_imagePathEdit = nullptr;
    m_browseImageBtn = nullptr;
    m_comboImageFormat = nullptr;

    if (!m_targetComponent) return;

    QFormLayout* form = new QFormLayout();
    form->setSpacing(8);

    auto addColorRow = [this, form](const QString& label, const QColor& initialColor, auto setter, const QString& desc) {
        QPushButton* btn = new QPushButton(this);
        updateColorButton(btn, initialColor);
        connect(btn, &QPushButton::clicked, this, [this, btn, setter, desc]() {
            if (!m_targetComponent) return;
            QColor current(btn->text());
            QColor picked = QColorDialog::getColor(current, this, "Choose Color");
            if (picked.isValid()) {
                updateColorButton(btn, picked);
                setter(picked);
                commitPropertyChange(desc);
            }
        });
        form->addRow(label, btn);
        return btn;
    };

    if (auto btn = dynamic_cast<ButtonComponent*>(m_targetComponent)) {
        m_textEdit = new QLineEdit(btn->text(), this);
        connect(m_textEdit, &QLineEdit::textChanged, this, [this, btn](const QString& t) {
            if (!m_updatingFromComponent) btn->setText(t);
        });
        connect(m_textEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Button Text");
        });
        form->addRow("Text:", m_textEdit);

        m_colorBtn1 = addColorRow("Background:", btn->backgroundColor(), [btn](const QColor& c) { btn->setBackgroundColor(c); }, "Change Background Color");
        m_colorBtn2 = addColorRow("Text Color:", btn->textColor(), [btn](const QColor& c) { btn->setTextColor(c); }, "Change Text Color");

        m_spinRadius = new QSpinBox(this);
        m_spinRadius->setRange(0, 50);
        m_spinRadius->setValue(btn->cornerRadius());
        connect(m_spinRadius, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, btn](int v) {
            if (!m_updatingFromComponent) btn->setCornerRadius(v);
        });
        connect(m_spinRadius, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Corner Radius");
        });
        form->addRow("Radius:", m_spinRadius);

        m_handlerEdit = new QLineEdit(btn->onClickedHandler(), this);
        connect(m_handlerEdit, &QLineEdit::textChanged, this, [this, btn](const QString& h) {
            if (!m_updatingFromComponent) btn->setOnClickedHandler(h);
        });
        connect(m_handlerEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Click Handler");
        });
        form->addRow("OnClicked:", m_handlerEdit);

    } else if (auto lbl = dynamic_cast<LabelComponent*>(m_targetComponent)) {
        m_textEdit = new QLineEdit(lbl->text(), this);
        connect(m_textEdit, &QLineEdit::textChanged, this, [this, lbl](const QString& t) {
            if (!m_updatingFromComponent) lbl->setText(t);
        });
        connect(m_textEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Label Text");
        });
        form->addRow("Text:", m_textEdit);

        m_colorBtn1 = addColorRow("Color:", lbl->color(), [lbl](const QColor& c) { lbl->setColor(c); }, "Change Label Color");

        m_spinPixelSize = new QSpinBox(this);
        m_spinPixelSize->setRange(6, 96);
        m_spinPixelSize->setValue(lbl->pixelSize());
        connect(m_spinPixelSize, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, lbl](int v) {
            if (!m_updatingFromComponent) lbl->setPixelSize(v);
        });
        connect(m_spinPixelSize, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Font Size");
        });
        form->addRow("Font Size:", m_spinPixelSize);

        QHBoxLayout* fontStyles = new QHBoxLayout();
        m_chkBold = new QCheckBox("Bold", this);
        m_chkBold->setChecked(lbl->bold());
        connect(m_chkBold, &QCheckBox::toggled, this, [this, lbl](bool b) {
            if (!m_updatingFromComponent) {
                lbl->setBold(b);
                commitPropertyChange("Toggle Bold");
            }
        });
        m_chkItalic = new QCheckBox("Italic", this);
        m_chkItalic->setChecked(lbl->italic());
        connect(m_chkItalic, &QCheckBox::toggled, this, [this, lbl](bool i) {
            if (!m_updatingFromComponent) {
                lbl->setItalic(i);
                commitPropertyChange("Toggle Italic");
            }
        });
        fontStyles->addWidget(m_chkBold);
        fontStyles->addWidget(m_chkItalic);
        form->addRow("Style:", fontStyles);

    } else if (auto rect = dynamic_cast<RectangleComponent*>(m_targetComponent)) {
        m_colorBtn1 = addColorRow("Fill Color:", rect->fillColor(), [rect](const QColor& c) { rect->setFillColor(c); }, "Change Fill Color");
        m_colorBtn2 = addColorRow("Stroke Color:", rect->strokeColor(), [rect](const QColor& c) { rect->setStrokeColor(c); }, "Change Stroke Color");

        m_spinStrokeW = new QSpinBox(this);
        m_spinStrokeW->setRange(0, 20);
        m_spinStrokeW->setValue(rect->strokeWidth());
        connect(m_spinStrokeW, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, rect](int v) {
            if (!m_updatingFromComponent) rect->setStrokeWidth(v);
        });
        connect(m_spinStrokeW, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Stroke Width");
        });
        form->addRow("Stroke Width:", m_spinStrokeW);

        m_spinRadius = new QSpinBox(this);
        m_spinRadius->setRange(0, 50);
        m_spinRadius->setValue(rect->cornerRadius());
        connect(m_spinRadius, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, rect](int v) {
            if (!m_updatingFromComponent) rect->setCornerRadius(v);
        });
        connect(m_spinRadius, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Corner Radius");
        });
        form->addRow("Corner Radius:", m_spinRadius);

    } else if (auto prog = dynamic_cast<ProgressBarComponent*>(m_targetComponent)) {
        m_spinProgressValue = new QDoubleSpinBox(this);
        m_spinProgressValue->setRange(0.0, 1.0);
        m_spinProgressValue->setSingleStep(0.05);
        m_spinProgressValue->setValue(prog->value());
        connect(m_spinProgressValue, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, prog](double v) {
            if (!m_updatingFromComponent) prog->setValue(v);
        });
        connect(m_spinProgressValue, &QDoubleSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Progress Value");
        });
        form->addRow("Progress (0-1):", m_spinProgressValue);

        m_colorBtn1 = addColorRow("Bar Color:", prog->barColor(), [prog](const QColor& c) { prog->setBarColor(c); }, "Change Bar Color");
        m_colorBtn2 = addColorRow("Track Color:", prog->trackColor(), [prog](const QColor& c) { prog->setTrackColor(c); }, "Change Track Color");

        m_spinRadius = new QSpinBox(this);
        m_spinRadius->setRange(0, 20);
        m_spinRadius->setValue(prog->cornerRadius());
        connect(m_spinRadius, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, prog](int v) {
            if (!m_updatingFromComponent) prog->setCornerRadius(v);
        });
        connect(m_spinRadius, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Corner Radius");
        });
        form->addRow("Radius:", m_spinRadius);

    } else if (auto img = dynamic_cast<ImageComponent*>(m_targetComponent)) {
        QHBoxLayout* pathLayout = new QHBoxLayout();
        m_imagePathEdit = new QLineEdit(img->imagePath(), this);
        m_browseImageBtn = new QPushButton("Browse...", this);
        pathLayout->addWidget(m_imagePathEdit, 1);
        pathLayout->addWidget(m_browseImageBtn);

        connect(m_imagePathEdit, &QLineEdit::textChanged, this, [this, img](const QString& p) {
            if (!m_updatingFromComponent) img->setImagePath(p);
        });
        connect(m_imagePathEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Image Path");
        });

        connect(m_browseImageBtn, &QPushButton::clicked, this, [this, img]() {
            QString path = QFileDialog::getOpenFileName(this, "Select Image", "", "Images (*.png *.jpg *.bmp);;All Files (*)");
            if (!path.isEmpty()) {
                m_imagePathEdit->setText(path);
                img->setImagePath(path);
                commitPropertyChange("Change Image Path");
            }
        });
        form->addRow("Image File:", pathLayout);

        m_comboImageFormat = new QComboBox(this);
        m_comboImageFormat->addItem("RGB565");
        m_comboImageFormat->addItem("Monochrome");
        m_comboImageFormat->setCurrentText(img->format());
        connect(m_comboImageFormat, &QComboBox::currentTextChanged, this, [this, img](const QString& fmt) {
            if (!m_updatingFromComponent) {
                img->setFormat(fmt);
                commitPropertyChange("Change Image Format");
            }
        });
        form->addRow("Color Format:", m_comboImageFormat);
    } else if (auto slider = dynamic_cast<SliderComponent*>(m_targetComponent)) {
        QSpinBox* spinVal = new QSpinBox(this);
        spinVal->setRange(slider->minimum(), slider->maximum());
        spinVal->setValue(slider->value());
        connect(spinVal, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, slider](int v) {
            if (!m_updatingFromComponent) slider->setValue(v);
        });
        connect(spinVal, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Slider Value");
        });
        form->addRow("Value:", spinVal);

        QSpinBox* spinMin = new QSpinBox(this);
        spinMin->setRange(-10000, 10000);
        spinMin->setValue(slider->minimum());
        connect(spinMin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, slider](int v) {
            if (!m_updatingFromComponent) slider->setMinimum(v);
        });
        connect(spinMin, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Slider Minimum");
        });
        form->addRow("Minimum:", spinMin);

        QSpinBox* spinMax = new QSpinBox(this);
        spinMax->setRange(-10000, 10000);
        spinMax->setValue(slider->maximum());
        connect(spinMax, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, slider](int v) {
            if (!m_updatingFromComponent) slider->setMaximum(v);
        });
        connect(spinMax, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Slider Maximum");
        });
        form->addRow("Maximum:", spinMax);

        m_colorBtn1 = addColorRow("Track Color:", slider->trackColor(), [slider](const QColor& c) { slider->setTrackColor(c); }, "Change Track Color");
        m_colorBtn2 = addColorRow("Fill Color:", slider->fillColor(), [slider](const QColor& c) { slider->setFillColor(c); }, "Change Fill Color");
        addColorRow("Handle Color:", slider->handleColor(), [slider](const QColor& c) { slider->setHandleColor(c); }, "Change Handle Color");
    } else if (auto sw = dynamic_cast<SwitchComponent*>(m_targetComponent)) {
        QCheckBox* chkState = new QCheckBox("Checked", this);
        chkState->setChecked(sw->isChecked());
        connect(chkState, &QCheckBox::toggled, this, [this, sw](bool b) {
            if (!m_updatingFromComponent) {
                sw->setChecked(b);
                commitPropertyChange("Toggle Switch");
            }
        });
        form->addRow("State:", chkState);

        addColorRow("On Color:", sw->onColor(), [sw](const QColor& c) { sw->setOnColor(c); }, "Change On Color");
        addColorRow("Off Color:", sw->offColor(), [sw](const QColor& c) { sw->setOffColor(c); }, "Change Off Color");
        addColorRow("Thumb Color:", sw->thumbColor(), [sw](const QColor& c) { sw->setThumbColor(c); }, "Change Thumb Color");

        QLineEdit* handlerEdit = new QLineEdit(sw->onToggledHandler(), this);
        connect(handlerEdit, &QLineEdit::textChanged, this, [this, sw](const QString& h) {
            if (!m_updatingFromComponent) sw->setOnToggledHandler(h);
        });
        connect(handlerEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Toggled Handler");
        });
        form->addRow("OnToggled:", handlerEdit);
    } else if (auto chk = dynamic_cast<CheckboxComponent*>(m_targetComponent)) {
        QLineEdit* txtEdit = new QLineEdit(chk->text(), this);
        connect(txtEdit, &QLineEdit::textChanged, this, [this, chk](const QString& t) {
            if (!m_updatingFromComponent) chk->setText(t);
        });
        connect(txtEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Checkbox Text");
        });
        form->addRow("Text:", txtEdit);

        QCheckBox* chkState = new QCheckBox("Checked", this);
        chkState->setChecked(chk->isChecked());
        connect(chkState, &QCheckBox::toggled, this, [this, chk](bool b) {
            if (!m_updatingFromComponent) {
                chk->setChecked(b);
                commitPropertyChange("Toggle Checkbox");
            }
        });
        form->addRow("State:", chkState);

        addColorRow("Text Color:", chk->textColor(), [chk](const QColor& c) { chk->setTextColor(c); }, "Change Text Color");
        addColorRow("Check Color:", chk->checkColor(), [chk](const QColor& c) { chk->setCheckColor(c); }, "Change Check Color");
        addColorRow("Box Color:", chk->boxColor(), [chk](const QColor& c) { chk->setBoxColor(c); }, "Change Box Color");
        addColorRow("Border Color:", chk->borderColor(), [chk](const QColor& c) { chk->setBorderColor(c); }, "Change Border Color");

        QLineEdit* handlerEdit = new QLineEdit(chk->onToggledHandler(), this);
        connect(handlerEdit, &QLineEdit::textChanged, this, [this, chk](const QString& h) {
            if (!m_updatingFromComponent) chk->setOnToggledHandler(h);
        });
        connect(handlerEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Toggled Handler");
        });
        form->addRow("OnToggled:", handlerEdit);
    }

    m_specificLayout->addLayout(form);
}

void PropertiesPanel::onGeometryChanged() {
    if (m_updatingFromComponent || !m_targetComponent) return;
    m_targetComponent->setCompPos(m_spinX->value(), m_spinY->value());
    m_targetComponent->setCompSize(m_spinW->value(), m_spinH->value());
}

void PropertiesPanel::onIdChanged(const QString& newId) {
    if (m_updatingFromComponent || !m_targetComponent || newId.trimmed().isEmpty()) return;
    m_targetComponent->setComponentId(newId.trimmed());
}

void PropertiesPanel::onSpecificPropertyChanged() {
    if (m_updatingFromComponent || !m_targetComponent) return;
    m_targetComponent->update();
}
