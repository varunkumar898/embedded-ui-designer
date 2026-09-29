#include "ColorPickerDialog.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFrame>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// PaletteIconWidget - Vector icon matching the "Preset Palette" tab
// ============================================================================

class PaletteIconWidget : public QWidget {
public:
    explicit PaletteIconWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setFixedSize(14, 14);
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(width() / 2.0, height() / 2.0);
        p.rotate(-35.0);

        QPen pen(QColor("#0ea5e9"), 1.8);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(-5.0, -3.8, 10.0, 7.6));

        // Diagonal inner slash / hole
        p.setPen(QPen(QColor("#0ea5e9"), 1.4));
        p.drawLine(QPointF(-1.8, -0.8), QPointF(1.8, 0.8));
    }
};

// ============================================================================
// ColorWheelWidget
// ============================================================================

ColorWheelWidget::ColorWheelWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(146, 146);
    setCursor(Qt::CrossCursor);
}

void ColorWheelWidget::setColor(const QColor& color) {
    if (!color.isValid()) return;
    float h, s, v;
    color.getHsvF(&h, &s, &v);
    m_hue = (h < 0) ? 0.0 : h;
    m_sat = std::clamp<qreal>(s, 0.0, 1.0);
    m_val = std::clamp<qreal>(v, 0.2, 1.0);
    update();
}

QColor ColorWheelWidget::color() const {
    return QColor::fromHsvF(m_hue, m_sat, m_val);
}

void ColorWheelWidget::setHarmonyColor(const QColor& color) {
    m_harmonyColor = color;
    float h, s, v;
    color.getHsvF(&h, &s, &v);
    m_harmonyHue = (h < 0) ? 0.0 : h;
    update();
}

void ColorWheelWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    renderWheelPixmap();
}

void ColorWheelWidget::renderWheelPixmap() {
    int w = width();
    int h = height();
    if (w <= 0 || h <= 0) return;

    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    qreal cx = w / 2.0;
    qreal cy = h / 2.0;
    qreal radius = 55.0;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            qreal dx = x - cx;
            qreal dy = y - cy;
            qreal dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= radius) {
                // Hue starts with Red at 12 o'clock, rotating clockwise
                qreal angle = std::atan2(dy, dx) + M_PI / 2.0;
                if (angle < 0.0) angle += 2.0 * M_PI;
                if (angle >= 2.0 * M_PI) angle -= 2.0 * M_PI;

                qreal hue = angle / (2.0 * M_PI);
                qreal sat = dist / radius;
                QColor c = QColor::fromHsvF(hue, sat, 1.0);

                if (dist > radius - 1.2) {
                    qreal alpha = (radius - dist) / 1.2;
                    c.setAlphaF(std::clamp(alpha, 0.0, 1.0));
                }
                img.setPixelColor(x, y, c);
            }
        }
    }
    m_wheelCache = QPixmap::fromImage(img);
}

void ColorWheelWidget::updateFromPoint(const QPointF& pos) {
    qreal cx = width() / 2.0;
    qreal cy = height() / 2.0;
    qreal radius = 55.0;

    qreal dx = pos.x() - cx;
    qreal dy = pos.y() - cy;
    qreal dist = std::sqrt(dx * dx + dy * dy);

    qreal angle = std::atan2(dy, dx) + M_PI / 2.0;
    if (angle < 0.0) angle += 2.0 * M_PI;
    if (angle >= 2.0 * M_PI) angle -= 2.0 * M_PI;

    m_hue = angle / (2.0 * M_PI);
    m_sat = std::clamp(dist / radius, 0.0, 1.0);

    update();
    emit colorChanged(color());
}

void ColorWheelWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        updateFromPoint(event->position());
    }
}

void ColorWheelWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() & Qt::LeftButton) {
        updateFromPoint(event->position());
    }
}

void ColorWheelWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    if (m_wheelCache.isNull()) {
        renderWheelPixmap();
    }
    p.drawPixmap(0, 0, m_wheelCache);

    qreal cx = width() / 2.0;
    qreal cy = height() / 2.0;
    qreal radius = 55.0;

    // Dark outer track ring
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(15, 23, 42), 3.0));
    p.drawEllipse(QPointF(cx, cy), radius + 1.5, radius + 1.5);

    // Rim marker dot at ~105 deg (or current hue if interacting)
    qreal rimAngle = 105.0 * M_PI / 180.0 - M_PI / 2.0;
    qreal rimX = cx + (radius + 1.5) * std::cos(rimAngle);
    qreal rimY = cy + (radius + 1.5) * std::sin(rimAngle);
    p.setBrush(Qt::white);
    p.setPen(QPen(QColor(15, 23, 42), 1.5));
    p.drawEllipse(QPointF(rimX, rimY), 3.2, 3.2);

    // Harmony indicator ring (secondary ring at opposite hue, filled with harmony color)
    qreal harmAngle = m_harmonyHue * 2.0 * M_PI - M_PI / 2.0;
    qreal harmDist = std::clamp(m_sat, 0.25, 0.72) * radius;
    qreal harmX = cx + harmDist * std::cos(harmAngle);
    qreal harmY = cy + harmDist * std::sin(harmAngle);
    p.setBrush(m_harmonyColor);
    p.setPen(QPen(Qt::white, 2.2));
    p.drawEllipse(QPointF(harmX, harmY), 6.5, 6.5);

    // Base indicator ring (primary ring at base hue, filled with base color)
    qreal baseAngle = m_hue * 2.0 * M_PI - M_PI / 2.0;
    qreal baseDist = std::clamp(m_sat, 0.25, 0.72) * radius;
    qreal baseX = cx + baseDist * std::cos(baseAngle);
    qreal baseY = cy + baseDist * std::sin(baseAngle);
    p.setBrush(color());
    p.setPen(QPen(Qt::white, 2.5));
    p.drawEllipse(QPointF(baseX, baseY), 8.0, 8.0);
}

// ============================================================================
// ColorPickerDialog
// ============================================================================

