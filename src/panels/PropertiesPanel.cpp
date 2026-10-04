#include "PropertiesPanel.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include "ImageComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include "TextInputComponent.h"
#include "CircleComponent.h"
#include "PathComponent.h"
#include "CustomComponentInstance.h"
#include "PropertyChangeCommand.h"
#include "ColorPickerDialog.h"
#include "Project.h"
#include "ColorStyle.h"
#include "HardwareBridge.h"
#include <QUndoStack>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFileDialog>
#include <QPainter>
#include <QButtonGroup>

static QIcon createAlignmentIcon(Qt::AlignmentFlag align) {
    QPixmap pixmap(22, 22);
    pixmap.fill(Qt::transparent);

    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(220, 225, 235));

    int y1 = 4, y2 = 10, y3 = 16;
    int h = 2; // line thickness
    int r = 1; // corner radius

    if (align == Qt::AlignLeft) {
        // Left alignment bars
        p.drawRoundedRect(3, y1, 16, h, r, r);
        p.drawRoundedRect(3, y2, 10, h, r, r);
        p.drawRoundedRect(3, y3, 14, h, r, r);
    } else if (align == Qt::AlignHCenter) {
        // Center alignment bars
        p.drawRoundedRect(5, y1, 12, h, r, r);
        p.drawRoundedRect(3, y2, 16, h, r, r);
        p.drawRoundedRect(5, y3, 12, h, r, r);
    } else if (align == Qt::AlignRight) {
        // Right alignment bars
        p.drawRoundedRect(3, y1, 16, h, r, r);
        p.drawRoundedRect(9, y2, 10, h, r, r);
        p.drawRoundedRect(5, y3, 14, h, r, r);
    }

    p.end();
    return QIcon(pixmap);
}

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    setTargetComponent(nullptr);
}

