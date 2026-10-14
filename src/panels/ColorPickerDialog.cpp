#include "ColorPickerDialog.h"
#include "theme/Theme.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFrame>
#include <QGroupBox>
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

        QPen pen(Theme::cAccent(), 1.8);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(-5.0, -3.8, 10.0, 7.6));

        // Diagonal inner slash / hole
        p.setPen(QPen(Theme::cAccent(), 1.4));
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

    // Rim marker dot
    qreal rimAngle = 105.0 * M_PI / 180.0 - M_PI / 2.0;
    qreal rimX = cx + (radius + 1.5) * std::cos(rimAngle);
    qreal rimY = cy + (radius + 1.5) * std::sin(rimAngle);
    p.setBrush(Qt::white);
    p.setPen(QPen(QColor(15, 23, 42), 1.5));
    p.drawEllipse(QPointF(rimX, rimY), 3.2, 3.2);

    // Harmony indicator ring
    qreal harmAngle = m_harmonyHue * 2.0 * M_PI - M_PI / 2.0;
    qreal harmDist = std::clamp(m_sat, 0.25, 0.72) * radius;
    qreal harmX = cx + harmDist * std::cos(harmAngle);
    qreal harmY = cy + harmDist * std::sin(harmAngle);
    p.setBrush(m_harmonyColor);
    p.setPen(QPen(Qt::white, 2.2));
    p.drawEllipse(QPointF(harmX, harmY), 6.5, 6.5);

    // Base indicator ring
    qreal baseAngle = m_hue * 2.0 * M_PI - M_PI / 2.0;
    qreal baseDist = std::clamp(m_sat, 0.25, 0.72) * radius;
    qreal baseX = cx + baseDist * std::cos(baseAngle);
    qreal baseY = cy + baseDist * std::sin(baseAngle);
    p.setBrush(color());
    p.setPen(QPen(Qt::white, 2.5));
    p.drawEllipse(QPointF(baseX, baseY), 8.0, 8.0);
}

// ============================================================================
// GradientPreviewWidget
// ============================================================================

GradientPreviewWidget::GradientPreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(48);
    setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: %3px;").arg(Theme::BgBase, Theme::BorderSubtle).arg(Theme::RadiusMd));
}

void GradientPreviewWidget::setGradient(const ComponentGradient& grad) {
    m_gradient = grad;
    update();
}

void GradientPreviewWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF r = rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(r, 6, 6);

    // Checkered background pattern for alpha visibility
    p.save();
    p.setClipPath(path);
    p.setBrush(Theme::cBgRaised());
    p.setPen(Qt::NoPen);
    p.drawRect(r);

    // Gradient fill
    p.setBrush(m_gradient.toQBrush(r));
    p.drawRect(r);
    p.restore();

    // Subtle border
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(Theme::cBorderStrong(), 1.0));
    p.drawRoundedRect(r, 6, 6);
}

// ============================================================================
// GradientStopBarWidget
// ============================================================================

GradientStopBarWidget::GradientStopBarWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(42);
    setCursor(Qt::PointingHandCursor);
}

void GradientStopBarWidget::setGradient(const ComponentGradient& grad) {
    m_gradient = grad;
    if (m_selectedIndex >= m_gradient.stops.size()) {
        m_selectedIndex = std::max(0, static_cast<int>(m_gradient.stops.size()) - 1);
    }
    update();
}

void GradientStopBarWidget::setSelectedStopIndex(int idx) {
    if (idx >= 0 && idx < m_gradient.stops.size() && m_selectedIndex != idx) {
        m_selectedIndex = idx;
        update();
        emit stopSelected(m_selectedIndex);
    }
}

int GradientStopBarWidget::stopIndexAt(const QPointF& pos) const {
    const qreal margin = 12.0;
    const qreal barW = width() - margin * 2.0;
    if (barW <= 0.0) return -1;

    for (int i = 0; i < m_gradient.stops.size(); ++i) {
        qreal sx = margin + m_gradient.stops[i].position * barW;
        qreal sy = 28.0; // Marker pin center
        qreal dx = pos.x() - sx;
        qreal dy = pos.y() - sy;
        if (dx * dx + dy * dy <= 10.0 * 10.0) {
            return i;
        }
    }
    return -1;
}

void GradientStopBarWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int idx = stopIndexAt(event->position());
        if (idx >= 0) {
            m_selectedIndex = idx;
            m_isDragging = true;
            m_draggedIndex = idx;
            update();
            emit stopSelected(m_selectedIndex);
        } else {
            const qreal margin = 12.0;
            const qreal barW = width() - margin * 2.0;
            if (barW > 0.0 && event->position().y() <= 22.0) {
                qreal pos = std::clamp((event->position().x() - margin) / barW, 0.0, 1.0);
                emit stopAddRequested(pos);
            }
        }
    }
}

void GradientStopBarWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging && m_draggedIndex >= 0 && (event->buttons() & Qt::LeftButton)) {
        const qreal margin = 12.0;
        const qreal barW = width() - margin * 2.0;
        if (barW > 0.0) {
            qreal newPos = std::clamp((event->position().x() - margin) / barW, 0.0, 1.0);
            m_gradient.stops[m_draggedIndex].position = newPos;
            update();
            emit stopMoved(m_draggedIndex, newPos);
        }
    }
}

void GradientStopBarWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_isDragging) {
        m_isDragging = false;
        m_draggedIndex = -1;
    }
}

void GradientStopBarWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    const qreal margin = 12.0;
    const qreal barW = width() - margin * 2.0;
    if (barW > 0.0) {
        qreal pos = std::clamp((event->position().x() - margin) / barW, 0.0, 1.0);
        emit stopAddRequested(pos);
    }
}

void GradientStopBarWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal margin = 12.0;
    const qreal barW = width() - margin * 2.0;
    const QRectF barRect(margin, 2.0, barW, 18.0);

    // 1. Draw horizontal gradient bar
    QLinearGradient lg(barRect.left(), barRect.top(), barRect.right(), barRect.top());
    for (const auto& s : m_gradient.stops) {
        lg.setColorAt(std::clamp(s.position, 0.0, 1.0), s.color);
    }

    p.setBrush(QBrush(lg));
    p.setPen(QPen(Theme::cBorderStrong(), 1.0));
    p.drawRoundedRect(barRect, 4, 4);

    // 2. Draw stop marker pins
    for (int i = 0; i < m_gradient.stops.size(); ++i) {
        const auto& s = m_gradient.stops[i];
        qreal sx = margin + s.position * barW;
        qreal sy = 29.0;
        bool isSel = (i == m_selectedIndex);

        // Marker pin pointer triangle
        QPolygonF tri;
        tri << QPointF(sx, 21.0)
            << QPointF(sx - 5.0, 26.0)
            << QPointF(sx + 5.0, 26.0);
        p.setBrush(isSel ? Theme::cAccent() : Theme::cTextMuted());
        p.setPen(Qt::NoPen);
        p.drawPolygon(tri);

        // Marker circle
        p.setBrush(QBrush(s.color));
        p.setPen(QPen(isSel ? Theme::cAccent() : Theme::cBorderSubtle(), isSel ? 2.5 : 1.5));
        p.drawEllipse(QPointF(sx, sy), isSel ? 6.5 : 5.0, isSel ? 6.5 : 5.0);
    }
}

// ============================================================================
// ColorPickerDialog
// ============================================================================

ColorPickerDialog::ColorPickerDialog(QWidget* parent)
    : ColorPickerDialog(QColor(Theme::Accent), parent)
{
}

ColorPickerDialog::ColorPickerDialog(const QColor& initialColor, QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
    , m_baseColor(initialColor.isValid() ? initialColor : QColor(Theme::Accent))
    , m_selectedColor(m_baseColor)
{
    setupUi();
    m_wheelWidget->setColor(m_baseColor);
    updateHexAndRgb565();
    updateHarmonySwatches();
}

QColor ColorPickerDialog::selectedColor() const {
    return m_selectedColor;
}

void ColorPickerDialog::setSelectedColor(const QColor& c) {
    if (!c.isValid()) return;
    m_baseColor = c;
    m_selectedColor = c;
    if (m_wheelWidget) m_wheelWidget->setColor(c);
    updateHexAndRgb565();
    updateHarmonySwatches();
}

void ColorPickerDialog::setGradient(const ComponentGradient& grad) {
    m_gradient = grad;
    m_isGradientMode = true;
    setGradientMode(true);
    updateGradientUi();
}

void ColorPickerDialog::setGradientMode(bool gradMode) {
    m_isGradientMode = gradMode;
    m_btnModeSolid->setStyleSheet(
        !m_isGradientMode 
        ? QString("QPushButton { background: %1; color: %2; border: none; border-radius: %3px; padding: 4px 14px; font-weight: 600; font-size: 11px; }").arg(Theme::Accent, Theme::BgBase).arg(Theme::RadiusSm)
        : QString("QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 4px 14px; font-weight: 500; font-size: 11px; }").arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle).arg(Theme::RadiusSm)
    );
    m_btnModeGradient->setStyleSheet(
        m_isGradientMode
        ? QString("QPushButton { background: %1; color: %2; border: none; border-radius: %3px; padding: 4px 14px; font-weight: 600; font-size: 11px; }").arg(Theme::Accent, Theme::BgBase).arg(Theme::RadiusSm)
        : QString("QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 4px 14px; font-weight: 500; font-size: 11px; }").arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle).arg(Theme::RadiusSm)
    );
    m_modeStack->setCurrentIndex(m_isGradientMode ? 1 : 0);
    if (m_isGradientMode) {
        updateGradientUi();
    }
}

void ColorPickerDialog::onModeSolidClicked() {
    setGradientMode(false);
}

void ColorPickerDialog::onModeGradientClicked() {
    setGradientMode(true);
}

QString ColorPickerDialog::toRgb565Hex(const QColor& c) {
    int r = c.red();
    int g = c.green();
    int b = c.blue();
    uint16_t rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return QString("0x%1").arg(QString("%1").arg(rgb565, 4, 16, QChar('0')).toUpper());
}

QColor ColorPickerDialog::getColor(const QColor& initial, QWidget* parent, const QString& /*title*/) {
    ColorPickerDialog dlg(initial, parent);
    if (dlg.exec() == QDialog::Accepted) {
        return dlg.selectedColor();
    }
    return QColor();
}