ColorPickerDialog::ColorPickerDialog(const QColor& initialColor, QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
    , m_baseColor(initialColor.isValid() ? initialColor : QColor("#1ECBE1"))
    , m_selectedColor(m_baseColor)
{
    setupUi();
    m_wheelWidget->setColor(m_baseColor);
    updateHexAndRgb565();
    updateHarmonySwatches();
}

QString ColorPickerDialog::toRgb565Hex(const QColor& c) {
    int r = c.red();
    int g = c.green();
    int b = c.blue();
    uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return QString("0x%1").arg(rgb565, 4, 16, QChar('0')).toUpper();
}

void ColorPickerDialog::setupUi() {
    setFixedSize(520, 380);
    setStyleSheet(
        "ColorPickerDialog { background-color: #0b0f17; border: 1px solid #1e2638; border-radius: 8px; } "
        "QWidget { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; }"
    );

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 12, 14, 12);
    rootLayout->setSpacing(8);

    // 1. Top Title Bar (Cancel, Title, Select)
    QHBoxLayout* titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setStyleSheet(
        "QPushButton { background: #18202c; color: #94a3b8; border: 1px solid #283446; border-radius: 4px; padding: 5px 15px; font-weight: 600; font-size: 11.5px; } "
        "QPushButton:hover { background: #222b3b; color: #ffffff; border-color: #3b4961; }"
    );
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    QLabel* titleLabel = new QLabel("Choose Color", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #ffffff; font-size: 13px; font-weight: bold;");

    QPushButton* selectBtn = new QPushButton("Select", this);
    selectBtn->setStyleSheet(
        "QPushButton { background: #0284c7; color: #ffffff; border: none; border-radius: 4px; padding: 5px 18px; font-weight: bold; font-size: 11.5px; } "
        "QPushButton:hover { background: #0ea5e9; }"
    );
    connect(selectBtn, &QPushButton::clicked, this, &QDialog::accept);

    titleLayout->addWidget(cancelBtn);
    titleLayout->addStretch(1);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch(1);
    titleLayout->addWidget(selectBtn);
    rootLayout->addLayout(titleLayout);

    // Full-width subtle horizontal line under title bar
    QFrame* titleSep = new QFrame(this);
    titleSep->setFrameShape(QFrame::HLine);
    titleSep->setStyleSheet("color: #16202c; background-color: #16202c; height: 1px; border: none;");
    rootLayout->addWidget(titleSep);

    // 2. Tab Bar ("Preset Palette" with active indicator line)
    QWidget* tabContainer = new QWidget(this);
    QVBoxLayout* tabContainerLayout = new QVBoxLayout(tabContainer);
    tabContainerLayout->setContentsMargins(0, 2, 0, 0);
    tabContainerLayout->setSpacing(4);

    QHBoxLayout* tabLayout = new QHBoxLayout();
    tabLayout->setContentsMargins(2, 0, 0, 0);
    tabLayout->setSpacing(6);

    PaletteIconWidget* paletteIcon = new PaletteIconWidget(tabContainer);
    QLabel* tabLabel = new QLabel("Preset Palette", tabContainer);
    tabLabel->setStyleSheet("color: #38bdf8; font-size: 11.5px; font-weight: 600;");

    tabLayout->addWidget(paletteIcon);
    tabLayout->addWidget(tabLabel);
    tabLayout->addStretch(1);
    tabContainerLayout->addLayout(tabLayout);

    // Cyan Active Tab Underline
    QWidget* activeBar = new QWidget(tabContainer);
    activeBar->setFixedSize(108, 2);
    activeBar->setStyleSheet("background-color: #0ea5e9;");
    tabContainerLayout->addWidget(activeBar);

    rootLayout->addWidget(tabContainer);

    // 3. Inner Card Area (Color Wheel & Harmony Builder)
    QWidget* card = new QWidget(this);
    card->setStyleSheet("QWidget#colorCard { background-color: #0d131e; border: 1px solid #1a2333; border-radius: 8px; }");
    card->setObjectName("colorCard");
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->setSpacing(10);

    // Card Header
    QHBoxLayout* cardHeader = new QHBoxLayout();
    QLabel* cardTitle = new QLabel("COLOR WHEEL & HARMONY BUILDER", card);
    cardTitle->setStyleSheet("color: #8fa2b8; font-size: 10px; font-weight: bold; font-family: monospace; letter-spacing: 0.5px;");

    QLabel* chromaBadge = new QLabel("LV_COLOR_CHROMA", card);
    chromaBadge->setStyleSheet(
        "background-color: #071f30; color: #0ea5e9; border: 1px solid #0369a1; "
        "border-radius: 3px; padding: 2px 7px; font-size: 9px; font-weight: bold; font-family: monospace;"
    );

    cardHeader->addWidget(cardTitle);
    cardHeader->addStretch(1);
    cardHeader->addWidget(chromaBadge);
    cardLayout->addLayout(cardHeader);

    // Middle Row: Wheel on left, controls on right
    QHBoxLayout* middleLayout = new QHBoxLayout();
    middleLayout->setSpacing(16);

    m_wheelWidget = new ColorWheelWidget(card);
    connect(m_wheelWidget, &ColorWheelWidget::colorChanged, this, &ColorPickerDialog::onWheelColorChanged);
    middleLayout->addWidget(m_wheelWidget);

    QVBoxLayout* controlsLayout = new QVBoxLayout();
    controlsLayout->setSpacing(8);

    // Step 1: Pick a color
    QLabel* step1 = new QLabel("1. Pick a color", card);
    step1->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: 600;");
    controlsLayout->addWidget(step1);

    QHBoxLayout* pickLayout = new QHBoxLayout();
    pickLayout->setSpacing(8);

    m_circleSwatch = new QWidget(card);
    m_circleSwatch->setFixedSize(24, 24);
    m_circleSwatch->setStyleSheet(QString("background-color: %1; border-radius: 12px; border: 1px solid rgba(255,255,255,0.25);").arg(m_baseColor.name()));
    pickLayout->addWidget(m_circleSwatch);

    QWidget* hexBox = new QWidget(card);
    hexBox->setStyleSheet("background-color: #101722; border: 1px solid #1e293b; border-radius: 4px;");
    QHBoxLayout* hexBoxLayout = new QHBoxLayout(hexBox);
    hexBoxLayout->setContentsMargins(8, 2, 8, 2);

    m_hexEdit = new QLineEdit(m_baseColor.name().toUpper(), hexBox);
    m_hexEdit->setStyleSheet("color: #ffffff; font-size: 11.5px; font-weight: bold; font-family: monospace; border: none; background: transparent;");
    connect(m_hexEdit, &QLineEdit::textEdited, this, &ColorPickerDialog::onHexEdited);

    m_rgb565Label = new QLabel(QString("RGB565: %1").arg(toRgb565Hex(m_baseColor)), hexBox);
    m_rgb565Label->setStyleSheet("color: #64748b; font-size: 9.5px; font-family: monospace;");

    hexBoxLayout->addWidget(m_hexEdit, 1);
    hexBoxLayout->addWidget(m_rgb565Label);
    pickLayout->addWidget(hexBox, 1);
    controlsLayout->addLayout(pickLayout);

    // Step 2: Choose color harmony
    QLabel* step2 = new QLabel("2. Choose color harmony", card);
    step2->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: 600; padding-top: 2px;");
    controlsLayout->addWidget(step2);

    m_harmonyCombo = new QComboBox(card);
    m_harmonyCombo->addItem("Complementary");
    m_harmonyCombo->addItem("Monochromatic");
    m_harmonyCombo->addItem("Analogous");
    m_harmonyCombo->addItem("Triadic");
    m_harmonyCombo->addItem("Tetradic");
    m_harmonyCombo->setStyleSheet(
        "QComboBox { background-color: #101722; color: #ffffff; border: 1px solid #1e293b; border-radius: 4px; padding: 5px 10px; font-size: 11.5px; font-weight: 500; } "
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 24px; border: none; } "
        "QComboBox::down-arrow { image: url(:/combo_arrow.png); width: 10px; height: 6px; margin-right: 6px; } "
        "QComboBox QAbstractItemView { background-color: #101722; color: #e2e8f0; border: 1px solid #1e293b; selection-background-color: #0284c7; }"
    );
    connect(m_harmonyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ColorPickerDialog::onHarmonyModeChanged);
    controlsLayout->addWidget(m_harmonyCombo);

    controlsLayout->addStretch(1);
    middleLayout->addLayout(controlsLayout, 1);
    cardLayout->addLayout(middleLayout);

    // Bottom Harmony Swatches Bar
    QWidget* swatchesContainer = new QWidget(card);
    QVBoxLayout* swatchesContainerLayout = new QVBoxLayout(swatchesContainer);
    swatchesContainerLayout->setContentsMargins(0, 0, 0, 0);
    swatchesContainerLayout->setSpacing(2);

    m_swatchesBarLayout = new QHBoxLayout();
    m_swatchesBarLayout->setContentsMargins(0, 0, 0, 0);
    m_swatchesBarLayout->setSpacing(0);
    swatchesContainerLayout->addLayout(m_swatchesBarLayout);

    m_swatchesHexLayout = new QHBoxLayout();
    m_swatchesHexLayout->setContentsMargins(4, 0, 4, 0);
    swatchesContainerLayout->addLayout(m_swatchesHexLayout);

    cardLayout->addWidget(swatchesContainer);
    rootLayout->addWidget(card);
}

