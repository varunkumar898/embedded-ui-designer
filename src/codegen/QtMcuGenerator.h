#pragma once

#include "CodeGenerator.h"
#include "CanvasScene.h"
#include "Project.h"

class QtMcuGenerator : public CodeGenerator {
public:
    explicit QtMcuGenerator(Project* project, CanvasScene* scene);
    explicit QtMcuGenerator(const QString& projectFilePath);
    ~QtMcuGenerator() override;

    bool generate(const QString& outputDirectory) override;

    QString generateCMakeLists();
    QString generateQmlProject();
    QString generateDesignQml();
    QString generateReadme();

    // PC-Simulator subfolder generators
    QString generateSimulatorCMakeLists();
    QString generateSimulatorMainSource();
    QString generateSimulatorReadme();

    static bool isSupportedQulType(const QString& typeName);

private:
    CanvasScene* m_scene = nullptr;
    bool m_ownsProjectAndScene = false;
};
