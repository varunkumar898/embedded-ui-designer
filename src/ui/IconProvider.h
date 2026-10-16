#pragma once

#include <QIcon>
#include <QPixmap>
#include <QString>

/**
 * Lucide icon provider for the minimal UI (Task 7).
 *
 * SVGs live in resources/icons/ (ISC license, see the LICENSE file there).
 * They are authored with stroke="currentColor" on a 24x24 grid and are
 * recolored from the Theme icon-state tokens at load time, so every icon
 * follows the active theme. Icons are rendered at ActionIconSize (20 px)
 * with 1x/2x pixmaps for high-DPI screens.
 */
namespace Icons {

/// Standard action icon size in logical pixels.
inline constexpr int ActionIconSize = 20;

/// Icon with normal / hover / active / disabled and on states from theme tokens.
QIcon icon(const QString& name);

/// Checkable icon whose "on" state uses `onColor` (e.g. Live Test).
QIcon icon(const QString& name, const QString& onColor);

/// Renders one recolored pixmap; used by tests and previews.
QPixmap pixmap(const QString& name, const QString& color, int size = ActionIconSize, qreal dpr = 1.0);

/// Resource path for an icon name: ":/icons/<name>.svg".
QString resourcePath(const QString& name);

} // namespace Icons
