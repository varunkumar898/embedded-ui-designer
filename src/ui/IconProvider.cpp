#include "ui/IconProvider.h"
#include "theme/Theme.h"

#include <QFile>
#include <QHash>
#include <QPainter>
#include <QRectF>
#include <QSvgRenderer>

namespace {

bool readSvg(const QString& name, QString* svg)
{
    QFile file(Icons::resourcePath(name));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    *svg = QString::fromUtf8(file.readAll());
    return true;
}

QPixmap renderPixmap(const QString& svgData, const QString& color, int size, qreal dpr)
{
    QString svg = svgData;
    svg.replace(QLatin1String("currentColor"), color);

    QSvgRenderer renderer(svg.toUtf8());
    const int pixels = qMax(1, qRound(size * dpr));
    QPixmap pixmap(pixels, pixels);
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&painter, QRectF(0, 0, size, size));
    return pixmap;
}

void addPixmapSet(QIcon& icon, const QString& svg, const QString& normal,
                  const QString& hover, const QString& on)
{
    const qreal factors[] = {1.0, 2.0};
    for (const qreal dpr : factors) {
        icon.addPixmap(renderPixmap(svg, normal, Icons::ActionIconSize, dpr),
                       QIcon::Normal, QIcon::Off);
        icon.addPixmap(renderPixmap(svg, hover, Icons::ActionIconSize, dpr),
                       QIcon::Active, QIcon::Off);
        icon.addPixmap(renderPixmap(svg, Theme::IconDisabled, Icons::ActionIconSize, dpr),
                       QIcon::Disabled, QIcon::Off);
        icon.addPixmap(renderPixmap(svg, on, Icons::ActionIconSize, dpr),
                       QIcon::Normal, QIcon::On);
        icon.addPixmap(renderPixmap(svg, on, Icons::ActionIconSize, dpr),
                       QIcon::Active, QIcon::On);
        icon.addPixmap(renderPixmap(svg, Theme::IconDisabled, Icons::ActionIconSize, dpr),
                       QIcon::Disabled, QIcon::On);
    }
}

} // namespace

QString Icons::resourcePath(const QString& name)
{
    return QStringLiteral(":/icons/%1.svg").arg(name);
}

QPixmap Icons::pixmap(const QString& name, const QString& color, int size, qreal dpr)
{
    QString svg;
    if (!readSvg(name, &svg)) {
        return QPixmap();
    }
    return renderPixmap(svg, color, size, dpr);
}

QIcon Icons::icon(const QString& name)
{
    return icon(name, Theme::IconActive);
}

QIcon Icons::icon(const QString& name, const QString& onColor)
{
    static QHash<QString, QIcon> cache;
    const QString key = name + QLatin1Char('|') + onColor;
    const auto cached = cache.constFind(key);
    if (cached != cache.constEnd()) {
        return cached.value();
    }

    QIcon result;
    QString svg;
    if (readSvg(name, &svg)) {
        addPixmapSet(result, svg, Theme::IconNormal, Theme::IconHover, onColor);
    }
    cache.insert(key, result);
    return result;
}
