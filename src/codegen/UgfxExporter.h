#pragma once

#include "IExporter.h"

/**
 * @brief µGFX Exporter Subclass
 * 
 * Generates an end-to-end buildable µGFX C project (main.c, gfxconf.h, ui.c, ui.h, CMakeLists.txt)
 * from the DocumentModel DOM.
 */
class UgfxExporter : public IExporter {
public:
    UgfxExporter() = default;
    virtual ~UgfxExporter() override = default;

    bool exportProject(const DocumentModel* doc, const QString& outDir) override;
    QString lastError() const override { return m_lastError; }

private:
    QString m_lastError;

    QString generateCMakeLists(const DocumentModel* doc);
    QString generateGfxConf(const DocumentModel* doc);
    QString generateUiHeader(const DocumentModel* doc);
    QString generateUiSource(const DocumentModel* doc);
    QString generateMainSource(const DocumentModel* doc);
};