void ColorPickerDialog::setupUi() {
    setFixedSize(560, 480);
    setStyleSheet(QString(
        "ColorPickerDialog { background-color: %1; border: 1px solid %2; border-radius: %3px; } "
        "QWidget { color: %4; } "
        "QLabel { font-size: 11px; } "
        "QLineEdit, QSpinBox, QComboBox { background-color: %5; color: %4; border: 1px solid %2; border-radius: %6px; padding: 3px 6px; font-size: 11px; } "
        "QComboBox QAbstractItemView { background-color: %1; selection-background-color: %7; color: %4; }")
        .arg(Theme::BgSurface, Theme::BorderSubtle)
        .arg(Theme::RadiusLg)
        .arg(Theme::TextPrimary, Theme::BgRaised)
        .arg(Theme::RadiusSm)
        .arg(Theme::AccentSoft)
    );

    QVBoxLayout* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(14, 12, 14, 12);
    rootLayout->setSpacing(8);

    // 1. Top Title Bar (Cancel, Mode Toggle, Select)
    QHBoxLayout* titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 0);

    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 5px 15px; font-weight: 500; font-size: 11.5px; } "
        "QPushButton:hover { background: %5; color: %6; border-color: %7; }")
        .arg(Theme::BgRaised, Theme::TextSecondary, Theme::BorderSubtle)
        .arg(Theme::RadiusSm)
        .arg(Theme::BgOverlay, Theme::TextPrimary, Theme::BorderStrong)
    );
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    // Mode Selector (Section 2)
    QWidget* modeWidget = new QWidget(this);
    QHBoxLayout* modeLayout = new QHBoxLayout(modeWidget);
    modeLayout->setContentsMargins(0, 0, 0, 0);
    modeLayout->setSpacing(4);

    m_btnModeSolid = new QPushButton("Solid Color", modeWidget);
    m_btnModeGradient = new QPushButton("Gradient", modeWidget);
    modeLayout->addWidget(m_btnModeSolid);
    modeLayout->addWidget(m_btnModeGradient);

    connect(m_btnModeSolid, &QPushButton::clicked, this, &ColorPickerDialog::onModeSolidClicked);
    connect(m_btnModeGradient, &QPushButton::clicked, this, &ColorPickerDialog::onModeGradientClicked);

    QPushButton* selectBtn = new QPushButton("Select", this);
    selectBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: none; border-radius: %3px; padding: 5px 18px; font-weight: 600; font-size: 11.5px; } "
        "QPushButton:hover { background: %4; }")
        .arg(Theme::Accent, Theme::BgBase)
        .arg(Theme::RadiusSm)
        .arg(Theme::AccentHover)
    );
    connect(selectBtn, &QPushButton::clicked, this, &QDialog::accept);

    titleLayout->addWidget(cancelBtn);
    titleLayout->addStretch(1);
    titleLayout->addWidget(modeWidget);
    titleLayout->addStretch(1);
    titleLayout->addWidget(selectBtn);
    rootLayout->addLayout(titleLayout);

    // Subtle divider
    QFrame* titleSep = new QFrame(this);
    titleSep->setFrameShape(QFrame::HLine);
    titleSep->setStyleSheet(QString("color: %1; background-color: %1; height: 1px; border: none;").arg(Theme::BorderSubtle));
    rootLayout->addWidget(titleSep);

    // Central Mode Stack
    m_modeStack = new QStackedWidget(this);

    // ========================================================================
    // Page 0: Solid Color Mode
    // ========================================================================
    m_solidContainer = new QWidget(m_modeStack);
    QVBoxLayout* solidLayout = new QVBoxLayout(m_solidContainer);
    solidLayout->setContentsMargins(0, 0, 0, 0);
    solidLayout->setSpacing(8);

    // Tab Bar
    QWidget* tabContainer = new QWidget(m_solidContainer);
    QVBoxLayout* tabContainerLayout = new QVBoxLayout(tabContainer);
    tabContainerLayout->setContentsMargins(0, 2, 0, 0);
    tabContainerLayout->setSpacing(4);

    QHBoxLayout* tabLayout = new QHBoxLayout();
    tabLayout->setContentsMargins(2, 0, 0, 0);
    tabLayout->setSpacing(6);

    PaletteIconWidget* paletteIcon = new PaletteIconWidget(tabContainer);
    QLabel* tabLabel = new QLabel("Preset Palette", tabContainer);
    tabLabel->setStyleSheet(QString("color: %1; font-size: 11.5px; font-weight: 600;").arg(Theme::AccentText));

    tabLayout->addWidget(paletteIcon);
    tabLayout->addWidget(tabLabel);
    tabLayout->addStretch(1);
    tabContainerLayout->addLayout(tabLayout);

    QWidget* activeBar = new QWidget(tabContainer);
    activeBar->setFixedSize(108, 2);
    activeBar->setStyleSheet(QString("background-color: %1;").arg(Theme::Accent));
    tabContainerLayout->addWidget(activeBar);
    solidLayout->addWidget(tabContainer);

    // Inner Card
    QWidget* card = new QWidget(m_solidContainer);
    card->setStyleSheet(QString("QWidget#colorCard { background-color: %1; border: 1px solid %2; border-radius: %3px; }").arg(Theme::BgRaised, Theme::BorderSubtle).arg(Theme::RadiusLg));
    card->setObjectName("colorCard");
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 12);
    cardLayout->setSpacing(10);

    QHBoxLayout* cardHeader = new QHBoxLayout();
    QLabel* cardTitle = new QLabel("COLOR WHEEL & HARMONY BUILDER", card);
    cardTitle->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: 600; font-family: monospace; letter-spacing: 0.5px;").arg(Theme::TextMuted));

    QLabel* chromaBadge = new QLabel("LV_COLOR_CHROMA", card);
    chromaBadge->setStyleSheet(QString(
        "background-color: %1; color: %2; border: 1px solid %3; "
        "border-radius: 3px; padding: 2px 7px; font-size: 9px; font-weight: 600; font-family: monospace;")
        .arg(Theme::AccentSoft, Theme::AccentText, Theme::Accent)
    );
    cardHeader->addWidget(cardTitle);
    cardHeader->addStretch(1);
    cardHeader->addWidget(chromaBadge);
    cardLayout->addLayout(cardHeader);

    QHBoxLayout* middleLayout = new QHBoxLayout();
    middleLayout->setSpacing(16);

    m_wheelWidget = new ColorWheelWidget(card);
    connect(m_wheelWidget, &ColorWheelWidget::colorChanged, this, &ColorPickerDialog::onWheelColorChanged);
    middleLayout->addWidget(m_wheelWidget);

    QVBoxLayout* controlsLayout = new QVBoxLayout();
    controlsLayout->setSpacing(8);

    QLabel* step1 = new QLabel("1. Pick a color", card);
    step1->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600;").arg(Theme::TextSecondary));
    controlsLayout->addWidget(step1);

    QHBoxLayout* pickLayout = new QHBoxLayout();
    pickLayout->setSpacing(8);

    m_circleSwatch = new QWidget(card);
    m_circleSwatch->setFixedSize(24, 24);
    m_circleSwatch->setStyleSheet(QString("background-color: %1; border-radius: 12px; border: 1px solid rgba(255,255,255,0.25);").arg(m_baseColor.name()));
    pickLayout->addWidget(m_circleSwatch);

    QWidget* hexBox = new QWidget(card);
    hexBox->setStyleSheet(QString("background-color: %1; border: 1px solid %2; border-radius: %3px;").arg(Theme::BgBase, Theme::BorderSubtle).arg(Theme::RadiusSm));
    QHBoxLayout* hexBoxLayout = new QHBoxLayout(hexBox);
    hexBoxLayout->setContentsMargins(8, 2, 8, 2);

    m_hexEdit = new QLineEdit(m_baseColor.name().toUpper(), hexBox);
    m_hexEdit->setStyleSheet(QString("color: %1; font-size: 11.5px; font-weight: 600; font-family: monospace; border: none; background: transparent;").arg(Theme::TextPrimary));
    connect(m_hexEdit, &QLineEdit::textEdited, this, &ColorPickerDialog::onHexEdited);

    m_rgb565Label = new QLabel(QString("RGB565: %1").arg(toRgb565Hex(m_baseColor)), hexBox);
    m_rgb565Label->setStyleSheet(QString("color: %1; font-size: 9.5px; font-family: monospace;").arg(Theme::TextMuted));
    hexBoxLayout->addWidget(m_hexEdit);
    hexBoxLayout->addWidget(m_rgb565Label);
    pickLayout->addWidget(hexBox, 1);
    controlsLayout->addLayout(pickLayout);

    QLabel* step2 = new QLabel("2. Select harmony", card);
    step2->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: 600;").arg(Theme::TextSecondary));
    controlsLayout->addWidget(step2);

    m_harmonyCombo = new QComboBox(card);
    m_harmonyCombo->addItem("Complementary (2 colors)");
    m_harmonyCombo->addItem("Monochromatic (4 shades)");
    m_harmonyCombo->addItem("Analogous (3 adjacent)");
    m_harmonyCombo->addItem("Triadic (3 balanced)");
    m_harmonyCombo->addItem("Tetradic (4 colors)");
    connect(m_harmonyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ColorPickerDialog::onHarmonyModeChanged);
    controlsLayout->addWidget(m_harmonyCombo);

    middleLayout->addLayout(controlsLayout, 1);
    cardLayout->addLayout(middleLayout);

    // Swatches Bar
    m_swatchesBarLayout = new QHBoxLayout();
    m_swatchesBarLayout->setSpacing(4);
    cardLayout->addLayout(m_swatchesBarLayout);

    m_swatchesHexLayout = new QHBoxLayout();
    m_swatchesHexLayout->setSpacing(4);
    cardLayout->addLayout(m_swatchesHexLayout);

    solidLayout->addWidget(card);
    m_modeStack->addWidget(m_solidContainer);

    // ========================================================================
    // Page 1: Gradient Mode (Sections 3, 4, 5, 6)
    // ========================================================================
    m_gradientContainer = new QWidget(m_modeStack);
    QVBoxLayout* gradLayout = new QVBoxLayout(m_gradientContainer);
    gradLayout->setContentsMargins(0, 0, 0, 0);
    gradLayout->setSpacing(8);

    QWidget* gradCard = new QWidget(m_gradientContainer);
    gradCard->setStyleSheet(QString("QWidget#gradCard { background-color: %1; border: 1px solid %2; border-radius: %3px; }").arg(Theme::BgRaised, Theme::BorderSubtle).arg(Theme::RadiusLg));
    gradCard->setObjectName("gradCard");
    QVBoxLayout* gcLayout = new QVBoxLayout(gradCard);
    gcLayout->setContentsMargins(12, 10, 12, 10);
    gcLayout->setSpacing(8);

    // Top Controls: Gradient Type & Presets
    QHBoxLayout* topGradRow = new QHBoxLayout();
    topGradRow->setSpacing(8);

    QLabel* typeLbl = new QLabel("Type:", gradCard);
    typeLbl->setStyleSheet(QString("font-weight: 600; color: %1;").arg(Theme::TextSecondary));
    m_gradientTypeCombo = new QComboBox(gradCard);
    m_gradientTypeCombo->addItem("Linear Gradient", "linear");
    m_gradientTypeCombo->addItem("Radial Gradient", "radial");
    connect(m_gradientTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ColorPickerDialog::onGradientTypeChanged);

    QLabel* presetLbl = new QLabel("Preset:", gradCard);
    presetLbl->setStyleSheet(QString("font-weight: 600; color: %1;").arg(Theme::TextSecondary));
    m_gradientPresetCombo = new QComboBox(gradCard);
    m_gradientPresetCombo->addItem("Gradient Presets...");
    m_gradientPresetCombo->addItem("Sunset (Orange -> Pink)");
    m_gradientPresetCombo->addItem("Ocean (Deep Blue -> Cyan)");
    m_gradientPresetCombo->addItem("Forest (Emerald -> Mint)");
    m_gradientPresetCombo->addItem("Sky (Cyan -> Soft Blue)");
    m_gradientPresetCombo->addItem("Warm Spectrum (Red -> Orange)");
    m_gradientPresetCombo->addItem("Cool Spectrum (Indigo -> Azure)");
    m_gradientPresetCombo->addItem("Neon Glow (Cyan -> Purple)");
    m_gradientPresetCombo->addItem("Custom 4-Stop");
    connect(m_gradientPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ColorPickerDialog::onGradientPresetChanged);

    topGradRow->addWidget(typeLbl);
    topGradRow->addWidget(m_gradientTypeCombo, 1);
    topGradRow->addWidget(presetLbl);
    topGradRow->addWidget(m_gradientPresetCombo, 1);
    gcLayout->addLayout(topGradRow);

    // Large Live Preview
    m_gradientPreview = new GradientPreviewWidget(gradCard);
    gcLayout->addWidget(m_gradientPreview);

    // Draggable Stop Bar
    m_gradientStopBar = new GradientStopBarWidget(gradCard);
    connect(m_gradientStopBar, &GradientStopBarWidget::stopSelected, this, &ColorPickerDialog::onStopBarSelected);
    connect(m_gradientStopBar, &GradientStopBarWidget::stopMoved, this, &ColorPickerDialog::onStopBarMoved);
    connect(m_gradientStopBar, &GradientStopBarWidget::stopAddRequested, this, [this](qreal pos) {
        QColor interp = m_gradient.interpolateAt(pos);
        m_gradient.addStop(pos, interp);
        refreshGradientWidgets();
    });
    gcLayout->addWidget(m_gradientStopBar);

    // Selected Stop Actions & Direction Controls
    QHBoxLayout* midGradRow = new QHBoxLayout();
    midGradRow->setSpacing(12);

    // Left Column: Selected Stop Editor & Wheel
    QVBoxLayout* stopEditCol = new QVBoxLayout();
    stopEditCol->setSpacing(6);

    QHBoxLayout* stopBtnsRow = new QHBoxLayout();
    QPushButton* btnAddStop = new QPushButton("+ Add Color Stop", gradCard);
    btnAddStop->setStyleSheet(QString("background: %1; color: %2; border-radius: %3px; padding: 4px 8px; font-weight: 600; font-size: 11px;").arg(Theme::Accent, Theme::BgBase).arg(Theme::RadiusSm));
    connect(btnAddStop, &QPushButton::clicked, this, &ColorPickerDialog::onAddStopClicked);

    m_btnRemoveStop = new QPushButton("- Remove Stop", gradCard);
    m_btnRemoveStop->setStyleSheet(QString("background: %1; color: %2; border: 1px solid %3; border-radius: %4px; padding: 4px 8px; font-size: 11px;").arg(Theme::DangerBg, Theme::Danger, Theme::BorderSubtle).arg(Theme::RadiusSm));
    connect(m_btnRemoveStop, &QPushButton::clicked, this, &ColorPickerDialog::onRemoveStopClicked);

    stopBtnsRow->addWidget(btnAddStop);
    stopBtnsRow->addWidget(m_btnRemoveStop);
    stopEditCol->addLayout(stopBtnsRow);

    QHBoxLayout* stopParamRow = new QHBoxLayout();
    stopParamRow->addWidget(new QLabel("Position:", gradCard));
    m_stopPosSpin = new QSpinBox(gradCard);
    m_stopPosSpin->setRange(0, 100);
    m_stopPosSpin->setSuffix(" %");
    connect(m_stopPosSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ColorPickerDialog::onStopPosSpinChanged);
    stopParamRow->addWidget(m_stopPosSpin);

    m_stopColorSwatch = new QWidget(gradCard);
    m_stopColorSwatch->setFixedSize(18, 18);
    m_stopColorSwatch->setStyleSheet(QString("background-color: %1; border-radius: 3px; border: 1px solid rgba(255,255,255,0.3);").arg(Theme::Accent));
    stopParamRow->addWidget(m_stopColorSwatch);

    m_stopHexEdit = new QLineEdit(Theme::Accent, gradCard);
    m_stopHexEdit->setFixedWidth(75);
    connect(m_stopHexEdit, &QLineEdit::textEdited, this, [this](const QString& text) {
        if (QColor::isValidColorName(text)) {
            QColor col(text);
            if (m_gradientStopBar->selectedStopIndex() >= 0 && m_gradientStopBar->selectedStopIndex() < m_gradient.stops.size()) {
                m_gradient.stops[m_gradientStopBar->selectedStopIndex()].color = col;
                m_gradientWheelWidget->setColor(col);
                refreshGradientWidgets();
            }
        }
    });
    stopParamRow->addWidget(m_stopHexEdit);
    stopEditCol->addLayout(stopParamRow);

    // Linear vs Radial parameter controls
    m_linearControlsWidget = new QWidget(gradCard);
    QVBoxLayout* linearLayout = new QVBoxLayout(m_linearControlsWidget);
    linearLayout->setContentsMargins(0, 0, 0, 0);
    linearLayout->setSpacing(4);

    QHBoxLayout* angleRow = new QHBoxLayout();
    angleRow->addWidget(new QLabel("Angle:", m_linearControlsWidget));
    m_angleSlider = new QSlider(Qt::Horizontal, m_linearControlsWidget);
    m_angleSlider->setRange(0, 360);
    m_angleSlider->setValue(90);
    m_angleSpin = new QSpinBox(m_linearControlsWidget);
    m_angleSpin->setRange(0, 360);
    m_angleSpin->setValue(90);
    m_angleSpin->setSuffix("°");
    connect(m_angleSlider, &QSlider::valueChanged, m_angleSpin, &QSpinBox::setValue);
    connect(m_angleSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ColorPickerDialog::onGradientAngleChanged);
    connect(m_angleSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_angleSlider, &QSlider::setValue);
    angleRow->addWidget(m_angleSlider, 1);
    angleRow->addWidget(m_angleSpin);
    linearLayout->addLayout(angleRow);

    // Preset direction buttons (Section 4)
    QHBoxLayout* dirBtns = new QHBoxLayout();
    dirBtns->setSpacing(3);
    auto makeDirBtn = [this, dirBtns, gradCard](const QString& label, int ang) {
        QPushButton* b = new QPushButton(label, gradCard);
        b->setStyleSheet(QString("padding: 2px 5px; font-size: 10px; background: %1; border: 1px solid %2; border-radius: 3px; color: %3;").arg(Theme::BgBase, Theme::BorderSubtle, Theme::TextSecondary));
        connect(b, &QPushButton::clicked, this, [this, ang]() { onPresetDirectionClicked(ang); });
        dirBtns->addWidget(b);
    };
    makeDirBtn("→ 0°", 0);
    makeDirBtn("↘ 45°", 45);
    makeDirBtn("↓ 90°", 90);
    makeDirBtn("↙ 135°", 135);
    makeDirBtn("← 180°", 180);
    makeDirBtn("↑ 270°", 270);
    linearLayout->addLayout(dirBtns);
    stopEditCol->addWidget(m_linearControlsWidget);

    // Radial controls (Section 5)
    m_radialControlsWidget = new QWidget(gradCard);
    QVBoxLayout* radialLayout = new QVBoxLayout(m_radialControlsWidget);
    radialLayout->setContentsMargins(0, 0, 0, 0);
    radialLayout->setSpacing(4);

    QHBoxLayout* radRow1 = new QHBoxLayout();
    radRow1->addWidget(new QLabel("Center X:", m_radialControlsWidget));
    m_radialCenterXSlider = new QSlider(Qt::Horizontal, m_radialControlsWidget);
    m_radialCenterXSlider->setRange(0, 100);
    m_radialCenterXSlider->setValue(50);
    connect(m_radialCenterXSlider, &QSlider::valueChanged, this, &ColorPickerDialog::onRadialCenterChanged);
    radRow1->addWidget(m_radialCenterXSlider, 1);

    radRow1->addWidget(new QLabel("Y:", m_radialControlsWidget));
    m_radialCenterYSlider = new QSlider(Qt::Horizontal, m_radialControlsWidget);
    m_radialCenterYSlider->setRange(0, 100);
    m_radialCenterYSlider->setValue(50);
    connect(m_radialCenterYSlider, &QSlider::valueChanged, this, &ColorPickerDialog::onRadialCenterChanged);
    radRow1->addWidget(m_radialCenterYSlider, 1);
    radialLayout->addLayout(radRow1);

    QHBoxLayout* radRow2 = new QHBoxLayout();
    radRow2->addWidget(new QLabel("Radius:", m_radialControlsWidget));
    m_radialRadiusSlider = new QSlider(Qt::Horizontal, m_radialControlsWidget);
    m_radialRadiusSlider->setRange(10, 100);
    m_radialRadiusSlider->setValue(50);
    connect(m_radialRadiusSlider, &QSlider::valueChanged, this, &ColorPickerDialog::onRadialCenterChanged);
    radRow2->addWidget(m_radialRadiusSlider, 1);
    radialLayout->addLayout(radRow2);

    stopEditCol->addWidget(m_radialControlsWidget);
    m_radialControlsWidget->setVisible(false);

    midGradRow->addLayout(stopEditCol, 1);

    // Right Column: Color Wheel for Selected Stop
    m_gradientWheelWidget = new ColorWheelWidget(gradCard);
    connect(m_gradientWheelWidget, &ColorWheelWidget::colorChanged, this, [this](const QColor& c) {
        int idx = m_gradientStopBar->selectedStopIndex();
        if (idx >= 0 && idx < m_gradient.stops.size()) {
            m_gradient.stops[idx].color = c;
            m_stopColorSwatch->setStyleSheet(QString("background-color: %1; border-radius: 3px; border: 1px solid rgba(255,255,255,0.3);").arg(c.name()));
            m_stopHexEdit->setText(c.name().toUpper());
            m_gradientPreview->setGradient(m_gradient);
            m_gradientStopBar->setGradient(m_gradient);
        }
    });
    midGradRow->addWidget(m_gradientWheelWidget);

    gcLayout->addLayout(midGradRow);

    gradLayout->addWidget(gradCard);
    m_modeStack->addWidget(m_gradientContainer);

    rootLayout->addWidget(m_modeStack);

    // Initialize in Solid mode
    setGradientMode(false);
}