void PropertiesPanel::setupUi() {
    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    // ── Empty state ──────────────────────────────────────────────────────────
    m_emptyWidget = new QWidget(this);
    QVBoxLayout* emptyLayout = new QVBoxLayout(m_emptyWidget);
    emptyLayout->setContentsMargins(20, 20, 20, 20);
    emptyLayout->addStretch(1);

    QLabel* emptyTitle = new QLabel("No Component Selected", m_emptyWidget);
    emptyTitle->setAlignment(Qt::AlignCenter);
    emptyTitle->setStyleSheet("color: #d2d9e4; font-size: 13.5px; font-weight: bold; padding-bottom: 2px;");
    emptyLayout->addWidget(emptyTitle);

    emptyLayout->addSpacing(4);

    QLabel* emptyLabel = new QLabel("Click an element on the canvas to inspect and\nedit its properties.", m_emptyWidget);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: #5c6676; font-size: 11.5px; line-height: 1.4;");
    emptyLayout->addWidget(emptyLabel);

    emptyLayout->addStretch(1);
    rootLayout->addWidget(m_emptyWidget);

    // ── Multi-select state ───────────────────────────────────────────────────
    m_multiWidget = new QWidget(this);
    m_multiWidget->setVisible(false);
    QVBoxLayout* multiLayout = new QVBoxLayout(m_multiWidget);
    multiLayout->setContentsMargins(12, 20, 12, 20);
    multiLayout->setSpacing(12);

    // Header badge & title
    QWidget* multiHeaderCard = new QWidget(m_multiWidget);
    QVBoxLayout* multiHeaderLayout = new QVBoxLayout(multiHeaderCard);
    multiHeaderLayout->setContentsMargins(4, 4, 4, 4);
    multiHeaderLayout->setSpacing(4);

    QLabel* multiIcon = new QLabel("⊞", multiHeaderCard);
    multiIcon->setAlignment(Qt::AlignCenter);
    multiIcon->setStyleSheet("color: #1a96ff; font-size: 32px;");
    multiHeaderLayout->addWidget(multiIcon);

    m_multiLabel = new QLabel("2 components selected", multiHeaderCard);
    m_multiLabel->setAlignment(Qt::AlignCenter);
    m_multiLabel->setStyleSheet("color: #e2e8f0; font-size: 14px; font-weight: bold;");
    multiHeaderLayout->addWidget(m_multiLabel);

    QLabel* multiSubtitle = new QLabel("Batch Alignment & Distribution", multiHeaderCard);
    multiSubtitle->setAlignment(Qt::AlignCenter);
    multiSubtitle->setStyleSheet("color: #6a7382; font-size: 11px;");
    multiHeaderLayout->addWidget(multiSubtitle);
    multiLayout->addWidget(multiHeaderCard);

    multiLayout->addStretch(1);
    rootLayout->addWidget(m_multiWidget);

    // Content container inside scroll area
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background-color: transparent;");

    m_contentWidget = new QWidget(scrollArea);
    QVBoxLayout* mainLayout = new QVBoxLayout(m_contentWidget);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Header: Type badge & ID
    m_headerWidget = new QWidget(m_contentWidget);
    QHBoxLayout* headerLayout = new QHBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    m_typeBadge = new QLabel("COMPONENT", m_contentWidget);
    m_typeBadge->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1a96ff, stop:0.1 #0b82ec, stop:0.85 #0060b8, stop:1 #004485); "
        "color: white; padding: 5px 10px; border-top: 1px solid #82c6ff; border-bottom: 2px solid #002852; "
        "border-radius: 4px; font-weight: bold; font-size: 11px;"
    );
    m_idEdit = new QLineEdit(m_contentWidget);
    m_idEdit->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #16181f, stop:1 #1e222a); "
        "color: #FFFFFF; border-top: 1px solid #0f1115; border-left: 1px solid #14161d; "
        "border-right: 1px solid #2e3544; border-bottom: 1px solid #3c4558; border-radius: 4px; "
        "padding: 4px 8px; font-weight: bold;"
    );
    headerLayout->addWidget(m_typeBadge);
    headerLayout->addWidget(m_idEdit, 1);
    mainLayout->addWidget(m_headerWidget);

    connect(m_idEdit, &QLineEdit::textChanged, this, &PropertiesPanel::onIdChanged);
    connect(m_idEdit, &QLineEdit::editingFinished, this, [this]() {
        commitPropertyChange("Rename Component ID");
    });

    // Geometry Group
    m_geometryGroup = new QGroupBox("Transform & Geometry", m_contentWidget);
    m_geometryGroup->setStyleSheet(
        "QGroupBox { color: #8fa0b8; font-size: 11px; font-weight: bold; border: 1px solid #282e3b; "
        "border-top: 1px solid #14161d; border-bottom: 1px solid #3d4658; border-radius: 6px; "
        "margin-top: 12px; padding-top: 16px; background-color: rgba(22, 25, 32, 0.4); } "
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
    );
    QGridLayout* geomLayout = new QGridLayout(m_geometryGroup);
    geomLayout->setSpacing(8);

    auto makeSpin = [this](int minVal, int maxVal) {
        QSpinBox* spin = new QSpinBox(m_contentWidget);
        spin->setRange(minVal, maxVal);
        spin->setStyleSheet(
            "QSpinBox { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #15171e, stop:1 #1d2129); "
            "color: #FFFFFF; border-top: 1px solid #0d0f13; border-left: 1px solid #12141a; "
            "border-right: 1px solid #2c3240; border-bottom: 1px solid #384152; border-radius: 4px; "
            "padding: 3px 6px; font-weight: bold; }"
        );
        return spin;
    };

    m_spinX = makeSpin(-2000, 2000);
    m_spinY = makeSpin(-2000, 2000);
    m_spinW = makeSpin(10, 2000);
    m_spinH = makeSpin(10, 2000);

    geomLayout->addWidget(new QLabel("X:", m_geometryGroup), 0, 0);
    geomLayout->addWidget(m_spinX, 0, 1);
    geomLayout->addWidget(new QLabel("Y:", m_geometryGroup), 0, 2);
    geomLayout->addWidget(m_spinY, 0, 3);
    geomLayout->addWidget(new QLabel("W:", m_geometryGroup), 1, 0);
    geomLayout->addWidget(m_spinW, 1, 1);
    geomLayout->addWidget(new QLabel("H:", m_geometryGroup), 1, 2);
    geomLayout->addWidget(m_spinH, 1, 3);

    mainLayout->addWidget(m_geometryGroup);

    m_alignmentRow = new QWidget(m_contentWidget);
    m_alignmentRow->setObjectName("multiSelectionAlignmentRow");
    m_alignmentRow->setVisible(false);
    QHBoxLayout* alignmentLayout = new QHBoxLayout(m_alignmentRow);
    alignmentLayout->setContentsMargins(2, 0, 2, 0);
    alignmentLayout->setSpacing(2);
    const QStringList alignGlyphs = {"⇤", "↤", "⇥", "↥", "↕", "↧"};
    const QStringList alignTips = {"Align left", "Align horizontal center", "Align right", "Align top", "Align vertical center", "Align bottom"};
    for (int index = 0; index < alignGlyphs.size(); ++index) {
        QToolButton* button = new QToolButton(m_alignmentRow);
        button->setText(alignGlyphs[index]);
        button->setToolTip(alignTips[index]);
        button->setFixedSize(24, 24);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet("QToolButton { background: #252830; color: #dce3ee; border: 1px solid #3B404E; border-radius: 4px; padding: 0; font-size: 14px; } QToolButton:hover { background: #313642; }");
        connect(button, &QToolButton::clicked, this, [this, index]() {
            switch (index) {
                case 0: emit alignLeftRequested(); break;
                case 1: emit alignHCenterRequested(); break;
                case 2: emit alignRightRequested(); break;
                case 3: emit alignTopRequested(); break;
                case 4: emit alignVCenterRequested(); break;
                case 5: emit alignBottomRequested(); break;
            }
        });
        alignmentLayout->addWidget(button);
    }
    QFrame* divider = new QFrame(m_alignmentRow);
    divider->setFrameShape(QFrame::VLine);
    divider->setStyleSheet("color: #3B404E;");
    alignmentLayout->addWidget(divider);
    const QStringList distributeGlyphs = {"⇔", "⇕"};
    const QStringList distributeTips = {"Distribute horizontally (3+ components)", "Distribute vertically (3+ components)"};
    for (int index = 0; index < distributeGlyphs.size(); ++index) {
        QToolButton* button = new QToolButton(m_alignmentRow);
        button->setText(distributeGlyphs[index]);
        button->setToolTip(distributeTips[index]);
        button->setFixedSize(24, 24);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet("QToolButton { background: #252830; color: #dce3ee; border: 1px solid #3B404E; border-radius: 4px; padding: 0; font-size: 14px; } QToolButton:hover { background: #313642; } QToolButton:disabled { color: #596170; }");
        button->setEnabled(false);
        m_distributeButtons.append(button);
        connect(button, &QToolButton::clicked, this, [this, index]() {
            if (index == 0) emit distributeHRequested();
            else emit distributeVRequested();
        });
        alignmentLayout->addWidget(button);
    }
    alignmentLayout->addStretch(1);
    mainLayout->addWidget(m_alignmentRow);

    // ── Boolean path operations row (shown when 2+ shapes are selected) ──
    m_booleanOpsRow = new QWidget(m_contentWidget);
    m_booleanOpsRow->setObjectName("booleanOpsRow");
    m_booleanOpsRow->setVisible(false);
    QHBoxLayout* boolLayout = new QHBoxLayout(m_booleanOpsRow);
    boolLayout->setContentsMargins(2, 0, 2, 0);
    boolLayout->setSpacing(4);

    struct BoolBtn { QString label; QString tip; };
    const QList<BoolBtn> boolBtns = {
        {"∪", "Boolean Union — merge all selected shapes"},
        {"−", "Boolean Subtract — cut clips from subject"},
        {"∩", "Boolean Intersect — keep overlapping area"},
        {"⊕", "Boolean XOR — keep non-overlapping area"},
    };
    for (int idx = 0; idx < boolBtns.size(); ++idx) {
        QToolButton* btn = new QToolButton(m_booleanOpsRow);
        btn->setText(boolBtns[idx].label);
        btn->setToolTip(boolBtns[idx].tip);
        btn->setFixedSize(28, 24);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QToolButton { background: #1e2938; color: #7ecfff; border: 1px solid #3B404E; "
            "border-radius: 4px; padding: 0; font-size: 15px; font-weight: bold; } "
            "QToolButton:hover { background: #253448; border-color: #5a8fcc; }"
        );
        connect(btn, &QToolButton::clicked, this, [this, idx]() {
            switch (idx) {
                case 0: emit booleanUnionRequested();     break;
                case 1: emit booleanSubtractRequested();  break;
                case 2: emit booleanIntersectRequested(); break;
                case 3: emit booleanXorRequested();       break;
            }
        });
        boolLayout->addWidget(btn);
    }
    boolLayout->addStretch(1);
    mainLayout->addWidget(m_booleanOpsRow);

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
    m_specificGroup->setStyleSheet(
        "QGroupBox { color: #8fa0b8; font-size: 11px; font-weight: bold; border: 1px solid #282e3b; "
        "border-top: 1px solid #14161d; border-bottom: 1px solid #3d4658; border-radius: 6px; "
        "margin-top: 12px; padding-top: 16px; background-color: rgba(22, 25, 32, 0.4); } "
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
    );
    m_specificLayout = new QVBoxLayout(m_specificGroup);
    m_specificLayout->setSpacing(8);
    mainLayout->addWidget(m_specificGroup);

    // ── Hardware Protocol Binding Group ──────────────────────────────────
    m_protocolGroup = new QGroupBox("Hardware Protocol Binding", m_contentWidget);
    m_protocolGroup->setObjectName("hardwareProtocolGroup");
    m_protocolGroup->setStyleSheet(
        "QGroupBox { color: #8fa0b8; font-size: 11px; font-weight: bold; border: 1px solid #282e3b; "
        "border-top: 1px solid #14161d; border-bottom: 1px solid #3d4658; border-radius: 6px; "
        "margin-top: 12px; padding-top: 16px; background-color: rgba(22, 25, 32, 0.4); } "
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
    );
    QVBoxLayout* protoLayout = new QVBoxLayout(m_protocolGroup);
    protoLayout->setSpacing(8);

    // Protocol selector row
    QHBoxLayout* protoRow = new QHBoxLayout();
    QLabel* lblProto = new QLabel("Protocol:", m_protocolGroup);
    lblProto->setStyleSheet("color: #a0aec0; font-size: 11px; font-weight: bold; min-width: 52px;");
    m_protocolCombo = new QComboBox(m_protocolGroup);
    m_protocolCombo->setObjectName("protocolCombo");
    m_protocolCombo->setStyleSheet(
        "QComboBox { background: #1C1F26; color: #FFFFFF; font-size: 11px; font-weight: bold; "
        "border: 1px solid #3B404E; border-radius: 4px; padding: 4px 8px; } "
        "QComboBox::drop-down { border: none; } "
        "QComboBox QAbstractItemView { background: #1C1F26; color: #FFFFFF; selection-background-color: #2196F3; }"
    );
    m_protocolCombo->addItems({"None", "GPIO", "PWM", "ADC", "I2C", "SPI"});
    protoRow->addWidget(lblProto);
    protoRow->addWidget(m_protocolCombo, 1);
    protoLayout->addLayout(protoRow);

    auto makePinCombo = [](QWidget* parent, const QString& objName) {
        QComboBox* cb = new QComboBox(parent);
        cb->setObjectName(objName);
        cb->setStyleSheet(
            "QComboBox { background: #16181F; color: #FFFFFF; font-size: 11px; font-weight: bold; "
            "border: 1px solid #2F3543; border-radius: 4px; padding: 3px 6px; } "
            "QComboBox::drop-down { border: none; } "
            "QComboBox QAbstractItemView { background: #16181F; color: #FFFFFF; selection-background-color: #0b82ec; }"
        );
        return cb;
    };
    auto makeFieldLabel = [](const QString& text, QWidget* parent) {
        QLabel* lbl = new QLabel(text, parent);
        lbl->setStyleSheet("color: #a0aec0; font-size: 11px; font-weight: bold; min-width: 52px;");
        return lbl;
    };

    // 1. None
    m_protocolNoneWidget = new QWidget(m_protocolGroup);
    QVBoxLayout* noneLayout = new QVBoxLayout(m_protocolNoneWidget);
    noneLayout->setContentsMargins(0, 4, 0, 4);
    QLabel* lblNone = new QLabel("No hardware protocol bound. Select GPIO, PWM, ADC, I2C, or SPI to map hardware pins.", m_protocolNoneWidget);
    lblNone->setStyleSheet("color: #64748b; font-size: 10px; font-style: italic;");
    lblNone->setWordWrap(true);
    noneLayout->addWidget(lblNone);
    protoLayout->addWidget(m_protocolNoneWidget);

    // 2. GPIO
    m_gpioWidget = new QWidget(m_protocolGroup);
    m_gpioWidget->setVisible(false);
    QHBoxLayout* gpioLayout = new QHBoxLayout(m_gpioWidget);
    gpioLayout->setContentsMargins(0, 2, 0, 2);
    gpioLayout->addWidget(makeFieldLabel("Pin:", m_gpioWidget));
    m_comboGpioPin = makePinCombo(m_gpioWidget, "gpioPinCombo");
    gpioLayout->addWidget(m_comboGpioPin, 1);
    protoLayout->addWidget(m_gpioWidget);

    // 3. PWM
    m_pwmWidget = new QWidget(m_protocolGroup);
    m_pwmWidget->setVisible(false);
    QHBoxLayout* pwmLayout = new QHBoxLayout(m_pwmWidget);
    pwmLayout->setContentsMargins(0, 2, 0, 2);
    pwmLayout->addWidget(makeFieldLabel("Pin:", m_pwmWidget));
    m_comboPwmPin = makePinCombo(m_pwmWidget, "pwmPinCombo");
    pwmLayout->addWidget(m_comboPwmPin, 1);
    protoLayout->addWidget(m_pwmWidget);

    // 4. ADC
    m_adcWidget = new QWidget(m_protocolGroup);
    m_adcWidget->setVisible(false);
    QHBoxLayout* adcLayout = new QHBoxLayout(m_adcWidget);
    adcLayout->setContentsMargins(0, 2, 0, 2);
    adcLayout->addWidget(makeFieldLabel("Pin:", m_adcWidget));
    m_comboAdcPin = makePinCombo(m_adcWidget, "adcPinCombo");
    adcLayout->addWidget(m_comboAdcPin, 1);
    protoLayout->addWidget(m_adcWidget);

    // 5. SPI (MISO, MOSI, SCK, SS - raw transfer)
    m_spiWidget = new QWidget(m_protocolGroup);
    m_spiWidget->setVisible(false);
    QVBoxLayout* spiVLayout = new QVBoxLayout(m_spiWidget);
    spiVLayout->setContentsMargins(0, 2, 0, 2);
    spiVLayout->setSpacing(6);

    QLabel* spiNotice = new QLabel("SPI Raw Transfer (byte in/out)", m_spiWidget);
    spiNotice->setStyleSheet("color: #38bdf8; background-color: rgba(56, 189, 248, 0.1); border: 1px solid rgba(56, 189, 248, 0.3); border-radius: 4px; padding: 4px 6px; font-size: 10px; font-weight: bold;");
    spiVLayout->addWidget(spiNotice);

    QGridLayout* spiGrid = new QGridLayout();
    spiGrid->setSpacing(6);
    spiGrid->addWidget(makeFieldLabel("MISO:", m_spiWidget), 0, 0);
    m_comboSpiMiso = makePinCombo(m_spiWidget, "spiMisoCombo");
    spiGrid->addWidget(m_comboSpiMiso, 0, 1);

    spiGrid->addWidget(makeFieldLabel("MOSI:", m_spiWidget), 1, 0);
    m_comboSpiMosi = makePinCombo(m_spiWidget, "spiMosiCombo");
    spiGrid->addWidget(m_comboSpiMosi, 1, 1);

    spiGrid->addWidget(makeFieldLabel("SCK:", m_spiWidget), 2, 0);
    m_comboSpiSck = makePinCombo(m_spiWidget, "spiSckCombo");
    spiGrid->addWidget(m_comboSpiSck, 2, 1);

    spiGrid->addWidget(makeFieldLabel("SS:", m_spiWidget), 3, 0);
    m_comboSpiSs = makePinCombo(m_spiWidget, "spiSsCombo");
    spiGrid->addWidget(m_comboSpiSs, 3, 1);
    spiVLayout->addLayout(spiGrid);
    protoLayout->addWidget(m_spiWidget);

    // 6. I2C (SCL, SDA, Device Address - UI/config only)
    m_i2cWidget = new QWidget(m_protocolGroup);
    m_i2cWidget->setVisible(false);
    QVBoxLayout* i2cVLayout = new QVBoxLayout(m_i2cWidget);
    i2cVLayout->setContentsMargins(0, 2, 0, 2);
    i2cVLayout->setSpacing(6);

    QLabel* i2cNotice = new QLabel(
        "⚠ Configuration Only (Firmware export)\n"
        "Live OpenOCD polling disabled (unreliable over JTAG/SWD latency)",
        m_i2cWidget);
    i2cNotice->setWordWrap(true);
    i2cNotice->setStyleSheet(
        "color: #fbbf24; background-color: rgba(245, 158, 11, 0.12); "
        "border: 1px solid rgba(245, 158, 11, 0.35); border-radius: 4px; "
        "padding: 5px 8px; font-size: 10px; font-weight: bold; line-height: 1.3;"
    );
    i2cVLayout->addWidget(i2cNotice);

    QGridLayout* i2cGrid = new QGridLayout();
    i2cGrid->setSpacing(6);
    i2cGrid->addWidget(makeFieldLabel("SCL:", m_i2cWidget), 0, 0);
    m_comboI2cScl = makePinCombo(m_i2cWidget, "i2cSclCombo");
    i2cGrid->addWidget(m_comboI2cScl, 0, 1);

    i2cGrid->addWidget(makeFieldLabel("SDA:", m_i2cWidget), 1, 0);
    m_comboI2cSda = makePinCombo(m_i2cWidget, "i2cSdaCombo");
    i2cGrid->addWidget(m_comboI2cSda, 1, 1);

    i2cGrid->addWidget(makeFieldLabel("Address:", m_i2cWidget), 2, 0);
    m_editI2cAddress = new QLineEdit(m_i2cWidget);
    m_editI2cAddress->setObjectName("i2cAddressEdit");
    m_editI2cAddress->setPlaceholderText("0x48");
    m_editI2cAddress->setText("0x48");
    m_editI2cAddress->setStyleSheet(
        "QLineEdit { background: #16181F; color: #FFFFFF; font-size: 11px; font-weight: bold; "
        "border: 1px solid #2F3543; border-radius: 4px; padding: 3px 6px; }"
    );
    i2cGrid->addWidget(m_editI2cAddress, 2, 1);
    i2cVLayout->addLayout(i2cGrid);
    protoLayout->addWidget(m_i2cWidget);

    mainLayout->addWidget(m_protocolGroup);

    // Connect signals
    connect(m_protocolCombo, &QComboBox::currentTextChanged, this, &PropertiesPanel::onProtocolChanged);
    auto connectPin = [this](QComboBox* cb) {
        connect(cb, &QComboBox::currentTextChanged, this, &PropertiesPanel::onProtocolPinChanged);
    };
    connectPin(m_comboGpioPin);
    connectPin(m_comboPwmPin);
    connectPin(m_comboAdcPin);
    connectPin(m_comboSpiMiso);
    connectPin(m_comboSpiMosi);
    connectPin(m_comboSpiSck);
    connectPin(m_comboSpiSs);
    connectPin(m_comboI2cScl);
    connectPin(m_comboI2cSda);
    connect(m_editI2cAddress, &QLineEdit::textChanged, this, &PropertiesPanel::onProtocolPinChanged);
    connect(m_editI2cAddress, &QLineEdit::editingFinished, this, [this]() {
        commitPropertyChange("Change I2C Address");
    });

    updateBoardPins();
    connect(&HardwareBridge::instance(), &HardwareBridge::boardChanged, this, [this](const QString&) {
        updateBoardPins();
    });

    mainLayout->addStretch(1);
    scrollArea->setWidget(m_contentWidget);
    rootLayout->addWidget(scrollArea);
}

