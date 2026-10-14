#include "ImageComponent.h"
#include <QPen>
#include <QBrush>
#include <QFileInfo>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

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

void ImageComponent::setCropRect(const QRectF& rect) {
    QRectF normRect = rect.normalized();
    normRect.setLeft(std::clamp(normRect.left(), 0.0, 1.0));
    normRect.setTop(std::clamp(normRect.top(), 0.0, 1.0));
    normRect.setRight(std::clamp(normRect.right(), 0.0, 1.0));
    normRect.setBottom(std::clamp(normRect.bottom(), 0.0, 1.0));
    if (m_cropRect != normRect) {
        m_cropRect = normRect;
        m_cacheValid = false;
        update();
        emit propertyChanged(this);
    }
}

void ImageComponent::setBrightness(int b) {
    b = std::clamp(b, -100, 100);
    if (m_brightness != b) {
        m_brightness = b;
        m_cacheValid = false;
        update();
        emit propertyChanged(this);
    }
}

void ImageComponent::setContrast(int c) {
    c = std::clamp(c, -100, 100);
    if (m_contrast != c) {
        m_contrast = c;
        m_cacheValid = false;
        update();
        emit propertyChanged(this);
    }
}

void ImageComponent::setSaturation(int s) {
    s = std::clamp(s, -100, 100);
    if (m_saturation != s) {
        m_saturation = s;
        m_cacheValid = false;
        update();
        emit propertyChanged(this);
    }
}

void ImageComponent::setGrayscale(bool g) {
    if (m_grayscale != g) {
        m_grayscale = g;
        m_cacheValid = false;
        update();
        emit propertyChanged(this);
    }
}

void ImageComponent::applyAdjustments(QImage& img) const {
    if (img.isNull()) return;

    if (m_grayscale || m_brightness != 0 || m_contrast != 0) {
        img = img.convertToFormat(QImage::Format_ARGB32);
        const double contrastFactor = (259.0 * (m_contrast + 255.0)) / (255.0 * (259.0 - m_contrast));
        const int bright = (m_brightness * 255) / 100;

        for (int y = 0; y < img.height(); ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < img.width(); ++x) {
                QRgb p = line[x];
                int a = qAlpha(p);
                int r = qRed(p);
                int g = qGreen(p);
                int b = qBlue(p);

                if (m_grayscale) {
                    int gray = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);
                    r = g = b = gray;
                }

                if (m_brightness != 0) {
                    r = std::clamp(r + bright, 0, 255);
                    g = std::clamp(g + bright, 0, 255);
                    b = std::clamp(b + bright, 0, 255);
                }

                if (m_contrast != 0) {
                    r = std::clamp(static_cast<int>(contrastFactor * (r - 128) + 128), 0, 255);
                    g = std::clamp(static_cast<int>(contrastFactor * (g - 128) + 128), 0, 255);
                    b = std::clamp(static_cast<int>(contrastFactor * (b - 128) + 128), 0, 255);
                }

                line[x] = qRgba(r, g, b, a);
            }
        }
    }
}

QImage ImageComponent::loadedImage() const {
    if (!m_cacheValid) {
        if (!m_imagePath.isEmpty() && QFileInfo::exists(m_imagePath)) {
            m_cachedImage.load(m_imagePath);

            // Apply crop if set
            if (m_cropRect.isValid() && (m_cropRect.width() < 0.999 || m_cropRect.height() < 0.999 || m_cropRect.x() > 0.001 || m_cropRect.y() > 0.001)) {
                int cx = std::clamp(static_cast<int>(m_cropRect.x() * m_cachedImage.width()), 0, std::max(0, m_cachedImage.width() - 1));
                int cy = std::clamp(static_cast<int>(m_cropRect.y() * m_cachedImage.height()), 0, std::max(0, m_cachedImage.height() - 1));
                int cw = std::clamp(static_cast<int>(m_cropRect.width() * m_cachedImage.width()), 1, m_cachedImage.width() - cx);
                int ch = std::clamp(static_cast<int>(m_cropRect.height() * m_cachedImage.height()), 1, m_cachedImage.height() - cy);
                m_cachedImage = m_cachedImage.copy(cx, cy, cw, ch);
            }

            // Apply filters
            applyAdjustments(m_cachedImage);
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
    obj["opacityPercent"] = m_opacityPercent;
    obj["scalingMode"] = m_scalingMode;
    obj["brightness"] = m_brightness;
    obj["contrast"] = m_contrast;
    obj["saturation"] = m_saturation;
    obj["grayscale"] = m_grayscale;

    QJsonObject cropObj;
    cropObj["x"] = m_cropRect.x();
    cropObj["y"] = m_cropRect.y();
    cropObj["width"] = m_cropRect.width();
    cropObj["height"] = m_cropRect.height();
    obj["cropRect"] = cropObj;

    return obj;
}

void ImageComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    m_imagePath = json.value("imagePath").toString(m_imagePath);
    m_format = json.value("format").toString(m_format);
    m_opacityPercent = json.value("opacityPercent").toInt(m_opacityPercent);
    m_scalingMode = json.value("scalingMode").toString(m_scalingMode);
    m_brightness = json.value("brightness").toInt(0);
    m_contrast = json.value("contrast").toInt(0);
    m_saturation = json.value("saturation").toInt(0);
    m_grayscale = json.value("grayscale").toBool(false);

    if (json.contains("cropRect") && json.value("cropRect").isObject()) {
        QJsonObject c = json.value("cropRect").toObject();
        m_cropRect = QRectF(c.value("x").toDouble(0.0),
                            c.value("y").toDouble(0.0),
                            c.value("width").toDouble(1.0),
                            c.value("height").toDouble(1.0));
    } else {
        m_cropRect = QRectF(0, 0, 1.0, 1.0);
    }

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
    qml += QString("%1    opacity: %2\n").arg(indent).arg(m_opacityPercent / 100.0, 0, 'f', 2);
    if (m_scalingMode == "Stretch") {
        qml += QString("%1    fillMode: Image.Stretch\n").arg(indent);
    } else if (m_scalingMode == "Center") {
        qml += QString("%1    fillMode: Image.Pad\n").arg(indent);
    } else {
        qml += QString("%1    fillMode: Image.PreserveAspectFit\n").arg(indent);
    }
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString ImageComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Image: %2\n").arg(indent, m_id);
    code += QString("%1gdispImage img_%2;\n").arg(indent, m_id);
    code += QString("%1gdispImageOpenFile(&img_%2, \"%3\");\n").arg(indent, m_id, m_imagePath);
    code += QString("%1gdispImageDraw(&img_%2, %3, %4, %5, %6, 0, 0);\n")
        .arg(indent, m_id)
        .arg(static_cast<int>(pos().x()))
        .arg(static_cast<int>(pos().y()))
        .arg(static_cast<int>(m_width))
        .arg(static_cast<int>(m_height));
    code += QString("%1gdispImageClose(&img_%2);\n").arg(indent, m_id);
    return code;
}
