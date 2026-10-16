#include "BuildConfiguration.h"
#include <QDir>
#include <QRegularExpression>

bool BuildConfiguration::validate(QString* errorMessage) const {
    if (workingDirectory.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = "Working directory is empty.";
        return false;
    }

    QDir dir(workingDirectory);
    if (!dir.exists()) {
        if (errorMessage) *errorMessage = QString("Working directory does not exist: %1").arg(workingDirectory);
        return false;
    }

    // Check for unsafe shell injection characters: ; & | ` $ > <
    static const QRegularExpression unsafeRegex(R"([;&|`$><])");
    if (customFlags.contains(unsafeRegex)) {
        if (errorMessage) *errorMessage = "Custom flags contain illegal shell control characters.";
        return false;
    }

    return true;
}