// ── Visibility helpers ────────────────────────────────────────────────────
void PropertiesPanel::showEmpty() {
    m_emptyWidget->setVisible(true);
    m_multiWidget->setVisible(false);
    m_contentWidget->setVisible(false);
    m_alignmentRow->setVisible(false);
    m_booleanOpsRow->setVisible(false);
}

void PropertiesPanel::showSingle() {
    m_emptyWidget->setVisible(false);
    m_multiWidget->setVisible(false);
    m_contentWidget->setVisible(true);
    m_headerWidget->setVisible(true);
    m_specificGroup->setVisible(true);
    if (m_protocolGroup) m_protocolGroup->setVisible(true);
    m_geometryGroup->setEnabled(true);
    m_alignmentRow->setVisible(false);
    m_booleanOpsRow->setVisible(false);
}

void PropertiesPanel::showMulti(int count) {
    m_emptyWidget->setVisible(false);
    m_contentWidget->setVisible(true);
    m_multiLabel->setText(QString("%1 components selected").arg(count));
    m_headerWidget->setVisible(false);
    m_specificGroup->setVisible(false);
    if (m_protocolGroup) m_protocolGroup->setVisible(false);
    m_geometryGroup->setEnabled(false);
    m_alignmentRow->setVisible(count >= 2);
    m_booleanOpsRow->setVisible(count >= 2);
    for (QToolButton* button : m_distributeButtons) button->setEnabled(count >= 3);
    m_multiWidget->setVisible(true);
}

// ─────────────────────────────────────────────────────────────────────────────
void PropertiesPanel::setTargetComponent(UIComponent* comp) {
    if (m_targetComponent == comp) {
        if (comp) {
            refreshValues();
        }
        return;
    }

    m_targetComponent = comp;
    m_lastSavedState = comp ? comp->toJson() : QJsonObject();

    if (!m_targetComponent) {
        showEmpty();
        if (m_specificContainer) {
            m_specificContainer->hide();
            delete m_specificContainer;
            m_specificContainer = nullptr;
        }
        return;
    }

    showSingle();
    m_typeBadge->setText(m_targetComponent->componentType().toUpper());
    rebuildSpecificEditors();
    refreshValues();
}

