#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QString>
#include <QFont>
#include <QWidget>

/**
 * @brief Platform-native typography and font hierarchy helpers.
 */
class DesignSystem {
public:
    static QString defaultFontFamily();
    static QString monospaceFontFamily();

    static QFont titleFont();
    static QFont panelTitleFont();
    static QFont sectionHeaderFont();
    static QFont bodyFont();
    static QFont smallFont();
    static QFont monoFont();
};

/**
 * @brief Manages application stylesheets, theme switching, and persistent appearance settings.
 */
class StyleManager {
public:
    /**
     * @brief Load stylesheet text from Qt resource path or filesystem.
     */
    static QString loadStylesheet(const QString& path);

    /**
     * @brief Apply a theme by name ("dark" or "light") to the whole application or target widget.
     */
    static void applyTheme(const QString& theme, QWidget* target = nullptr);

    /**
     * @brief Retrieve currently configured theme ("dark" by default).
     */
    static QString currentTheme();

    /**
     * @brief Save and apply selected theme across the application.
     */
    static void setTheme(const QString& theme);
};

#endif // STYLEMANAGER_H
