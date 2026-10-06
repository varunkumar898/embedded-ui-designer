#include "ImageComponent.h"
#include <QPen>
#include <QBrush>
#include <QFileInfo>
#include <QPainterPath>
#include <algorithm>

ImageComponent::ImageComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "Image", parent)
{
    m_width = 100;
    m_height = 80;
}

void ImageComponent::setImagePath(const QString& path) {
    if (m_imagePath != path) {
        m_imagePath = path;
        m_cacheValid = false;
        update();
        emit propertyChanged(this);
    }
}

void ImageComponent::setFormat(const QString& format) {
    if (m_format != format) {
        m_format = format;
        update();
        emit propertyChanged(this);
    }
}

void ImageComponent::setOpacityPercent(int op) {
    op = std::clamp(op, 0, 100);
    if (m_opacityPercent != op) {
        m_opacityPercent = op;
        update();
        emit propertyChanged(this);
    }
}

void ImageComponent::setScalingMode(const QString& mode) {
    if (m_scalingMode != mode) {
        m_scalingMode = mode;
        update();
        emit propertyChanged(this);
    }
}

QImage ImageComponent::loadedImage() const {
    if (!m_cacheValid) {
        if (!m_imagePath.isEmpty() && QFileInfo::exists(m_imagePath)) {
            m_cachedImage.load(m_imagePath);
        } else {
            // Generate a clean placeholder pattern
            m_cachedImage = QImage(static_cast<int>(m_width), static_cast<int>(m_height), QImage::Format_ARGB32_Premultiplied);
            m_cachedImage.fill(QColor(28, 32, 40));
            QPainter p(&m_cachedImage);
            p.setPen(QPen(QColor(16, 185, 129, 140), 1.5));
            p.drawRect(0, 0, m_cachedImage.width() - 1, m_cachedImage.height() - 1);
            p.drawLine(0, 0, m_cachedImage.width(), m_cachedImage.height());
            p.drawLine(0, m_cachedImage.height(), m_cachedImage.width(), 0);
        }
        m_cacheValid = true;
    }
    return m_cachedImage;
}

void ImageComponent::paintComponent(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    qreal opacity = m_opacityPercent / 100.0;
    painter->setOpacity(opacity);

    QImage img = loadedImage();
    if (!img.isNull()) {
        QRectF targetRect(0, 0, m_width, m_height);
        if (m_scalingMode == "Stretch") {
            painter->drawImage(targetRect, img);
        } else if (m_scalingMode == "Center") {
            qreal ix = (m_width - img.width()) / 2.0;
            qreal iy = (m_height - img.height()) / 2.0;
            painter->drawImage(QPointF(ix, iy), img);
        } else { // KeepAspectRatio
            QSizeF scaledSize = QSizeF(img.size()).scaled(m_width, m_height, Qt::KeepAspectRatio);
            qreal ix = (m_width - scaledSize.width()) / 2.0;
            qreal iy = (m_height - scaledSize.height()) / 2.0;
            painter->drawImage(QRectF(ix, iy, scaledSize.width(), scaledSize.height()), img);
        }
    }

    // Border outline for designer visualization
    painter->setOpacity(1.0);
    painter->setPen(QPen(QColor(56, 189, 248, 120), 1.0, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(0, 0, m_width, m_height));
}

QJsonObject ImageComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["imagePath"] = m_imagePath;
    obj["format"] = m_format;
    obj["opacity"] = m_opacityPercent;
    obj["scalingMode"] = m_scalingMode;
    return obj;
}

void ImageComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_imagePath = json.value("imagePath").toString(m_imagePath);
    m_format = json.value("format").toString("RGB565");
    m_opacityPercent = json.value("opacity").toInt(100);
    m_scalingMode = json.value("scalingMode").toString("KeepAspectRatio");
    m_cacheValid = false;
    update();
}

QString ImageComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1Image {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    x: %2\n").arg(indent).arg(static_cast<int>(pos().x()));
    qml += QString("%1    y: %2\n").arg(indent).arg(static_cast<int>(pos().y()));
    qml += QString("%1    width: %2\n").arg(indent).arg(static_cast<int>(m_width));
    qml += QString("%1    height: %2\n").arg(indent).arg(static_cast<int>(m_height));
    qml += QString("%1    source: \"%2\"\n").arg(indent, m_imagePath);
    if (m_opacityPercent < 100) {
        qml += QString("%1    opacity: %2\n").arg(indent).arg(m_opacityPercent / 100.0, 0, 'f', 2);
    }
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString ImageComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Image: %2\n").arg(indent, m_id);
    code += QString("%1gdispDrawBitmap(%2, %3, %4_width, %4_height, (const gdisp_pixel_t*)%4);\n")
        .arg(indent)
        .arg(static_cast<int>(pos().x()))
        .arg(static_cast<int>(pos().y()))
        .arg(m_id + "_data");
    return code;
}