void PropertiesPanel::setSelectedComponents(const QList<UIComponent*>& comps) {
    if (comps.isEmpty()) {
        setTargetComponent(nullptr);
        showEmpty();
    } else if (comps.size() == 1) {
        setTargetComponent(comps.first());
    } else {
        setTargetComponent(nullptr);
        showMulti(comps.size());
    }
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
        if (m_spinRadius) {
            int maxR = static_cast<int>(std::floor(std::min(btn->compWidth(), btn->compHeight()) / 2.0));
            m_spinRadius->setMaximum(std::max(50, maxR));
            m_spinRadius->setValue(btn->cornerRadius());
        }
        if (m_handlerEdit) m_handlerEdit->setText(btn->onClickedHandler());
    } else if (auto lbl = dynamic_cast<LabelComponent*>(m_targetComponent)) {
        if (m_textEdit) m_textEdit->setText(lbl->text());
        if (m_colorBtn1) updateColorButton(m_colorBtn1, lbl->color());
        if (m_spinPixelSize) m_spinPixelSize->setValue(lbl->pixelSize());
        if (m_chkBold) m_chkBold->setChecked(lbl->bold());
        if (m_chkItalic) m_chkItalic->setChecked(lbl->italic());
        if (m_btnAlignLeft && m_btnAlignCenter && m_btnAlignRight) {
            Qt::Alignment a = lbl->alignment();
            if (a & Qt::AlignHCenter) {
                m_btnAlignCenter->setChecked(true);
            } else if (a & Qt::AlignRight) {
                m_btnAlignRight->setChecked(true);
            } else {
                m_btnAlignLeft->setChecked(true);
            }
        }
    } else if (auto rect = dynamic_cast<RectangleComponent*>(m_targetComponent)) {
        if (m_colorBtn1) updateColorButton(m_colorBtn1, rect->fillColor());
        if (m_colorBtn2) updateColorButton(m_colorBtn2, rect->strokeColor());
        if (m_spinStrokeW) m_spinStrokeW->setValue(rect->strokeWidth());
        if (m_spinRadius) {
            int maxR = static_cast<int>(std::floor(std::min(rect->compWidth(), rect->compHeight()) / 2.0));
            m_spinRadius->setMaximum(std::max(50, maxR));
            m_spinRadius->setValue(rect->cornerRadius());
        }
    } else if (auto prog = dynamic_cast<ProgressBarComponent*>(m_targetComponent)) {
        if (m_spinProgressValue) m_spinProgressValue->setValue(prog->value());
        if (m_colorBtn1) updateColorButton(m_colorBtn1, prog->barColor());
        if (m_colorBtn2) updateColorButton(m_colorBtn2, prog->trackColor());
        if (m_spinRadius) {
            int maxR = static_cast<int>(std::floor(std::min(prog->compWidth(), prog->compHeight()) / 2.0));
            m_spinRadius->setMaximum(std::max(50, maxR));
            m_spinRadius->setValue(prog->cornerRadius());
        }
    } else if (auto img = dynamic_cast<ImageComponent*>(m_targetComponent)) {
        if (m_imagePathEdit) m_imagePathEdit->setText(img->imagePath());
        if (m_comboImageFormat) m_comboImageFormat->setCurrentText(img->format());
    } else if (auto slider = dynamic_cast<SliderComponent*>(m_targetComponent)) {
        if (m_spinSliderVal) m_spinSliderVal->setValue(slider->value());
        if (m_spinSliderMin) m_spinSliderMin->setValue(slider->minimum());
        if (m_spinSliderMax) m_spinSliderMax->setValue(slider->maximum());
        if (m_colorBtn1) updateColorButton(m_colorBtn1, slider->trackColor());
        if (m_colorBtn2) updateColorButton(m_colorBtn2, slider->fillColor());
        if (m_colorBtn3) updateColorButton(m_colorBtn3, slider->handleColor());
    } else if (auto sw = dynamic_cast<SwitchComponent*>(m_targetComponent)) {
        if (m_chkState) m_chkState->setChecked(sw->isChecked());
        if (m_colorBtn1) updateColorButton(m_colorBtn1, sw->onColor());
        if (m_colorBtn2) updateColorButton(m_colorBtn2, sw->offColor());
        if (m_colorBtn3) updateColorButton(m_colorBtn3, sw->thumbColor());
        if (m_handlerEdit) m_handlerEdit->setText(sw->onToggledHandler());
    } else if (auto chk = dynamic_cast<CheckboxComponent*>(m_targetComponent)) {
        if (m_textEdit) m_textEdit->setText(chk->text());
        if (m_chkState) m_chkState->setChecked(chk->isChecked());
        if (m_colorBtn1) updateColorButton(m_colorBtn1, chk->textColor());
        if (m_colorBtn2) updateColorButton(m_colorBtn2, chk->checkColor());
        if (m_colorBtn3) updateColorButton(m_colorBtn3, chk->boxColor());
        if (m_colorBtn4) updateColorButton(m_colorBtn4, chk->borderColor());
        if (m_handlerEdit) m_handlerEdit->setText(chk->onToggledHandler());
    } else if (auto txt = dynamic_cast<TextInputComponent*>(m_targetComponent)) {
        if (m_textEdit) m_textEdit->setText(txt->text());
        if (m_placeholderEdit) m_placeholderEdit->setText(txt->placeholder());
        if (m_colorBtn1) updateColorButton(m_colorBtn1, txt->textColor());
        if (m_colorBtn2) updateColorButton(m_colorBtn2, txt->placeholderColor());
        if (m_colorBtn3) updateColorButton(m_colorBtn3, txt->backgroundColor());
        if (m_colorBtn4) updateColorButton(m_colorBtn4, txt->borderColor());
        if (m_spinStrokeW) m_spinStrokeW->setValue(txt->borderWidth());
        if (m_spinRadius) m_spinRadius->setValue(txt->cornerRadius());
        if (m_spinPixelSize) m_spinPixelSize->setValue(txt->pixelSize());
        if (m_chkReadOnly) m_chkReadOnly->setChecked(txt->isReadOnly());
        if (m_handlerEdit) m_handlerEdit->setText(txt->onTextChangedHandler());
    } else if (auto circ = dynamic_cast<CircleComponent*>(m_targetComponent)) {
        if (m_colorBtn1) updateColorButton(m_colorBtn1, circ->fillColor());
        if (m_colorBtn2) updateColorButton(m_colorBtn2, circ->strokeColor());
        if (m_spinStrokeW) m_spinStrokeW->setValue(circ->strokeWidth());
        if (m_chkFilled) m_chkFilled->setChecked(circ->isFilled());
    } else if (auto path = dynamic_cast<PathComponent*>(m_targetComponent)) {
        if (m_colorBtn1) updateColorButton(m_colorBtn1, path->strokeColor());
        if (m_spinStrokeW) m_spinStrokeW->setValue(qRound(path->strokeThickness()));
        if (m_spinOpacity) m_spinOpacity->setValue(path->opacityPercent());
        if (m_spinPathFlatten) m_spinPathFlatten->setValue(path->flattenSubdivisions());
    }

    // ── Synchronize Hardware Protocol Binding ─────────────────────────
    if (m_protocolCombo && m_targetComponent) {
        QString proto = m_targetComponent->protocol();
        if (proto.isEmpty()) proto = "None";
        int idx = m_protocolCombo->findText(proto);
        if (idx >= 0) m_protocolCombo->setCurrentIndex(idx);
        else m_protocolCombo->setCurrentIndex(0);

        updateProtocolFieldsVisibility(proto);

        auto setComboVal = [](QComboBox* cb, const QString& val) {
            if (!cb) return;
            bool b = cb->blockSignals(true);
            int i = cb->findText(val);
            if (i >= 0) cb->setCurrentIndex(i);
            else if (!val.isEmpty()) {
                cb->addItem(val);
                cb->setCurrentText(val);
            } else {
                cb->setCurrentIndex(0);
            }
            cb->blockSignals(b);
        };

        if (proto == "GPIO") {
            setComboVal(m_comboGpioPin, m_targetComponent->protocolPin("pin"));
        } else if (proto == "PWM") {
            setComboVal(m_comboPwmPin, m_targetComponent->protocolPin("pin"));
        } else if (proto == "ADC") {
            setComboVal(m_comboAdcPin, m_targetComponent->protocolPin("pin"));
        } else if (proto == "SPI") {
            setComboVal(m_comboSpiMiso, m_targetComponent->protocolPin("miso"));
            setComboVal(m_comboSpiMosi, m_targetComponent->protocolPin("mosi"));
            setComboVal(m_comboSpiSck,  m_targetComponent->protocolPin("sck"));
            setComboVal(m_comboSpiSs,   m_targetComponent->protocolPin("ss"));
        } else if (proto == "I2C") {
            setComboVal(m_comboI2cScl, m_targetComponent->protocolPin("scl"));
            setComboVal(m_comboI2cSda, m_targetComponent->protocolPin("sda"));
            if (m_editI2cAddress) {
                bool b = m_editI2cAddress->blockSignals(true);
                m_editI2cAddress->setText(m_targetComponent->protocolPin("address", "0x48"));
                m_editI2cAddress->blockSignals(b);
            }
        }
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
    // 1. Cleanly delete previous container widget and all its children/layouts
    if (m_specificContainer) {
        m_specificContainer->hide();
        m_specificLayout->removeWidget(m_specificContainer);
        delete m_specificContainer;
        m_specificContainer = nullptr;
    }

    // Recursively clear any remaining layout items from m_specificLayout
    QLayoutItem* item;
    while ((item = m_specificLayout->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) {
            w->hide();
            delete w;
        } else if (QLayout* l = item->layout()) {
            QLayoutItem* subItem;
            while ((subItem = l->takeAt(0)) != nullptr) {
                if (QWidget* subW = subItem->widget()) {
                    subW->hide();
                    delete subW;
                }
                delete subItem;
            }
            delete l;
        }
        delete item;
    }

    if (m_specificGroup) {
        const auto remainingWidgets = m_specificGroup->findChildren<QWidget*>(Qt::FindDirectChildrenOnly);
        for (QWidget* w : remainingWidgets) {
            delete w;
        }
    }

    // Reset control pointers
    m_textEdit = nullptr;
    m_colorBtn1 = nullptr;
    m_colorBtn2 = nullptr;
    m_colorBtn3 = nullptr;
    m_colorBtn4 = nullptr;
    m_spinRadius = nullptr;
    m_spinStrokeW = nullptr;
    m_spinOpacity = nullptr;
    m_spinPixelSize = nullptr;
    m_chkBold = nullptr;
    m_chkItalic = nullptr;
    m_handlerEdit = nullptr;
    m_spinProgressValue = nullptr;
    m_imagePathEdit = nullptr;
    m_browseImageBtn = nullptr;
    m_comboImageFormat = nullptr;
    m_spinSliderVal = nullptr;
    m_spinSliderMin = nullptr;
    m_spinSliderMax = nullptr;
    m_chkState = nullptr;
    m_placeholderEdit = nullptr;
    m_chkReadOnly = nullptr;
    m_chkFilled = nullptr;
    m_spinPathFlatten = nullptr;
    m_btnAlignLeft = nullptr;
    m_btnAlignCenter = nullptr;
    m_btnAlignRight = nullptr;

    if (!m_targetComponent) return;

    // 2. Create fresh container parented to m_specificGroup
    m_specificContainer = new QWidget(m_specificGroup);
    QVBoxLayout* containerLayout = new QVBoxLayout(m_specificContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(8);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(8);
    form->setContentsMargins(0, 0, 0, 0);

    auto addColorRow = [this, form](const QString& label, const QString& propKey, const QColor& initialColor, auto setter, const QString& desc) {
        QWidget* rowWidget = new QWidget(m_specificContainer);
        QHBoxLayout* h = new QHBoxLayout(rowWidget);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(4);

        QPushButton* hexBtn = new QPushButton(rowWidget);
        updateColorButton(hexBtn, initialColor);

        QComboBox* styleCombo = new QComboBox(rowWidget);
        styleCombo->setStyleSheet(
            "QComboBox { background: #1C1F26; color: #FFFFFF; font-size: 11px; font-weight: bold; border: 1px solid #3B404E; border-radius: 4px; padding: 3px 6px; }"
            "QComboBox::drop-down { border: none; }"
            "QComboBox QAbstractItemView { background: #1C1F26; color: #FFFFFF; selection-background-color: #2196F3; }"
        );

        auto populateCombo = [this, styleCombo]() {
            styleCombo->clear();
            if (m_project) {
                for (const ColorStyle& s : m_project->colorStyles()) {
                    QPixmap px(12, 12);
                    px.fill(s.color);
                    styleCombo->addItem(QIcon(px), QString("%1 (%2)").arg(s.name, s.color.name().toUpper()), s.name);
                }
            }
        };
        populateCombo();

        bool hasRef = m_targetComponent && m_targetComponent->hasColorStyleRef(propKey);
        QString currentRef = hasRef ? m_targetComponent->colorStyleRef(propKey) : QString();

        QPushButton* toggleBtn = new QPushButton(hasRef ? "🏷" : "🎨", rowWidget);
        toggleBtn->setFixedSize(30, 26);
        toggleBtn->setToolTip(hasRef ? "Named Style active. Click to switch to direct Hex." : "Direct Hex active. Click to pick a named style.");
        toggleBtn->setStyleSheet(hasRef ?
            "QPushButton { background: #1976D2; color: #FFFFFF; font-size: 11px; border-radius: 4px; border: 1px solid #2196F3; padding: 0px; }" :
            "QPushButton { background: #252830; color: #A0ABC0; font-size: 11px; border-radius: 4px; border: 1px solid #3B404E; padding: 0px; }"
        );

        if (hasRef) {
            hexBtn->hide();
            styleCombo->show();
            int idx = styleCombo->findData(currentRef);
            if (idx >= 0) styleCombo->setCurrentIndex(idx);
        } else {
            styleCombo->hide();
            hexBtn->show();
        }

        connect(toggleBtn, &QPushButton::clicked, this, [this, toggleBtn, hexBtn, styleCombo, propKey, setter, desc, populateCombo]() {
            if (!m_targetComponent) return;
            bool nowStyle = hexBtn->isVisible(); // switching to style mode
            if (nowStyle) {
                populateCombo();
                hexBtn->hide();
                styleCombo->show();
                toggleBtn->setText("🏷");
                toggleBtn->setToolTip("Named Style active. Click to switch to direct Hex.");
                toggleBtn->setStyleSheet("QPushButton { background: #1976D2; color: #FFFFFF; font-size: 11px; border-radius: 4px; border: 1px solid #2196F3; padding: 0px; }");
                if (styleCombo->count() > 0) {
                    QString styleName = styleCombo->currentData().toString();
                    m_targetComponent->setColorStyleRef(propKey, styleName);
                    if (m_project) {
                        QColor sc = m_project->resolveColor(styleName);
                        setter(sc);
                        updateColorButton(hexBtn, sc);
                    }
                    commitPropertyChange(desc);
                }
            } else {
                styleCombo->hide();
                hexBtn->show();
                toggleBtn->setText("🎨");
                toggleBtn->setToolTip("Direct Hex active. Click to pick a named style.");
                toggleBtn->setStyleSheet("QPushButton { background: #252830; color: #A0ABC0; font-size: 11px; border-radius: 4px; border: 1px solid #3B404E; padding: 0px; }");
                m_targetComponent->clearColorStyleRef(propKey);
                commitPropertyChange(desc);
            }
        });

        connect(hexBtn, &QPushButton::clicked, this, [this, hexBtn, propKey, setter, desc]() {
            if (!m_targetComponent) return;
            QColor current(hexBtn->text());
            QColor picked = ColorPickerDialog::getColor(current, this, "Choose Color");
            if (picked.isValid()) {
                m_targetComponent->clearColorStyleRef(propKey);
                updateColorButton(hexBtn, picked);
                setter(picked);
                commitPropertyChange(desc);
            }
        });

        connect(styleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this, hexBtn, styleCombo, propKey, setter, desc](int idx) {
            if (!m_targetComponent || idx < 0 || !styleCombo->isVisible()) return;
            QString styleName = styleCombo->itemData(idx).toString();
            m_targetComponent->setColorStyleRef(propKey, styleName);
            if (m_project) {
                QColor sc = m_project->resolveColor(styleName);
                setter(sc);
                updateColorButton(hexBtn, sc);
            }
            commitPropertyChange(desc);
        });

        h->addWidget(toggleBtn);
        h->addWidget(hexBtn, 1);
        h->addWidget(styleCombo, 1);
        form->addRow(label, rowWidget);
        return hexBtn;
    };

    if (auto btn = dynamic_cast<ButtonComponent*>(m_targetComponent)) {
        m_textEdit = new QLineEdit(btn->text(), m_specificContainer);
        connect(m_textEdit, &QLineEdit::textChanged, this, [this, btn](const QString& t) {
            if (!m_updatingFromComponent) btn->setText(t);
        });
        connect(m_textEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Button Text");
        });
        form->addRow("Text:", m_textEdit);

        m_colorBtn1 = addColorRow("Background:", "backgroundColor", btn->backgroundColor(), [btn](const QColor& c) { btn->setBackgroundColor(c); }, "Change Background Color");
        m_colorBtn2 = addColorRow("Text Color:", "textColor", btn->textColor(), [btn](const QColor& c) { btn->setTextColor(c); }, "Change Text Color");

        m_spinRadius = new QSpinBox(m_specificContainer);
        int maxR = static_cast<int>(std::floor(std::min(btn->compWidth(), btn->compHeight()) / 2.0));
        m_spinRadius->setRange(0, std::max(50, maxR));
        m_spinRadius->setValue(btn->cornerRadius());
        connect(m_spinRadius, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, btn](int v) {
            if (!m_updatingFromComponent) btn->setCornerRadius(v);
        });
        connect(m_spinRadius, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Corner Radius");
        });
        form->addRow("Corner Radius:", m_spinRadius);

        m_handlerEdit = new QLineEdit(btn->onClickedHandler(), m_specificContainer);
        connect(m_handlerEdit, &QLineEdit::textChanged, this, [this, btn](const QString& h) {
            if (!m_updatingFromComponent) btn->setOnClickedHandler(h);
        });
        connect(m_handlerEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Click Handler");
        });
        form->addRow("OnClicked:", m_handlerEdit);

    } else if (auto lbl = dynamic_cast<LabelComponent*>(m_targetComponent)) {
        m_textEdit = new QLineEdit(lbl->text(), m_specificContainer);
        connect(m_textEdit, &QLineEdit::textChanged, this, [this, lbl](const QString& t) {
            if (!m_updatingFromComponent) lbl->setText(t);
        });
        connect(m_textEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Label Text");
        });
        form->addRow("Text:", m_textEdit);

        m_colorBtn1 = addColorRow("Color:", "color", lbl->color(), [lbl](const QColor& c) { lbl->setColor(c); }, "Change Label Color");

        m_spinPixelSize = new QSpinBox(m_specificContainer);
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
        m_chkBold = new QCheckBox("Bold", m_specificContainer);
        m_chkBold->setChecked(lbl->bold());
        connect(m_chkBold, &QCheckBox::toggled, this, [this, lbl](bool b) {
            if (!m_updatingFromComponent) {
                lbl->setBold(b);
                commitPropertyChange("Toggle Bold");
            }
        });
        m_chkItalic = new QCheckBox("Italic", m_specificContainer);
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

        // Alignment controls (Left, Center, Right)
        QHBoxLayout* alignLayout = new QHBoxLayout();
        alignLayout->setSpacing(6);

        QButtonGroup* alignGroup = new QButtonGroup(m_specificContainer);
        alignGroup->setExclusive(true);

        auto styleAlignBtn = [](QPushButton* b) {
            b->setCheckable(true);
            b->setFixedSize(38, 28);
            b->setIconSize(QSize(20, 20));
            b->setCursor(Qt::PointingHandCursor);
            b->setStyleSheet(
                "QPushButton { background-color: #252830; border: 1px solid #3B404E; border-radius: 4px; padding: 2px; }"
                "QPushButton:hover { background-color: #313642; border-color: #4D5466; }"
                "QPushButton:checked { background-color: #2196F3; border-color: #1976D2; }"
            );
        };

        m_btnAlignLeft = new QPushButton(m_specificContainer);
        m_btnAlignLeft->setIcon(createAlignmentIcon(Qt::AlignLeft));
        m_btnAlignLeft->setToolTip("Align Left");
        styleAlignBtn(m_btnAlignLeft);

        m_btnAlignCenter = new QPushButton(m_specificContainer);
        m_btnAlignCenter->setIcon(createAlignmentIcon(Qt::AlignHCenter));
        m_btnAlignCenter->setToolTip("Align Center");
        styleAlignBtn(m_btnAlignCenter);

        m_btnAlignRight = new QPushButton(m_specificContainer);
        m_btnAlignRight->setIcon(createAlignmentIcon(Qt::AlignRight));
        m_btnAlignRight->setToolTip("Align Right");
        styleAlignBtn(m_btnAlignRight);

        alignGroup->addButton(m_btnAlignLeft);
        alignGroup->addButton(m_btnAlignCenter);
        alignGroup->addButton(m_btnAlignRight);

        Qt::Alignment align = lbl->alignment();
        if (align & Qt::AlignHCenter) {
            m_btnAlignCenter->setChecked(true);
        } else if (align & Qt::AlignRight) {
            m_btnAlignRight->setChecked(true);
        } else {
            m_btnAlignLeft->setChecked(true);
        }

        connect(m_btnAlignLeft, &QPushButton::clicked, this, [this, lbl]() {
            if (!m_updatingFromComponent) {
                lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                commitPropertyChange("Align Text Left");
            }
        });
        connect(m_btnAlignCenter, &QPushButton::clicked, this, [this, lbl]() {
            if (!m_updatingFromComponent) {
                lbl->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
                commitPropertyChange("Align Text Center");
            }
        });
        connect(m_btnAlignRight, &QPushButton::clicked, this, [this, lbl]() {
            if (!m_updatingFromComponent) {
                lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
                commitPropertyChange("Align Text Right");
            }
        });

        alignLayout->addWidget(m_btnAlignLeft);
        alignLayout->addWidget(m_btnAlignCenter);
        alignLayout->addWidget(m_btnAlignRight);
        alignLayout->addStretch(1);
        form->addRow("Alignment:", alignLayout);

        // ── Task 5: Font family dropdown ──────────────────────────────────
        QComboBox* fontCombo = new QComboBox(m_specificContainer);
        fontCombo->setStyleSheet(
            "QComboBox { background:#1c1f26; color:#e4ecf7; border:1px solid #2b2f38; "
            "border-radius:5px; padding:3px 8px; font-size:12px; } "
            "QComboBox QAbstractItemView { background:#1a1d24; color:#e0e5ee; "
            "selection-background-color:#1a73e8; selection-color:#fff; border:1px solid #2c313e; }"
        );
        for (const QString& f : LabelComponent::availableFonts())
            fontCombo->addItem(f);
        fontCombo->setCurrentText(lbl->fontFamily());
        connect(fontCombo, &QComboBox::currentTextChanged, this, [this, lbl](const QString& fam) {
            if (!m_updatingFromComponent) {
                lbl->setFontFamily(fam);
                commitPropertyChange("Change Font Family");
            }
        });
        form->addRow("Font Family:", fontCombo);

        // ── Task 5: Letter Spacing ────────────────────────────────────────
        QDoubleSpinBox* spinLetterSp = new QDoubleSpinBox(m_specificContainer);
        spinLetterSp->setRange(-5.0, 20.0);
        spinLetterSp->setSingleStep(0.5);
        spinLetterSp->setDecimals(1);
        spinLetterSp->setSuffix(" px");
        spinLetterSp->setValue(lbl->letterSpacing());
        connect(spinLetterSp, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [this, lbl](double v) {
            if (!m_updatingFromComponent) lbl->setLetterSpacing(v);
        });
        connect(spinLetterSp, &QDoubleSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Letter Spacing");
        });
        form->addRow("Letter Spacing:", spinLetterSp);

        // ── Task 5: Line Height ───────────────────────────────────────────
        QSpinBox* spinLineH = new QSpinBox(m_specificContainer);
        spinLineH->setRange(0, 400);
        spinLineH->setSuffix(" %");
        spinLineH->setSpecialValueText("Default");
        spinLineH->setValue(lbl->lineHeight());
        connect(spinLineH, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this, lbl](int v) {
            if (!m_updatingFromComponent) lbl->setLineHeight(v);
        });
        connect(spinLineH, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Line Height");
        });
        form->addRow("Line Height:", spinLineH);

    } else if (auto rect = dynamic_cast<RectangleComponent*>(m_targetComponent)) {
        m_colorBtn1 = addColorRow("Fill Color:", "fillColor", rect->fillColor(), [rect](const QColor& c) { rect->setFillColor(c); }, "Change Fill Color");
        m_colorBtn2 = addColorRow("Stroke Color:", "strokeColor", rect->strokeColor(), [rect](const QColor& c) { rect->setStrokeColor(c); }, "Change Stroke Color");

        m_spinStrokeW = new QSpinBox(m_specificContainer);
        m_spinStrokeW->setRange(0, 20);
        m_spinStrokeW->setValue(rect->strokeWidth());
        connect(m_spinStrokeW, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, rect](int v) {
            if (!m_updatingFromComponent) rect->setStrokeWidth(v);
        });
        connect(m_spinStrokeW, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Stroke Width");
        });
        form->addRow("Stroke Width:", m_spinStrokeW);

        m_spinRadius = new QSpinBox(m_specificContainer);
        int maxR = static_cast<int>(std::floor(std::min(rect->compWidth(), rect->compHeight()) / 2.0));
        m_spinRadius->setRange(0, std::max(50, maxR));
        m_spinRadius->setValue(rect->cornerRadius());
        connect(m_spinRadius, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, rect](int v) {
            if (!m_updatingFromComponent) rect->setCornerRadius(v);
        });
        connect(m_spinRadius, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Corner Radius");
        });
        form->addRow("Corner Radius:", m_spinRadius);

    } else if (auto prog = dynamic_cast<ProgressBarComponent*>(m_targetComponent)) {
        m_spinProgressValue = new QDoubleSpinBox(m_specificContainer);
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

        m_colorBtn1 = addColorRow("Bar Color:", "barColor", prog->barColor(), [prog](const QColor& c) { prog->setBarColor(c); }, "Change Bar Color");
        m_colorBtn2 = addColorRow("Track Color:", "trackColor", prog->trackColor(), [prog](const QColor& c) { prog->setTrackColor(c); }, "Change Track Color");

        m_spinRadius = new QSpinBox(m_specificContainer);
        int progMaxR = static_cast<int>(std::floor(std::min(prog->compWidth(), prog->compHeight()) / 2.0));
        m_spinRadius->setRange(0, std::max(20, progMaxR));
        m_spinRadius->setValue(prog->cornerRadius());
        connect(m_spinRadius, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, prog](int v) {
            if (!m_updatingFromComponent) prog->setCornerRadius(v);
        });
        connect(m_spinRadius, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Corner Radius");
        });
        form->addRow("Corner Radius:", m_spinRadius);

    } else if (auto img = dynamic_cast<ImageComponent*>(m_targetComponent)) {
        QHBoxLayout* pathLayout = new QHBoxLayout();
        m_imagePathEdit = new QLineEdit(img->imagePath(), m_specificContainer);
        m_browseImageBtn = new QPushButton("Browse...", m_specificContainer);
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

        m_comboImageFormat = new QComboBox(m_specificContainer);
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
        m_spinSliderVal = new QSpinBox(m_specificContainer);
        m_spinSliderVal->setRange(slider->minimum(), slider->maximum());
        m_spinSliderVal->setValue(slider->value());
        connect(m_spinSliderVal, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, slider](int v) {
            if (!m_updatingFromComponent) slider->setValue(v);
        });
        connect(m_spinSliderVal, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Slider Value");
        });
        form->addRow("Value:", m_spinSliderVal);

        m_spinSliderMin = new QSpinBox(m_specificContainer);
        m_spinSliderMin->setRange(-10000, 10000);
        m_spinSliderMin->setValue(slider->minimum());
        connect(m_spinSliderMin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, slider](int v) {
            if (!m_updatingFromComponent) slider->setMinimum(v);
        });
        connect(m_spinSliderMin, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Slider Minimum");
        });
        form->addRow("Minimum:", m_spinSliderMin);

        m_spinSliderMax = new QSpinBox(m_specificContainer);
        m_spinSliderMax->setRange(-10000, 10000);
        m_spinSliderMax->setValue(slider->maximum());
        connect(m_spinSliderMax, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, slider](int v) {
            if (!m_updatingFromComponent) slider->setMaximum(v);
        });
        connect(m_spinSliderMax, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Slider Maximum");
        });
        form->addRow("Maximum:", m_spinSliderMax);

        m_colorBtn1 = addColorRow("Track Color:", "trackColor", slider->trackColor(), [slider](const QColor& c) { slider->setTrackColor(c); }, "Change Track Color");
        m_colorBtn2 = addColorRow("Fill Color:", "fillColor", slider->fillColor(), [slider](const QColor& c) { slider->setFillColor(c); }, "Change Fill Color");
        m_colorBtn3 = addColorRow("Handle Color:", "handleColor", slider->handleColor(), [slider](const QColor& c) { slider->setHandleColor(c); }, "Change Handle Color");

    } else if (auto sw = dynamic_cast<SwitchComponent*>(m_targetComponent)) {
        m_chkState = new QCheckBox("Checked", m_specificContainer);
        m_chkState->setChecked(sw->isChecked());
        connect(m_chkState, &QCheckBox::toggled, this, [this, sw](bool b) {
            if (!m_updatingFromComponent) {
                sw->setChecked(b);
                commitPropertyChange("Toggle Switch");
            }
        });
        form->addRow("State:", m_chkState);

        m_colorBtn1 = addColorRow("On Color:", "onColor", sw->onColor(), [sw](const QColor& c) { sw->setOnColor(c); }, "Change On Color");
        m_colorBtn2 = addColorRow("Off Color:", "offColor", sw->offColor(), [sw](const QColor& c) { sw->setOffColor(c); }, "Change Off Color");
        m_colorBtn3 = addColorRow("Thumb Color:", "thumbColor", sw->thumbColor(), [sw](const QColor& c) { sw->setThumbColor(c); }, "Change Thumb Color");

        m_handlerEdit = new QLineEdit(sw->onToggledHandler(), m_specificContainer);
        connect(m_handlerEdit, &QLineEdit::textChanged, this, [this, sw](const QString& h) {
            if (!m_updatingFromComponent) sw->setOnToggledHandler(h);
        });
        connect(m_handlerEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Toggled Handler");
        });
        form->addRow("OnToggled:", m_handlerEdit);

    } else if (auto chk = dynamic_cast<CheckboxComponent*>(m_targetComponent)) {
        m_textEdit = new QLineEdit(chk->text(), m_specificContainer);
        connect(m_textEdit, &QLineEdit::textChanged, this, [this, chk](const QString& t) {
            if (!m_updatingFromComponent) chk->setText(t);
        });
        connect(m_textEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Checkbox Text");
        });
        form->addRow("Text:", m_textEdit);

        m_chkState = new QCheckBox("Checked", m_specificContainer);
        m_chkState->setChecked(chk->isChecked());
        connect(m_chkState, &QCheckBox::toggled, this, [this, chk](bool b) {
            if (!m_updatingFromComponent) {
                chk->setChecked(b);
                commitPropertyChange("Toggle Checkbox");
            }
        });
        form->addRow("State:", m_chkState);

        m_colorBtn1 = addColorRow("Text Color:", "textColor", chk->textColor(), [chk](const QColor& c) { chk->setTextColor(c); }, "Change Text Color");
        m_colorBtn2 = addColorRow("Check Color:", "checkColor", chk->checkColor(), [chk](const QColor& c) { chk->setCheckColor(c); }, "Change Check Color");
        m_colorBtn3 = addColorRow("Box Color:", "boxColor", chk->boxColor(), [chk](const QColor& c) { chk->setBoxColor(c); }, "Change Box Color");
        m_colorBtn4 = addColorRow("Border Color:", "borderColor", chk->borderColor(), [chk](const QColor& c) { chk->setBorderColor(c); }, "Change Border Color");

        m_handlerEdit = new QLineEdit(chk->onToggledHandler(), m_specificContainer);
        connect(m_handlerEdit, &QLineEdit::textChanged, this, [this, chk](const QString& h) {
            if (!m_updatingFromComponent) chk->setOnToggledHandler(h);
        });
        connect(m_handlerEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Toggled Handler");
        });
        form->addRow("OnToggled:", m_handlerEdit);

    } else if (auto txt = dynamic_cast<TextInputComponent*>(m_targetComponent)) {
        m_textEdit = new QLineEdit(txt->text(), m_specificContainer);
        connect(m_textEdit, &QLineEdit::textChanged, this, [this, txt](const QString& t) {
            if (!m_updatingFromComponent) txt->setText(t);
        });
        connect(m_textEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Input Text");
        });
        form->addRow("Text:", m_textEdit);

        m_placeholderEdit = new QLineEdit(txt->placeholder(), m_specificContainer);
        connect(m_placeholderEdit, &QLineEdit::textChanged, this, [this, txt](const QString& p) {
            if (!m_updatingFromComponent) txt->setPlaceholder(p);
        });
        connect(m_placeholderEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Placeholder");
        });
        form->addRow("Placeholder:", m_placeholderEdit);

        m_colorBtn1 = addColorRow("Text Color:", "textColor", txt->textColor(), [txt](const QColor& c) { txt->setTextColor(c); }, "Change Text Color");
        m_colorBtn2 = addColorRow("Placeholder Color:", "placeholderColor", txt->placeholderColor(), [txt](const QColor& c) { txt->setPlaceholderColor(c); }, "Change Placeholder Color");
        m_colorBtn3 = addColorRow("Background Color:", "backgroundColor", txt->backgroundColor(), [txt](const QColor& c) { txt->setBackgroundColor(c); }, "Change Background Color");
        m_colorBtn4 = addColorRow("Border Color:", "borderColor", txt->borderColor(), [txt](const QColor& c) { txt->setBorderColor(c); }, "Change Border Color");

        m_spinStrokeW = new QSpinBox(m_specificContainer);
        m_spinStrokeW->setRange(0, 20);
        m_spinStrokeW->setValue(txt->borderWidth());
        connect(m_spinStrokeW, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, txt](int v) {
            if (!m_updatingFromComponent) txt->setBorderWidth(v);
        });
        connect(m_spinStrokeW, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Border Width");
        });
        form->addRow("Border Width:", m_spinStrokeW);

        m_spinRadius = new QSpinBox(m_specificContainer);
        m_spinRadius->setRange(0, 50);
        m_spinRadius->setValue(txt->cornerRadius());
        connect(m_spinRadius, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, txt](int v) {
            if (!m_updatingFromComponent) txt->setCornerRadius(v);
        });
        connect(m_spinRadius, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Corner Radius");
        });
        form->addRow("Corner Radius:", m_spinRadius);

        m_spinPixelSize = new QSpinBox(m_specificContainer);
        m_spinPixelSize->setRange(6, 96);
        m_spinPixelSize->setValue(txt->pixelSize());
        connect(m_spinPixelSize, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, txt](int v) {
            if (!m_updatingFromComponent) txt->setPixelSize(v);
        });
        connect(m_spinPixelSize, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Font Size");
        });
        form->addRow("Font Size:", m_spinPixelSize);

        m_chkReadOnly = new QCheckBox("Read Only", m_specificContainer);
        m_chkReadOnly->setChecked(txt->isReadOnly());
        connect(m_chkReadOnly, &QCheckBox::toggled, this, [this, txt](bool b) {
            if (!m_updatingFromComponent) {
                txt->setReadOnly(b);
                commitPropertyChange("Toggle Read Only");
            }
        });
        form->addRow("Behavior:", m_chkReadOnly);

        m_handlerEdit = new QLineEdit(txt->onTextChangedHandler(), m_specificContainer);
        connect(m_handlerEdit, &QLineEdit::textChanged, this, [this, txt](const QString& h) {
            if (!m_updatingFromComponent) txt->setOnTextChangedHandler(h);
        });
        connect(m_handlerEdit, &QLineEdit::editingFinished, this, [this]() {
            commitPropertyChange("Change Text Changed Handler");
        });
        form->addRow("OnTextChanged:", m_handlerEdit);

    } else if (auto circ = dynamic_cast<CircleComponent*>(m_targetComponent)) {
        m_colorBtn1 = addColorRow("Fill Color:", "fillColor", circ->fillColor(), [circ](const QColor& c) { circ->setFillColor(c); }, "Change Fill Color");
        m_colorBtn2 = addColorRow("Stroke Color:", "strokeColor", circ->strokeColor(), [circ](const QColor& c) { circ->setStrokeColor(c); }, "Change Stroke Color");

        m_spinStrokeW = new QSpinBox(m_specificContainer);
        m_spinStrokeW->setRange(0, 20);
        m_spinStrokeW->setValue(circ->strokeWidth());
        connect(m_spinStrokeW, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, circ](int v) {
            if (!m_updatingFromComponent) circ->setStrokeWidth(v);
        });
        connect(m_spinStrokeW, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Stroke Width");
        });
        form->addRow("Stroke Width:", m_spinStrokeW);

        m_chkFilled = new QCheckBox("Filled", m_specificContainer);
        m_chkFilled->setChecked(circ->isFilled());
        connect(m_chkFilled, &QCheckBox::toggled, this, [this, circ](bool b) {
            if (!m_updatingFromComponent) {
                circ->setFilled(b);
                commitPropertyChange("Toggle Circle Fill");
            }
        });
        form->addRow("Fill:", m_chkFilled);
    } else if (auto path = dynamic_cast<PathComponent*>(m_targetComponent)) {
        m_colorBtn1 = addColorRow("Stroke Color:", "strokeColor", path->strokeColor(), [path](const QColor& color) { path->setStrokeColor(color); }, "Change Path Stroke Color");

        m_spinStrokeW = new QSpinBox(m_specificContainer);
        m_spinStrokeW->setRange(1, 64);
        m_spinStrokeW->setValue(qRound(path->strokeThickness()));
        connect(m_spinStrokeW, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, path](int value) {
            if (!m_updatingFromComponent) path->setStrokeThickness(value);
        });
        connect(m_spinStrokeW, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Path Stroke Thickness");
        });
        form->addRow("Stroke Thickness:", m_spinStrokeW);

        m_spinOpacity = new QSpinBox(m_specificContainer);
        m_spinOpacity->setRange(0, 100);
        m_spinOpacity->setValue(path->opacityPercent());
        connect(m_spinOpacity, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, path](int value) {
            if (!m_updatingFromComponent) path->setOpacityPercent(value);
        });
        connect(m_spinOpacity, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Path Opacity");
        });
        form->addRow("Opacity (%):", m_spinOpacity);

        m_spinPathFlatten = new QSpinBox(m_specificContainer);
        m_spinPathFlatten->setRange(1, 256);
        m_spinPathFlatten->setValue(path->flattenSubdivisions());
        connect(m_spinPathFlatten, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, path](int value) {
            if (!m_updatingFromComponent) path->setFlattenSubdivisions(value);
        });
        connect(m_spinPathFlatten, &QSpinBox::editingFinished, this, [this]() {
            commitPropertyChange("Change Path Flattening");
        });
        form->addRow("Flatten subdivisions:", m_spinPathFlatten);

    } else if (auto ci = dynamic_cast<CustomComponentInstance*>(m_targetComponent)) {
        // ── Definition info (read-only) ────────────────────────────────────
        QLabel* defLabel = new QLabel(ci->definitionId(), m_specificContainer);
        defLabel->setStyleSheet("color:#8a9bb0;font-size:11px;");
        form->addRow("Definition:", defLabel);

        // ── Variant dropdown ────────────────────────────────────────────────
        m_variantCombo = new QComboBox(m_specificContainer);
        m_variantCombo->setStyleSheet(
            "QComboBox { background:#1c1f26; color:#e4ecf7; border:1px solid #2b2f38; "
            "border-radius:5px; padding:4px 10px; font-size:12px; } "
            "QComboBox QAbstractItemView { background:#1a1d24; color:#e0e5ee; "
            "selection-background-color:#1a73e8; selection-color:#fff; border:1px solid #2c313e; }"
        );
        // Populate from project-level definitions if available; fall back to defaults
        const QStringList defaultNames = {"Primary", "Secondary", "Danger"};
        m_variantCombo->addItems(defaultNames);
        m_variantCombo->setCurrentText(ci->activeVariantName());
        connect(m_variantCombo, &QComboBox::currentTextChanged,
                this, [this, ci](const QString& variantName) {
            if (m_updatingFromComponent) return;
            ci->setActiveVariantName(variantName);
            // Apply color overrides from built-in palette
            ComponentVariant v;
            v.name        = variantName;
            v.useFill     = true;
            v.useStroke   = true;
            v.useAccent   = true;
            if (variantName == "Primary") {
                v.fillColor = QColor("#1a73e8"); v.strokeColor = QColor("#0d47a1"); v.accentColor = Qt::white;
            } else if (variantName == "Secondary") {
                v.fillColor = QColor("#34a853"); v.strokeColor = QColor("#1b6e2e"); v.accentColor = Qt::white;
            } else if (variantName == "Danger") {
                v.fillColor = QColor("#ea4335"); v.strokeColor = QColor("#b31412"); v.accentColor = Qt::white;
            }
            ci->applyVariant(v);
            commitPropertyChange("Change Variant");
        });
        form->addRow("Variant:", m_variantCombo);
    }

    containerLayout->addLayout(form);
    m_specificLayout->addWidget(m_specificContainer);
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

