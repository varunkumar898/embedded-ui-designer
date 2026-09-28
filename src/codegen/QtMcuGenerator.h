#pragma once

#include "CodeGenerator.h"
#include "CanvasScene.h"

class QtMcuGenerator : public CodeGenerator {
public:
    explicit QtMcuGenerator(Project* project, CanvasScene* scene);

    bool generate(const QString& outputDirectory) override;

private:
    CanvasScene* m_scene = nullptr;

    QString generateCMakeLists();
    QString generateQmlProject();
    QString generateMainScreenQml();
    QString generateReadme();
};