void ColorPickerDialog::onGradientTypeChanged(int index) {
    m_gradient.type = (index == 1) ? ComponentGradient::Type::Radial : ComponentGradient::Type::Linear;
    m_linearControlsWidget->setVisible(m_gradient.type == ComponentGradient::Type::Linear);
    m_radialControlsWidget->setVisible(m_gradient.type == ComponentGradient::Type::Radial);
    refreshGradientWidgets();
}

void ColorPickerDialog::onGradientAngleChanged(int angle) {
    m_gradient.angle = angle;
    refreshGradientWidgets();
}

void ColorPickerDialog::onPresetDirectionClicked(int angle) {
    m_angleSpin->setValue(angle);
}

void ColorPickerDialog::onRadialCenterChanged() {
    m_gradient.centerX = m_radialCenterXSlider->value() / 100.0;
    m_gradient.centerY = m_radialCenterYSlider->value() / 100.0;
    m_gradient.radius = m_radialRadiusSlider->value() / 100.0;
    refreshGradientWidgets();
}

void ColorPickerDialog::onStopBarSelected(int index) {
    if (index >= 0 && index < m_gradient.stops.size()) {
        const auto& s = m_gradient.stops[index];
        m_stopPosSpin->blockSignals(true);
        m_stopPosSpin->setValue(static_cast<int>(std::round(s.position * 100.0)));
        m_stopPosSpin->blockSignals(false);

        m_stopColorSwatch->setStyleSheet(QString("background-color: %1; border-radius: 3px; border: 1px solid rgba(255,255,255,0.3);").arg(s.color.name()));
        m_stopHexEdit->setText(s.color.name().toUpper());
        m_gradientWheelWidget->setColor(s.color);
        m_btnRemoveStop->setEnabled(m_gradient.stops.size() > 2);
    }
}

