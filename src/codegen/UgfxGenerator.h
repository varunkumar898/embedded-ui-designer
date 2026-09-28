#pragma once

#include "CodeGenerator.h"
#include "CanvasScene.h"
#include "Project.h"

class UgfxGenerator : public CodeGenerator {
public:
    explicit UgfxGenerator(Project* project, CanvasScene* scene);
    explicit UgfxGenerator(const QString& projectFilePath);
    virtual ~UgfxGenerator() override;

    bool generate(const QString& outputDirectory) override;

private:
    CanvasScene* m_scene = nullptr;
    bool m_ownsProjectAndScene = false;

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
    bool hasImages() const;
    bool hasRoundedCorners() const;
};
