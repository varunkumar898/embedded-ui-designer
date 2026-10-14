#ifndef EMBEDWEB_GENERATOR_H
#define EMBEDWEB_GENERATOR_H

#include <QString>
#include <QStringList>
#include <QDir>
#include "project/Project.h"

class Project;

namespace CodeGen {

class EmbedWebGenerator {
public:
    explicit EmbedWebGenerator(const Project* project);

    /**
     * @brief Generate all EmbedWeb artifacts into generated/ directory.
     */
    bool generate(const QDir& genDir, QStringList& generatedFiles, QString* outError);

    /**
     * @brief Generate manifest and resource estimation report.
     */
    QString generateManifestJson() const;

private:
    const Project* m_project;
};

} // namespace CodeGen

#endif /* EMBEDWEB_GENERATOR_H */