void ColorPickerDialog::onStopBarMoved(int index, qreal pos) {
    if (index >= 0 && index < m_gradient.stops.size()) {
        m_stopPosSpin->blockSignals(true);
        m_stopPosSpin->setValue(static_cast<int>(std::round(pos * 100.0)));
        m_stopPosSpin->blockSignals(false);
        m_gradientPreview->setGradient(m_gradient);
    }
}

void ColorPickerDialog::onAddStopClicked() {
    // Insert at midpoint of largest interval
    if (m_gradient.stops.size() < 2) {
        m_gradient.addStop(0.5, Theme::cAccent());
    } else {
        qreal maxGap = 0.0;
        qreal insertPos = 0.5;
        for (int i = 0; i < m_gradient.stops.size() - 1; ++i) {
            qreal gap = m_gradient.stops[i + 1].position - m_gradient.stops[i].position;
            if (gap > maxGap) {
                maxGap = gap;
                insertPos = (m_gradient.stops[i].position + m_gradient.stops[i + 1].position) / 2.0;
            }
        }
        QColor interp = m_gradient.interpolateAt(insertPos);
        m_gradient.addStop(insertPos, interp);
    }
    refreshGradientWidgets();
}

void ColorPickerDialog::onRemoveStopClicked() {
    if (m_gradient.stops.size() > 2) {
        int idx = m_gradientStopBar->selectedStopIndex();
        m_gradient.removeStop(idx);
        m_gradientStopBar->setSelectedStopIndex(std::max(0, idx - 1));
        refreshGradientWidgets();
    }
}

