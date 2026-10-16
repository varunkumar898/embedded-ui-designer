#ifndef EW_HYDRATE_H
#define EW_HYDRATE_H

#include <QString>
#include <QStringList>
#include <QList>
#include "ir/IRTypes.h"
#include "runtime/ew_types.h"
#include "runtime/ew_tree.h"
#include "runtime/ew_layout.h"

namespace EmbedWeb {

struct HydrationDiagnostic {
    enum class Level { Info, Warning, Error };
    Level level = Level::Info;
    QString message;
    QString widgetId;
    QString widgetType;
};

struct HydrationResult {
    bool success = false;
    QList<HydrationDiagnostic> diagnostics;
    ew_tree_t tree;
    
    QStringList errorMessages() const {
        QStringList list;
        for (const auto& d : diagnostics) {
            if (d.level == HydrationDiagnostic::Level::Error) {
                list.append(d.message);
            }
        }
        return list;
    }
    
    QStringList warningMessages() const {
        QStringList list;
        for (const auto& d : diagnostics) {
            if (d.level == HydrationDiagnostic::Level::Warning) {
                list.append(d.message);
            }
        }
        return list;
    }
};

class IRHydrator {
public:
    IRHydrator();

    /**
     * @brief Hydrate an IRProject (active screen or specified screen) into an in-memory runtime ew_tree_t.
     * Validates IR, verifies node capacity, validates assets, and checks supported types.
     */
    HydrationResult hydrate(const IR::IRProject& project, int screenIndex = -1) const;

    /**
     * @brief Generate static C source file (ui_tree.c) for MCU target.
     * Zero JSON parsing on embedded device.
     */
    QString generateStaticTreeSource(const IR::IRProject& project, int screenIndex = -1, QString* outError = nullptr) const;

    /**
     * @brief Generate static C header file (ui_tree.h) for MCU target.
     */
    QString generateStaticTreeHeader(const IR::IRProject& project, int screenIndex = -1) const;

    /**
     * @brief Generate static C asset source file (ui_assets.c).
     */
    QString generateStaticAssetsSource(const IR::IRProject& project) const;

    /**
     * @brief Generate static C asset header file (ui_assets.h).
     */
    QString generateStaticAssetsHeader(const IR::IRProject& project) const;

    /**
     * @brief Check whether a component type is supported by the EmbedWeb runtime.
     */
    static bool isSupportedComponentType(const QString& type);

    /**
     * @brief Map normalized component type string to ew_widget_type_t.
     */
    static ew_widget_type_t mapWidgetType(const QString& type);

    /**
     * @brief Escape arbitrary text into a safe C string literal escaping backslashes,
     * quotes, newlines, carriage returns, tabs, and non-printable control characters.
     */
    static QString escapeCStringLiteral(const QString& s);
};

} // namespace EmbedWeb

#endif /* EW_HYDRATE_H */
