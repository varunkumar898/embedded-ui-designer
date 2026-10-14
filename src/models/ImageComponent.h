#pragma once

#include "UIComponent.h"
#include <QImage>
#include <QRectF>

class ImageComponent : public UIComponent {
    Q_OBJECT

public:
    explicit ImageComponent(const QString& id = "image", QGraphicsItem* parent = nullptr);

    QString imagePath() const { return m_imagePath; }
    void setImagePath(const QString& path);

    QString format() const { return m_format; }
    void setFormat(const QString& format); // "RGB565" or "Monochrome"

    int opacityPercent() const { return m_opacityPercent; }
    void setOpacityPercent(int op);

    QString scalingMode() const { return m_scalingMode; }
    void setScalingMode(const QString& mode);

    // ── Crop ──────────────────────────────────────────────────────────
    QRectF cropRect() const { return m_cropRect; } // Normalized 0..1
    void setCropRect(const QRectF& rect);

    // ── Adjustments ───────────────────────────────────────────────────
    int brightness() const { return m_brightness; }
    void setBrightness(int b);

    int contrast() const { return m_contrast; }
    void setContrast(int c);

    int saturation() const { return m_saturation; }
    void setSaturation(int s);

    bool grayscale() const { return m_grayscale; }
    void setGrayscale(bool g);

    QImage loadedImage() const;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    void applyAdjustments(QImage& img) const;

    QString m_imagePath;
    QString m_format = "RGB565";
    int m_opacityPercent = 100;
    QString m_scalingMode = "KeepAspectRatio";
    QRectF m_cropRect = QRectF(0, 0, 1.0, 1.0);
    int m_brightness = 0;
    int m_contrast = 0;
    int m_saturation = 0;
    bool m_grayscale = false;

    mutable QImage m_cachedImage;
    mutable bool m_cacheValid = false;
};
