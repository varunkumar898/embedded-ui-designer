#pragma once

#include "CodeGenerator.h"
#include "CanvasScene.h"
#include "Project.h"

class LvglGenerator : public CodeGenerator {
public:
    explicit LvglGenerator(Project* project, CanvasScene* scene);
    explicit LvglGenerator(const QString& projectFilePath);
    virtual ~LvglGenerator() override;

    bool generate(const QString& outputDirectory) override;

    QString generateCMakeLists();
    QString generateLvConf();
    QString generateUiHeader();
    QString generateUiSource();
    QString generateMainSource();
    QString generateReadme();
    QString generateIdfComponentYml();
    QString generatePlatformioIni();

    bool hasButtons() const;
    bool hasLabels() const;
    bool hasProgressBars() const;
    bool hasSliders() const;
    bool hasSwitches() const;
    bool hasCheckboxes() const;
    bool hasTextInputs() const;
    bool hasCircles() const;
    bool hasRectangles() const;
    bool hasImages() const;

private:
    CanvasScene* m_scene = nullptr;
    bool m_ownsProjectAndScene = false;
};