int PropertiesPanel::specificEditorsLayoutCount() const {
    return m_specificLayout ? m_specificLayout->count() : 0;
}

void PropertiesPanel::updateBoardPins() {
    QStringList pins = HardwareBridge::instance().availablePins();
    QStringList items;
    items << "[None]";
    items << pins;

    auto updateCombo = [&items](QComboBox* cb) {
        if (!cb) return;
        QString cur = cb->currentText();
        bool b = cb->blockSignals(true);
        cb->clear();
        cb->addItems(items);
        int idx = cb->findText(cur);
        if (idx >= 0) cb->setCurrentIndex(idx);
        else cb->setCurrentIndex(0);
        cb->blockSignals(b);
    };

    updateCombo(m_comboGpioPin);
    updateCombo(m_comboPwmPin);
    updateCombo(m_comboAdcPin);
    updateCombo(m_comboSpiMiso);
    updateCombo(m_comboSpiMosi);
    updateCombo(m_comboSpiSck);
    updateCombo(m_comboSpiSs);
    updateCombo(m_comboI2cScl);
    updateCombo(m_comboI2cSda);
}

void PropertiesPanel::updateProtocolFieldsVisibility(const QString& protocol) {
    if (m_protocolNoneWidget) m_protocolNoneWidget->setVisible(protocol == "None");
    if (m_gpioWidget) m_gpioWidget->setVisible(protocol == "GPIO");
    if (m_pwmWidget) m_pwmWidget->setVisible(protocol == "PWM");
    if (m_adcWidget) m_adcWidget->setVisible(protocol == "ADC");
    if (m_spiWidget) m_spiWidget->setVisible(protocol == "SPI");
    if (m_i2cWidget) m_i2cWidget->setVisible(protocol == "I2C");
}