void ColorPickerDialog::onStopPosSpinChanged(int posPercent) {
    int idx = m_gradientStopBar->selectedStopIndex();
    if (idx >= 0 && idx < m_gradient.stops.size()) {
        m_gradient.stops[idx].position = posPercent / 100.0;
        refreshGradientWidgets();
    }
}

void ColorPickerDialog::onGradientPresetChanged(int index) {
    if (index == 1) m_gradient = ComponentGradient::presetSunset();
    else if (index == 2) m_gradient = ComponentGradient::presetOcean();
    else if (index == 3) m_gradient = ComponentGradient::presetForest();
    else if (index == 4) m_gradient = ComponentGradient::presetSky();
    else if (index == 5) m_gradient = ComponentGradient::presetWarmSpectrum();
    else if (index == 6) m_gradient = ComponentGradient::presetCoolSpectrum();
    else if (index == 7) m_gradient = ComponentGradient::presetNeonGlow();
    else if (index == 8) m_gradient = ComponentGradient::presetCustom();

    if (index > 0) {
        updateGradientUi();
    }
}

void ColorPickerDialog::updateGradientUi() {
    m_gradientTypeCombo->blockSignals(true);
    m_gradientTypeCombo->setCurrentIndex(m_gradient.type == ComponentGradient::Type::Radial ? 1 : 0);
    m_gradientTypeCombo->blockSignals(false);

    m_linearControlsWidget->setVisible(m_gradient.type == ComponentGradient::Type::Linear);
    m_radialControlsWidget->setVisible(m_gradient.type == ComponentGradient::Type::Radial);

    m_angleSpin->blockSignals(true);
    m_angleSpin->setValue(static_cast<int>(std::round(m_gradient.angle)));
    m_angleSpin->blockSignals(false);

    m_angleSlider->blockSignals(true);
    m_angleSlider->setValue(static_cast<int>(std::round(m_gradient.angle)));
    m_angleSlider->blockSignals(false);

    m_radialCenterXSlider->setValue(static_cast<int>(std::round(m_gradient.centerX * 100.0)));
    m_radialCenterYSlider->setValue(static_cast<int>(std::round(m_gradient.centerY * 100.0)));
    m_radialRadiusSlider->setValue(static_cast<int>(std::round(m_gradient.radius * 100.0)));

    refreshGradientWidgets();
}

