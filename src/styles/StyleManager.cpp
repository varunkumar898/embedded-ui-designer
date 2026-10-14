#include "StyleManager.h"
#include "theme/Theme.h"

#include <QFile>
#include <QTextStream>
#include <QApplication>
#include <QSettings>
#include <QDebug>
#include <QSysInfo>

QString DesignSystem::defaultFontFamily() {
#if defined(Q_OS_WIN)
    return QStringLiteral("Segoe UI");
#elif defined(Q_OS_MAC)
    return QStringLiteral("SF Pro Display");
#else
    return QStringLiteral("Ubuntu");
#endif
}

QString DesignSystem::monospaceFontFamily() {
#if defined(Q_OS_WIN)
    return QStringLiteral("Consolas");
#elif defined(Q_OS_MAC)
    return QStringLiteral("Monaco");
#else
    return QStringLiteral("Ubuntu Mono");
#endif
}

QFont DesignSystem::titleFont() {
    QFont font(defaultFontFamily(), 18, QFont::DemiBold);
    font.setStyleHint(QFont::SansSerif);
    return font;
}

QFont DesignSystem::panelTitleFont() {
    QFont font(defaultFontFamily(), 13, QFont::DemiBold);
    font.setStyleHint(QFont::SansSerif);
    return font;
}

QFont DesignSystem::sectionHeaderFont() {
    QFont font(defaultFontFamily(), 12, QFont::DemiBold);
    font.setStyleHint(QFont::SansSerif);
    return font;
}

QFont DesignSystem::bodyFont() {
    QFont font(defaultFontFamily(), 11, QFont::Normal);
    font.setStyleHint(QFont::SansSerif);
    return font;
}

QFont DesignSystem::smallFont() {
    QFont font(defaultFontFamily(), 10, QFont::Normal);
    font.setStyleHint(QFont::SansSerif);
    return font;
}

QFont DesignSystem::monoFont() {
    QFont font(monospaceFontFamily(), 10, QFont::Normal);
    font.setFixedPitch(true);
    font.setStyleHint(QFont::Monospace);
    return font;
}

QString StyleManager::loadStylesheet(const QString& path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        return in.readAll();
    }
    
    /* Fallback search for source tree paths if running unbundled */
    QString fallbackPath = path;
    if (path.startsWith(QLatin1String(":/styles/"))) {
        fallbackPath = QStringLiteral("src/styles/") + path.mid(9);
        QFile fallbackFile(fallbackPath);
        if (fallbackFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&fallbackFile);
            return in.readAll();
        }
    }
    
    qWarning() << "[StyleManager] Failed to load stylesheet from:" << path << "(fallback:" << fallbackPath << ")";
    return QString();
}

void StyleManager::applyTheme(const QString& theme, QWidget* target) {
    if (theme.toLower() != QLatin1String("light")) {
        QString qss = Theme::stylesheet();
        if (qApp) {
            qApp->setFont(DesignSystem::bodyFont());
        }
        if (target) {
            target->setStyleSheet(qss);
        } else if (qApp) {
            qApp->setStyleSheet(qss);
        }
        return;
    }

    QString qssPath = QStringLiteral(":/styles/LightTheme.qss");
    QString qss = loadStylesheet(qssPath);
    if (!qss.isEmpty()) {
        if (qApp) {
            qApp->setFont(DesignSystem::bodyFont());
        }
        if (target) {
            target->setStyleSheet(qss);
        } else if (qApp) {
            qApp->setStyleSheet(qss);
        }
    }
}

QString StyleManager::currentTheme() {
    QSettings settings;
    return settings.value(QStringLiteral("appearance/theme"), QStringLiteral("dark")).toString();
}

void StyleManager::setTheme(const QString& theme) {
    QSettings settings;
    settings.setValue(QStringLiteral("appearance/theme"), theme);
    applyTheme(theme);
}