void ColorPickerDialog::onWheelColorChanged(const QColor& color) {
    m_baseColor = color;
    updateHexAndRgb565();
    updateHarmonySwatches();
}

void ColorPickerDialog::onHexEdited(const QString& text) {
    QString hex = text.trimmed();
    if (!hex.startsWith('#')) hex = "#" + hex;
    QColor c(hex);
    if (c.isValid()) {
        m_baseColor = c;
        m_wheelWidget->setColor(c);
        updateHexAndRgb565();
        updateHarmonySwatches();
    }
}

void ColorPickerDialog::onHarmonyModeChanged(int index) {
    m_harmonyMode = index;
    m_activeSwatchIndex = 0;
    updateHarmonySwatches();
}

void ColorPickerDialog::updateHexAndRgb565() {
    m_hexEdit->setText(m_baseColor.name().toUpper());
    m_rgb565Label->setText(QString("RGB565: %1").arg(toRgb565Hex(m_baseColor)));
    m_circleSwatch->setStyleSheet(QString("background-color: %1; border-radius: 12px; border: 1px solid rgba(255,255,255,0.25);").arg(m_baseColor.name()));
}

void ColorPickerDialog::updateHarmonySwatches() {
    // Clear previous swatches layout
    QLayoutItem* child;
    while ((child = m_swatchesBarLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }
    while ((child = m_swatchesHexLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }
    m_swatches.clear();

    float h, s, v;
    m_baseColor.getHsvF(&h, &s, &v);
    if (h < 0) h = 0.0f;

    std::vector<std::pair<QString, QColor>> colors;
    colors.push_back({"Base", m_baseColor});

    QColor harmColor = m_baseColor;

    if (m_harmonyMode == 0) {
        // Complementary: +180 deg
        qreal harmHue = std::fmod(h + 0.5, 1.0);
        harmColor = QColor::fromHsvF(harmHue, s, v);
        colors.push_back({"Harmony", harmColor});
    } else if (m_harmonyMode == 1) {
        // Monochromatic: darker & lighter
        qreal vDark = std::max(0.2, v * 0.65);
        qreal vLight = std::min(1.0, v * 1.35);
        harmColor = QColor::fromHsvF(h, s, vDark);
        colors.push_back({"Dark", harmColor});
        colors.push_back({"Light", QColor::fromHsvF(h, std::max(0.1, s * 0.5), vLight)});
    } else if (m_harmonyMode == 2) {
        // Analogous: +30 deg, -30 deg
        qreal a1 = std::fmod(h + (30.0 / 360.0), 1.0);
        qreal a2 = std::fmod(h - (30.0 / 360.0) + 1.0, 1.0);
        harmColor = QColor::fromHsvF(a1, s, v);
        colors.push_back({"Analog 1", harmColor});
        colors.push_back({"Analog 2", QColor::fromHsvF(a2, s, v)});
    } else if (m_harmonyMode == 3) {
        // Triadic: +120 deg, +240 deg
        qreal t1 = std::fmod(h + (120.0 / 360.0), 1.0);
        qreal t2 = std::fmod(h + (240.0 / 360.0), 1.0);
        harmColor = QColor::fromHsvF(t1, s, v);
        colors.push_back({"Triad 1", harmColor});
        colors.push_back({"Triad 2", QColor::fromHsvF(t2, s, v)});
    } else if (m_harmonyMode == 4) {
        // Tetradic: +90, +180, +270
        qreal q1 = std::fmod(h + 0.25, 1.0);
        qreal q2 = std::fmod(h + 0.50, 1.0);
        qreal q3 = std::fmod(h + 0.75, 1.0);
        harmColor = QColor::fromHsvF(q2, s, v);
        colors.push_back({"Tetra 1", QColor::fromHsvF(q1, s, v)});
        colors.push_back({"Tetra 2", harmColor});
        colors.push_back({"Tetra 3", QColor::fromHsvF(q3, s, v)});
    }

    m_wheelWidget->setHarmonyColor(harmColor);

    size_t count = colors.size();
    for (size_t i = 0; i < count; ++i) {
        SwatchData sd;
        sd.name = colors[i].first;
        sd.color = colors[i].second;
        sd.hex = sd.color.name().toUpper();

        // Outer focus wrapper
        QWidget* wrapper = new QWidget(this);
        QVBoxLayout* wLayout = new QVBoxLayout(wrapper);
        wLayout->setContentsMargins(2, 2, 2, 2);
        wLayout->setSpacing(0);

        QPushButton* btn = new QPushButton(wrapper);
        btn->setFixedHeight(32);
        btn->setCursor(Qt::PointingHandCursor);

        // Pill badge inside button
        QHBoxLayout* btnLayout = new QHBoxLayout(btn);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        QLabel* pill = new QLabel(sd.name, btn);
        pill->setAlignment(Qt::AlignCenter);

        if (i == 0) {
            pill->setStyleSheet("background-color: #cbf5fb; color: #08283b; font-weight: bold; border-radius: 4px; font-size: 10px; padding: 2px 10px;");
        } else {
            pill->setStyleSheet("background-color: rgba(0,0,0,0.5); color: #f8fafc; font-weight: bold; border-radius: 4px; font-size: 10px; padding: 2px 10px;");
        }
        btnLayout->addWidget(pill, 0, Qt::AlignCenter);

        wLayout->addWidget(btn);

        sd.wrapper = wrapper;
        sd.swatchBtn = btn;
        sd.pill = pill;

        int idx = static_cast<int>(i);
        connect(btn, &QPushButton::clicked, this, [this, idx]() {
            selectSwatch(idx);
        });

        m_swatches.push_back(sd);
        m_swatchesBarLayout->addWidget(wrapper, 1);
    }

    // Add hex labels under swatches
    if (count == 2) {
        QLabel* leftHex = new QLabel(m_swatches[0].hex, this);
        leftHex->setStyleSheet("color: #64748b; font-size: 9.5px; font-family: monospace;");
        leftHex->setAlignment(Qt::AlignLeft);

        QLabel* rightHex = new QLabel(m_swatches[1].hex, this);
        rightHex->setStyleSheet("color: #64748b; font-size: 9.5px; font-family: monospace;");
        rightHex->setAlignment(Qt::AlignRight);

        m_swatchesHexLayout->addWidget(leftHex);
        m_swatchesHexLayout->addStretch(1);
        m_swatchesHexLayout->addWidget(rightHex);
    } else {
        for (size_t i = 0; i < count; ++i) {
            QLabel* lbl = new QLabel(m_swatches[i].hex, this);
            lbl->setStyleSheet("color: #64748b; font-size: 9.5px; font-family: monospace;");
            lbl->setAlignment(Qt::AlignCenter);
            m_swatchesHexLayout->addWidget(lbl, 1);
        }
    }

    if (m_activeSwatchIndex >= static_cast<int>(m_swatches.size())) {
        m_activeSwatchIndex = 0;
    }
    selectSwatch(m_activeSwatchIndex);
}

void ColorPickerDialog::selectSwatch(int index) {
    if (index < 0 || index >= static_cast<int>(m_swatches.size())) return;
    m_activeSwatchIndex = index;
    m_selectedColor = m_swatches[index].color;

    size_t count = m_swatches.size();
    for (size_t i = 0; i < count; ++i) {
        QString hex = m_swatches[i].hex;
        QString leftRadius = (i == 0) ? "4px" : "0px";
        QString rightRadius = (i == count - 1) ? "4px" : "0px";

        m_swatches[i].swatchBtn->setStyleSheet(
            QString("QPushButton { background-color: %1; border: none; border-top-left-radius: %2; border-bottom-left-radius: %2; "
                    "border-top-right-radius: %3; border-bottom-right-radius: %3; outline: none; }")
            .arg(hex, leftRadius, rightRadius)
        );

        if (static_cast<int>(i) == m_activeSwatchIndex) {
            // Indigo/purple selection focus border around the active swatch wrapper
            m_swatches[i].wrapper->setStyleSheet("QWidget { border: 2px solid #7986cb; border-radius: 6px; }");
        } else {
            m_swatches[i].wrapper->setStyleSheet("QWidget { border: 2px solid transparent; border-radius: 6px; }");
        }
    }
}

QColor ColorPickerDialog::selectedColor() const {
    return m_selectedColor;
}

QColor ColorPickerDialog::getColor(const QColor& initial, QWidget* parent, const QString& title) {
    Q_UNUSED(title);
    ColorPickerDialog dlg(initial, parent);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.selectedColor();
    }
    return QColor(); // Invalid color if canceled
}