void ColorPickerDialog::refreshGradientWidgets() {
    m_gradientPreview->setGradient(m_gradient);
    m_gradientStopBar->setGradient(m_gradient);
    onStopBarSelected(m_gradientStopBar->selectedStopIndex());
}

void ColorPickerDialog::onWheelColorChanged(const QColor& color) {
    m_baseColor = color;
    m_selectedColor = color;
    m_circleSwatch->setStyleSheet(QString("background-color: %1; border-radius: 12px; border: 1px solid rgba(255,255,255,0.25);").arg(m_baseColor.name()));
    updateHexAndRgb565();
    updateHarmonySwatches();
}

void ColorPickerDialog::onHexEdited(const QString& text) {
    if (QColor::isValidColorName(text)) {
        QColor c(text);
        m_baseColor = c;
        m_selectedColor = c;
        m_wheelWidget->setColor(c);
        m_circleSwatch->setStyleSheet(QString("background-color: %1; border-radius: 12px; border: 1px solid rgba(255,255,255,0.25);").arg(m_baseColor.name()));
        m_rgb565Label->setText(QString("RGB565: %1").arg(toRgb565Hex(m_baseColor)));
        updateHarmonySwatches();
    }
}

void ColorPickerDialog::onHarmonyModeChanged(int index) {
    m_harmonyMode = index;
    updateHarmonySwatches();
}

