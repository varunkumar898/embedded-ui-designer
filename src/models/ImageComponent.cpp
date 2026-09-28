#include "ImageComponent.h"
#include <QPen>
#include <QBrush>
#include <QFileInfo>
#include <QPainterPath>

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

QImage ImageComponent::loadedImage() const {
    if (!m_cacheValid) {
        if (!m_imagePath.isEmpty() && QFileInfo::exists(m_imagePath)) {
            m_cachedImage.load(m_imagePath);
        } else {
            // Generate a placeholder test pattern
            m_cachedImage = QImage(static_cast<int>(m_width), static_cast<int>(m_height), QImage::Format_RGB32);
            m_cachedImage.fill(QColor("#2C313A"));
            QPainter p(&m_cachedImage);
            p.setPen(QPen(QColor("#61AFEF"), 2));
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

    QImage img = loadedImage();
    if (!img.isNull()) {
        painter->drawImage(QRectF(0, 0, m_width, m_height), img);
    }

    // Border outline
    painter->setPen(QPen(QColor("#61AFEF"), 1.0, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(0, 0, m_width, m_height));
}

QJsonObject ImageComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    obj["imagePath"] = m_imagePath;
    obj["format"] = m_format;
    return obj;
}

void ImageComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_imagePath = json.value("imagePath").toString(m_imagePath);
    m_format = json.value("format").toString("RGB565");
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
