#pragma once

#include "CodeGenerator.h"
#include "CanvasScene.h"

class UgfxGenerator : public CodeGenerator {
public:
    explicit UgfxGenerator(Project* project, CanvasScene* scene);

    bool generate(const QString& outputDirectory) override;

private:
    CanvasScene* m_scene = nullptr;

    QString generateCMakeLists();
    QString generateGfxConf();
    QString generateUiHeader();
    QString generateUiSource();
    QString generateMainSource();
    QString generateReadme();

    bool hasButtons() const;
    bool hasLabels() const;
    bool hasProgressBars() const;
    bool hasRectangles() const;
};