void PropertiesPanel::onProtocolChanged(const QString& newProtocol) {
    if (m_updatingFromComponent || !m_targetComponent) return;

    m_targetComponent->setProtocol(newProtocol);
    updateProtocolFieldsVisibility(newProtocol);

    // Provide sensible defaults for freshly chosen protocol if pins not yet assigned
    if (newProtocol == "GPIO") {
        if (m_targetComponent->protocolPin("pin").isEmpty() && m_comboGpioPin) {
            m_targetComponent->setProtocolPin("pin", m_comboGpioPin->currentText());
        }
    } else if (newProtocol == "PWM") {
        if (m_targetComponent->protocolPin("pin").isEmpty() && m_comboPwmPin) {
            m_targetComponent->setProtocolPin("pin", m_comboPwmPin->currentText());
        }
    } else if (newProtocol == "ADC") {
        if (m_targetComponent->protocolPin("pin").isEmpty() && m_comboAdcPin) {
            m_targetComponent->setProtocolPin("pin", m_comboAdcPin->currentText());
        }
    } else if (newProtocol == "SPI") {
        if (m_targetComponent->protocolPin("miso").isEmpty() && m_comboSpiMiso) {
            int idx = m_comboSpiMiso->findText("PA6");
            if (idx >= 0) m_comboSpiMiso->setCurrentIndex(idx);
            m_targetComponent->setProtocolPin("miso", m_comboSpiMiso->currentText());
        }
        if (m_targetComponent->protocolPin("mosi").isEmpty() && m_comboSpiMosi) {
            int idx = m_comboSpiMosi->findText("PA7");
            if (idx >= 0) m_comboSpiMosi->setCurrentIndex(idx);
            m_targetComponent->setProtocolPin("mosi", m_comboSpiMosi->currentText());
        }
        if (m_targetComponent->protocolPin("sck").isEmpty() && m_comboSpiSck) {
            int idx = m_comboSpiSck->findText("PA5");
            if (idx >= 0) m_comboSpiSck->setCurrentIndex(idx);
            m_targetComponent->setProtocolPin("sck", m_comboSpiSck->currentText());
        }
        if (m_targetComponent->protocolPin("ss").isEmpty() && m_comboSpiSs) {
            int idx = m_comboSpiSs->findText("PA4");
            if (idx >= 0) m_comboSpiSs->setCurrentIndex(idx);
            m_targetComponent->setProtocolPin("ss", m_comboSpiSs->currentText());
        }
    } else if (newProtocol == "I2C") {
        if (m_targetComponent->protocolPin("scl").isEmpty() && m_comboI2cScl) {
            int idx = m_comboI2cScl->findText("PB8");
            if (idx >= 0) m_comboI2cScl->setCurrentIndex(idx);
            m_targetComponent->setProtocolPin("scl", m_comboI2cScl->currentText());
        }
        if (m_targetComponent->protocolPin("sda").isEmpty() && m_comboI2cSda) {
            int idx = m_comboI2cSda->findText("PB9");
            if (idx >= 0) m_comboI2cSda->setCurrentIndex(idx);
            m_targetComponent->setProtocolPin("sda", m_comboI2cSda->currentText());
        }
        if (m_targetComponent->protocolPin("address").isEmpty() && m_editI2cAddress) {
            m_targetComponent->setProtocolPin("address", m_editI2cAddress->text().trimmed());
        }
    }

    commitPropertyChange("Change Protocol");
}

