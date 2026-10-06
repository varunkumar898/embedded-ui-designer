#include "PinoutViewWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QFontMetrics>
#include <cmath>

namespace Hardware {

PinoutViewWidget::PinoutViewWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(480, 400);
}

void PinoutViewWidget::setEngine(PinMuxEngine* engine) {
    m_engine = engine;
    if (m_engine) {
        connect(m_engine, &PinMuxEngine::configurationChanged, this, [this]() {
            update();
        });
    }
    update();
}

void PinoutViewWidget::setSelectedPin(const QString& pinName) {
    if (m_selectedPin != pinName) {
        m_selectedPin = pinName;
        update();
    }
}

QColor PinoutViewWidget::pinColor(const QString& pinName) const {
    if (!m_engine) return QColor("#334155");

    DeviceDefinition dev = m_engine->currentDevice();
    const PinDefinition* pdef = dev.findPin(pinName);
    if (pdef) {
        if (pdef->isPower) return QColor("#b91c1c"); // Red
        if (pdef->isGround) return QColor("#475569"); // Slate
        if (pdef->isReset) return QColor("#ea580c"); // Orange
        if (pdef->isReserved) return QColor("#7c3aed"); // Purple-gray / debug
    }

    QString owner = m_engine->getPinOwner(pinName);
    if (owner.isEmpty()) {
        return QColor("#3f3f46"); // Unconfigured
    }

    if (owner.contains("SPI", Qt::CaseInsensitive) || owner.contains("I2C", Qt::CaseInsensitive) ||
        owner.contains("UART", Qt::CaseInsensitive) || owner.contains("CAN", Qt::CaseInsensitive) ||
        owner.contains("USB", Qt::CaseInsensitive)) {
        return QColor("#0284c7"); // Peripheral AF - Vivid Blue
    }
    if (owner.contains("ADC", Qt::CaseInsensitive) || owner.contains("Analog", Qt::CaseInsensitive)) {
        return QColor("#d97706"); // Analog - Amber
    }
    if (owner.contains("PWM", Qt::CaseInsensitive) || owner.contains("Timer", Qt::CaseInsensitive)) {
        return QColor("#0d9488"); // Teal
    }
    if (owner.contains("GPIO", Qt::CaseInsensitive)) {
        return QColor("#16a34a"); // GPIO - Green
    }

    return QColor("#0284c7");
}

void PinoutViewWidget::computeLayout() {
    m_pinRects.clear();
    if (!m_engine) return;

    DeviceDefinition dev = m_engine->currentDevice();
    const auto& pins = dev.pins;
    if (pins.isEmpty()) return;

    int totalPins = pins.size();
    QRectF bounds = rect().adjusted(20, 20, -20, -20);

    // If package contains "LQFP" or "QFP" or "QFN" and pin count >= 32, use 4-sided package layout
    bool fourSided = (dev.package.contains("QFP", Qt::CaseInsensitive) || dev.package.contains("QFN", Qt::CaseInsensitive)) && totalPins >= 24;

    if (fourSided) {
        int sideCount = (totalPins + 3) / 4;
        qreal chipMargin = 120;
        QRectF chipRect(bounds.left() + chipMargin, bounds.top() + chipMargin, bounds.width() - 2 * chipMargin, bounds.height() - 2 * chipMargin);

        qreal pinLength = 32.0;
        qreal pinWidth = 14.0;

        for (int i = 0; i < totalPins; ++i) {
            PinRect pr;
            pr.pinName = pins[i].name;
            pr.pinNumber = pins[i].physicalPin > 0 ? pins[i].physicalPin : (i + 1);

            int side = i / sideCount; // 0: Left, 1: Bottom, 2: Right, 3: Top
            int idxInSide = i % sideCount;
            qreal stepY = (chipRect.height() - 20) / std::max(1, sideCount - 1);
            qreal stepX = (chipRect.width() - 20) / std::max(1, sideCount - 1);

            if (side == 0) { // Left
                qreal y = chipRect.top() + 10 + idxInSide * stepY;
                pr.rect = QRectF(chipRect.left() - pinLength, y - pinWidth / 2, pinLength, pinWidth);
            } else if (side == 1) { // Bottom
                qreal x = chipRect.left() + 10 + idxInSide * stepX;
                pr.rect = QRectF(x - pinWidth / 2, chipRect.bottom(), pinWidth, pinLength);
            } else if (side == 2) { // Right
                qreal y = chipRect.bottom() - 10 - idxInSide * stepY;
                pr.rect = QRectF(chipRect.right(), y - pinWidth / 2, pinLength, pinWidth);
            } else { // Top
                qreal x = chipRect.right() - 10 - idxInSide * stepX;
                pr.rect = QRectF(x - pinWidth / 2, chipRect.top() - pinLength, pinWidth, pinLength);
            }
            m_pinRects.append(pr);
        }
    } else {
        // Dual column header layout (e.g. DIP / DevKit / Headers)
        int half = (totalPins + 1) / 2;
        qreal chipWidth = bounds.width() * 0.45;
        qreal chipLeft = bounds.left() + (bounds.width() - chipWidth) / 2.0;
        QRectF chipRect(chipLeft, bounds.top() + 30, chipWidth, bounds.height() - 60);

        qreal pinH = std::min(22.0, (chipRect.height() - 20) / std::max(1, half));
        qreal pinW = 90.0;

        for (int i = 0; i < totalPins; ++i) {
            PinRect pr;
            pr.pinName = pins[i].name;
            pr.pinNumber = pins[i].physicalPin > 0 ? pins[i].physicalPin : (i + 1);

            if (i < half) {
                // Left side
                qreal y = chipRect.top() + 10 + i * pinH;
                pr.rect = QRectF(chipRect.left() - pinW - 6, y, pinW, pinH - 2);
            } else {
                // Right side
                int rIdx = i - half;
                qreal y = chipRect.top() + 10 + rIdx * pinH;
                pr.rect = QRectF(chipRect.right() + 6, y, pinW, pinH - 2);
            }
            m_pinRects.append(pr);
        }
    }
}

void PinoutViewWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor("#141418"));

    if (!m_engine || m_engine->currentDevice().partNumber.isEmpty()) {
        painter.setPen(QColor("#71717a"));
        painter.drawText(rect(), Qt::AlignCenter, "No hardware target selected.");
        return;
    }

    computeLayout();

    DeviceDefinition dev = m_engine->currentDevice();
    QRectF bounds = rect().adjusted(20, 20, -20, -20);
    bool fourSided = (dev.package.contains("QFP", Qt::CaseInsensitive) || dev.package.contains("QFN", Qt::CaseInsensitive)) && dev.pins.size() >= 24;

    // Draw central chip body
    QRectF chipRect;
    if (fourSided) {
        qreal chipMargin = 120;
        chipRect = QRectF(bounds.left() + chipMargin, bounds.top() + chipMargin, bounds.width() - 2 * chipMargin, bounds.height() - 2 * chipMargin);
    } else {
        qreal chipWidth = bounds.width() * 0.45;
        qreal chipLeft = bounds.left() + (bounds.width() - chipWidth) / 2.0;
        chipRect = QRectF(chipLeft, bounds.top() + 30, chipWidth, bounds.height() - 60);
    }

    // Chip shadow & body
    painter.setPen(QPen(QColor("#3f3f46"), 1.5));
    painter.setBrush(QColor("#202026"));
    painter.drawRoundedRect(chipRect, 6, 6);

    // Pin 1 orientation index (small notch / dot)
    painter.setBrush(QColor("#52525b"));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(chipRect.left() + 16, chipRect.top() + 16), 5, 5);

    // Chip labels
    painter.setPen(QColor("#f4f4f5"));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    painter.setFont(titleFont);
    painter.drawText(QRectF(chipRect.left() + 10, chipRect.top() + chipRect.height() * 0.3, chipRect.width() - 20, 24), Qt::AlignCenter, dev.partNumber);

    QFont subFont = font();
    subFont.setPointSize(9);
    painter.setFont(subFont);
    painter.setPen(QColor("#a1a1aa"));
    painter.drawText(QRectF(chipRect.left() + 10, chipRect.top() + chipRect.height() * 0.3 + 24, chipRect.width() - 20, 20), Qt::AlignCenter, dev.vendor);
    painter.drawText(QRectF(chipRect.left() + 10, chipRect.top() + chipRect.height() * 0.3 + 44, chipRect.width() - 20, 20), Qt::AlignCenter, QString("%1 (%2)").arg(dev.core, dev.package));

    // Draw pins
    QFont pinFont = font();
    pinFont.setPointSize(8);
    painter.setFont(pinFont);

    for (const auto& pr : m_pinRects) {
        QColor fill = pinColor(pr.pinName);
        bool isSel = (pr.pinName == m_selectedPin);
        bool isHover = (pr.pinName == m_hoverPin);

        if (isSel) {
            painter.setPen(QPen(QColor("#38bdf8"), 2.0));
        } else if (isHover) {
            painter.setPen(QPen(QColor("#93c5fd"), 1.5));
        } else {
            painter.setPen(QPen(QColor("#27272a"), 1.0));
        }

        painter.setBrush(fill);
        painter.drawRoundedRect(pr.rect, 2, 2);

        // Pin text label
        painter.setPen(QColor("#ffffff"));
        QString owner = m_engine ? m_engine->getPinOwner(pr.pinName) : QString();
        QString text = pr.pinName;
        if (!owner.isEmpty() && !fourSided) {
            text += QString(" [%1]").arg(owner);
        }

        painter.drawText(pr.rect, Qt::AlignCenter, text);
    }
}

void PinoutViewWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        for (const auto& pr : m_pinRects) {
            if (pr.rect.contains(event->position())) {
                m_selectedPin = pr.pinName;
                emit pinClicked(pr.pinName);
                update();
                return;
            }
        }
    }
    QWidget::mousePressEvent(event);
}

void PinoutViewWidget::mouseMoveEvent(QMouseEvent* event) {
    QString oldHover = m_hoverPin;
    m_hoverPin.clear();
    for (const auto& pr : m_pinRects) {
        if (pr.rect.contains(event->position())) {
            m_hoverPin = pr.pinName;
            break;
        }
    }
    if (oldHover != m_hoverPin) {
        update();
    }
    QWidget::mouseMoveEvent(event);
}

} // namespace Hardware
