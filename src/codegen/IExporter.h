#pragma once

#include <QString>
#include <QDir>
#include <QFile>
#include <QTextStream>

class DocumentModel;

/**
 * @brief Abstract Exporter Interface
 * 
 * Defines the contract for all microcontroller code generation backends.
 * Consumes the reactive DocumentModel (DOM) and produces complete buildable toolchains.
 */
class IExporter {
public:
    virtual ~IExporter() = default;

    /**
     * @brief Generates complete project source files and build manifests into target directory.
     * @param doc The DocumentModel instance representing the UI canvas hierarchy.
     * @param outDir Destination output folder on filesystem.
     * @return true if export succeeded without errors, false otherwise.
     */
    virtual bool exportProject(const DocumentModel* doc, const QString& outDir) = 0;

    /**
     * @brief Returns human-readable error description if exportProject returns false.
     */
    virtual QString lastError() const = 0;

protected:
    static bool writeTextFile(const QString& filePath, const QString& content, QString* errorOut = nullptr) {
        QFileInfo fi(filePath);
        QDir dir = fi.dir();
        if (!dir.exists() && !dir.mkpath(".")) {
            if (errorOut) *errorOut = QString("Could not create directory: %1").arg(dir.absolutePath());
            return false;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            if (errorOut) *errorOut = QString("Failed to open file for writing: %1").arg(filePath);
            return false;
        }

        QTextStream stream(&file);
        stream << content;
        file.close();
        return true;
    }
};