void ColorPickerDialog::updateHexAndRgb565() {
    m_hexEdit->setText(m_baseColor.name().toUpper());
    m_rgb565Label->setText(QString("RGB565: %1").arg(toRgb565Hex(m_baseColor)));
}

void ColorPickerDialog::selectSwatch(int index) {
    if (index >= 0 && index < static_cast<int>(m_swatches.size())) {
        m_activeSwatchIndex = index;
        m_selectedColor = m_swatches[index].color;

        for (size_t i = 0; i < m_swatches.size(); ++i) {
            bool sel = (static_cast<int>(i) == index);
            m_swatches[i].swatchBtn->setStyleSheet(
                QString("background-color: %1; border: %2 solid %3; border-radius: 3px;")
                .arg(m_swatches[i].color.name())
                .arg(sel ? "2px" : "1px")
                .arg(sel ? Theme::TextPrimary : "rgba(0,0,0,0.4)")
            );
            m_swatches[i].pill->setStyleSheet(
                sel ? QString("color: %1; background-color: %2; border: 1px solid %3; border-radius: 3px; font-weight: 600; font-family: monospace; font-size: 9.5px; padding: 2px 4px;").arg(Theme::AccentText, Theme::AccentSoft, Theme::Accent)
                    : QString("color: %1; background-color: %2; border: 1px solid %3; border-radius: 3px; font-family: monospace; font-size: 9.5px; padding: 2px 4px;").arg(Theme::TextMuted, Theme::BgBase, Theme::BorderSubtle)
            );
        }
    }
}

void ColorPickerDialog::updateHarmonySwatches() {
    // Clear previous layouts
    QLayoutItem* child;
    while ((child = m_swatchesBarLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    while ((child = m_swatchesHexLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    m_swatches.clear();

    float h, s, v;
    m_baseColor.getHsvF(&h, &s, &v);
    if (h < 0) h = 0.0;

    std::vector<std::pair<QColor, QString>> swData;
    swData.push_back({m_baseColor, "Base"});

    auto wrapHue = [](float val) {
        while (val < 0.0f) val += 1.0f;
        while (val >= 1.0f) val -= 1.0f;
        return val;
    };

    switch (m_harmonyMode) {
        case 0: // Complementary
        default: {
            QColor comp = QColor::fromHsvF(wrapHue(h + 0.5f), s, v);
            swData.push_back({comp, "Harmony"});
            m_wheelWidget->setHarmonyColor(comp);
            break;
        }
        case 1: // Monochromatic
            swData.push_back({QColor::fromHsvF(h, s * 0.7f, std::min(1.0f, v * 1.15f)), "Shade 1"});
            swData.push_back({QColor::fromHsvF(h, s, v * 0.7f), "Shade 2"});
            swData.push_back({QColor::fromHsvF(h, s * 0.4f, std::min(1.0f, v * 1.3f)), "Shade 3"});
            m_wheelWidget->setHarmonyColor(swData[1].first);
            break;
        case 2: // Analogous
            swData.push_back({QColor::fromHsvF(wrapHue(h + 30.0f / 360.0f), s, v), "+30°"});
            swData.push_back({QColor::fromHsvF(wrapHue(h - 30.0f / 360.0f), s, v), "-30°"});
            m_wheelWidget->setHarmonyColor(swData[1].first);
            break;
        case 3: // Triadic
            swData.push_back({QColor::fromHsvF(wrapHue(h + 120.0f / 360.0f), s, v), "+120°"});
            swData.push_back({QColor::fromHsvF(wrapHue(h + 240.0f / 360.0f), s, v), "+240°"});
            m_wheelWidget->setHarmonyColor(swData[1].first);
            break;
        case 4: // Tetradic
            swData.push_back({QColor::fromHsvF(wrapHue(h + 90.0f / 360.0f), s, v), "+90°"});
            swData.push_back({QColor::fromHsvF(wrapHue(h + 180.0f / 360.0f), s, v), "+180°"});
            swData.push_back({QColor::fromHsvF(wrapHue(h + 270.0f / 360.0f), s, v), "+270°"});
            m_wheelWidget->setHarmonyColor(swData[1].first);
            break;
    }

    for (size_t i = 0; i < swData.size(); ++i) {
        SwatchData sd;
        sd.color = swData[i].first;
        sd.name = swData[i].second;
        sd.hex = sd.color.name().toUpper();

        sd.swatchBtn = new QPushButton();
        sd.swatchBtn->setFixedHeight(22);
        sd.swatchBtn->setCursor(Qt::PointingHandCursor);
        connect(sd.swatchBtn, &QPushButton::clicked, this, [this, i]() { selectSwatch(static_cast<int>(i)); });

        sd.pill = new QLabel(QString("%1: %2").arg(sd.name, sd.hex));
        sd.pill->setAlignment(Qt::AlignCenter);

        m_swatchesBarLayout->addWidget(sd.swatchBtn, 1);
        m_swatchesHexLayout->addWidget(sd.pill, 1);

        m_swatches.push_back(sd);
    }

    selectSwatch(0);
}