void PropertiesPanel::onProtocolPinChanged() {
    if (m_updatingFromComponent || !m_targetComponent) return;

    QString proto = m_protocolCombo ? m_protocolCombo->currentText() : "None";
    if (proto == "GPIO") {
        if (m_comboGpioPin) m_targetComponent->setProtocolPin("pin", m_comboGpioPin->currentText());
    } else if (proto == "PWM") {
        if (m_comboPwmPin) m_targetComponent->setProtocolPin("pin", m_comboPwmPin->currentText());
    } else if (proto == "ADC") {
        if (m_comboAdcPin) m_targetComponent->setProtocolPin("pin", m_comboAdcPin->currentText());
    } else if (proto == "SPI") {
        if (m_comboSpiMiso) m_targetComponent->setProtocolPin("miso", m_comboSpiMiso->currentText());
        if (m_comboSpiMosi) m_targetComponent->setProtocolPin("mosi", m_comboSpiMosi->currentText());
        if (m_comboSpiSck)  m_targetComponent->setProtocolPin("sck",  m_comboSpiSck->currentText());
        if (m_comboSpiSs)   m_targetComponent->setProtocolPin("ss",   m_comboSpiSs->currentText());
    } else if (proto == "I2C") {
        if (m_comboI2cScl) m_targetComponent->setProtocolPin("scl", m_comboI2cScl->currentText());
        if (m_comboI2cSda) m_targetComponent->setProtocolPin("sda", m_comboI2cSda->currentText());
        if (m_editI2cAddress) m_targetComponent->setProtocolPin("address", m_editI2cAddress->text().trimmed());
    }

    commitPropertyChange("Change Protocol Pin Mapping");
}

