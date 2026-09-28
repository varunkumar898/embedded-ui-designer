#pragma once

#include <QString>
#include <QDir>
#include "Project.h"

class CodeGenerator {
public:
    explicit CodeGenerator(Project* project) : m_project(project) {}
    virtual ~CodeGenerator() = default;

    virtual bool generate(const QString& outputDirectory) = 0;
    QString lastError() const { return m_lastError; }

protected:
    Project* m_project = nullptr;
    QString m_lastError;

    bool writeFile(const QString& filePath, const QString& content) {
        QFileInfo fi(filePath);
        QDir dir = fi.dir();
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                m_lastError = QString("Failed to create directory: %1").arg(dir.absolutePath());
                return false;
            }
        }

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            m_lastError = QString("Failed to open file for writing: %1").arg(filePath);
            return false;
        }

        QTextStream out(&file);
        out << content;
        file.close();
        return true;
    }
};
