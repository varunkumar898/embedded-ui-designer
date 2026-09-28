#pragma once

#include "UIComponent.h"
#include <QImage>

class ImageComponent : public UIComponent {
    Q_OBJECT

public:
    explicit ImageComponent(const QString& id = "image", QGraphicsItem* parent = nullptr);

    QString imagePath() const { return m_imagePath; }
    void setImagePath(const QString& path);

    QString format() const { return m_format; }
    void setFormat(const QString& format); // "RGB565" or "Monochrome"

    QImage loadedImage() const;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    QString toQmlSnippet(int indentSpaces = 8) const override;
    QString toUgfxSnippet(int indentSpaces = 4) const override;

protected:
    void paintComponent(QPainter* painter) override;

private:
    QString m_imagePath;
    QString m_format = "RGB565";
    mutable QImage m_cachedImage;
    mutable bool m_cacheValid = false;
};
