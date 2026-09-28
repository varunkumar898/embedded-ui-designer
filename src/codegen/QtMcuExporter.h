#pragma once

#include "IExporter.h"

/**
 * @brief Qt for MCUs (Qt Quick Ultralite / QUL) Exporter Subclass
 * 
 * Generates ready-to-build QUL projects (CMakeLists.txt, .qmlproject, MainScreen.qml)
 * from the DocumentModel DOM utilizing strictly supported QUL QML types.
 */
class QtMcuExporter : public IExporter {
public:
    QtMcuExporter() = default;
    virtual ~QtMcuExporter() override = default;

    bool exportProject(const DocumentModel* doc, const QString& outDir) override;
    QString lastError() const override { return m_lastError; }

private:
    QString m_lastError;

    QString generateCMakeLists(const DocumentModel* doc);
    QString generateQmlProject(const DocumentModel* doc);
    QString generateMainScreenQml(const DocumentModel* doc);
    QString generateReadme(const DocumentModel* doc);
};
