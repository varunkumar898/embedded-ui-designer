#include "UgfxGenerator.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include "ImageComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include "TextInputComponent.h"
#include "CircleComponent.h"
#include "PathComponent.h"
#include <QSet>
#include <QDir>
#include <QFileInfo>

UgfxGenerator::UgfxGenerator(Project* project, CanvasScene* scene)
    : CodeGenerator(project)
    , m_scene(scene)
    , m_ownsProjectAndScene(false)
{
}

UgfxGenerator::UgfxGenerator(const QString& projectFilePath)
    : CodeGenerator(nullptr)
    , m_ownsProjectAndScene(true)
{
    m_scene = new CanvasScene();
    m_project = new Project(m_scene);
    m_project->loadFromFile(projectFilePath);
}

UgfxGenerator::~UgfxGenerator() {
    if (m_ownsProjectAndScene) {
        delete m_project;
        delete m_scene;
    }
}

bool UgfxGenerator::hasButtons() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<ButtonComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasLabels() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<LabelComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasProgressBars() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<ProgressBarComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasSliders() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<SliderComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasSwitches() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<SwitchComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasCheckboxes() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<CheckboxComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasTextInputs() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<TextInputComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasCircles() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<CircleComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasRectangles() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<RectangleComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasImages() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<ImageComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasRoundedCorners() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (auto btn = dynamic_cast<ButtonComponent*>(comp)) {
            if (btn->cornerRadius() > 0) return true;
        } else if (auto rect = dynamic_cast<RectangleComponent*>(comp)) {
            if (rect->cornerRadius() > 0) return true;
        } else if (auto prog = dynamic_cast<ProgressBarComponent*>(comp)) {
            if (prog->cornerRadius() > 0) return true;
        }
    }
    return false;
}

bool UgfxGenerator::generate(const QString& outputDirectory) {
    if (!m_project || !m_scene) {
        m_lastError = "Project or canvas scene is null.";
        return false;
    }

    QDir outDir(outputDirectory);
    if (!outDir.exists() && !outDir.mkpath(".")) {
        m_lastError = QString("Could not create output directory: %1").arg(outputDirectory);
        return false;
    }

    // 1. CMakeLists.txt
    if (!writeFile(outDir.filePath("CMakeLists.txt"), generateCMakeLists())) return false;

    // 2. gfxconf.h
    if (!writeFile(outDir.filePath("gfxconf.h"), generateGfxConf())) return false;

    // 3. ui.h
    if (!writeFile(outDir.filePath("ui.h"), generateUiHeader())) return false;

    // 4. ui.c
    if (!writeFile(outDir.filePath("ui.c"), generateUiSource())) return false;

    // 5. main.c
    if (!writeFile(outDir.filePath("main.c"), generateMainSource())) return false;

    // 6. README.md
    if (!writeFile(outDir.filePath("README.md"), generateReadme())) return false;

    outDir.mkdir("build");
    return true;
}

QString UgfxGenerator::generateCMakeLists() {
    QString name = m_project ? m_project->projectName() : "ugfx_app";
    if (name.isEmpty()) name = "ugfx_app";

    QString code;
    code += "cmake_minimum_required(VERSION 3.20)\n";
    code += QString("project(%1 LANGUAGES C CXX)\n\n").arg(name);
    code += "set(CMAKE_C_STANDARD 99)\n";
    code += "set(CMAKE_C_STANDARD_REQUIRED ON)\n\n";

    code += "# Locate or fetch µGFX library\n";
    code += "if(DEFINED UGFX_PATH)\n";
    code += "    file(TO_CMAKE_PATH \"${UGFX_PATH}\" UGFX_DIR)\n";
    code += "elseif(DEFINED ENV{UGFX_PATH})\n";
    code += "    file(TO_CMAKE_PATH \"$ENV{UGFX_PATH}\" UGFX_DIR)\n";
    code += "elseif(EXISTS \"${CMAKE_CURRENT_SOURCE_DIR}/ugfx\")\n";
    code += "    set(UGFX_DIR \"${CMAKE_CURRENT_SOURCE_DIR}/ugfx\")\n";
    code += "elseif(EXISTS \"${CMAKE_CURRENT_SOURCE_DIR}/../ugfx\")\n";
    code += "    set(UGFX_DIR \"${CMAKE_CURRENT_SOURCE_DIR}/../ugfx\")\n";
    code += "else()\n";
    code += "    message(STATUS \"UGFX_PATH not set. Fetching uGFX from official repository...\")\n";
    code += "    include(FetchContent)\n";
    code += "    FetchContent_Declare(\n";
    code += "        ugfx_src\n";
    code += "        GIT_REPOSITORY https://git.ugfx.io/uGFX/ugfx.git\n";
    code += "        GIT_TAG master\n";
    code += "    )\n";
    code += "    FetchContent_MakeAvailable(ugfx_src)\n";
    code += "    set(UGFX_DIR \"${ugfx_src_SOURCE_DIR}\")\n";
    code += "endif()\n\n";

    code += "message(STATUS \"Using µGFX from: ${UGFX_DIR}\")\n\n";

    code += "set(APP_SOURCES\n";
    code += "    main.c\n";
    code += "    ui.c\n";
    code += "    \"${UGFX_DIR}/src/gfx_mk.c\"\n";
    code += ")\n\n";

    code += QString("add_executable(%1 ${APP_SOURCES})\n\n").arg(name);

    code += QString("target_include_directories(%1 PRIVATE\n").arg(name);
    code += "    \"${CMAKE_CURRENT_SOURCE_DIR}\"\n";
    code += "    \"${UGFX_DIR}\"\n";
    code += "    \"${UGFX_DIR}/src\"\n";
    code += ")\n\n";

    code += "if(WIN32)\n";
    code += QString("    target_include_directories(%1 PRIVATE \"${UGFX_DIR}/drivers/multiple/Win32\")\n").arg(name);
    code += QString("    target_sources(%1 PRIVATE \"${UGFX_DIR}/drivers/multiple/Win32/gdisp_lld_Win32.c\")\n").arg(name);
    code += QString("    target_link_libraries(%1 PRIVATE gdi32 user32)\n").arg(name);
    code += "elseif(UNIX AND NOT APPLE)\n";
    code += QString("    target_include_directories(%1 PRIVATE \"${UGFX_DIR}/drivers/multiple/X\")\n").arg(name);
    code += QString("    target_sources(%1 PRIVATE \"${UGFX_DIR}/drivers/multiple/X/gdisp_lld_X.c\")\n").arg(name);
    code += QString("    target_link_libraries(%1 PRIVATE pthread X11 m)\n").arg(name);
    code += "endif()\n";

    return code;
}

QString UgfxGenerator::generateGfxConf() {
    bool bButtons = hasButtons();
    bool bLabels = hasLabels();
    bool bProgress = hasProgressBars();
    bool bSliders = hasSliders();
    bool bSwitches = hasSwitches();
    bool bCheckboxes = hasCheckboxes();
    bool bTextInputs = hasTextInputs();
    bool bCircles = hasCircles();
    bool bImages = hasImages();
    bool bRounded = hasRoundedCorners();

    bool bWidgets = bButtons || bLabels || bProgress || bSliders || bSwitches || bCheckboxes || bTextInputs;
    bool bNeedText = bButtons || bLabels || bCheckboxes || bTextInputs;
    bool bNeedMouse = bButtons || bSliders || bSwitches || bCheckboxes || bTextInputs;
    bool bNeedEvents = bButtons || bSliders || bSwitches || bCheckboxes || bTextInputs;

    QString code;
    code += "/**\n";
    code += " * Auto-generated gfxconf.h by Embedded UI Designer\n";
    code += " * Defines exact µGFX subsystems and drivers used in this project.\n";
    code += " */\n";
    code += "#ifndef _GFXCONF_H\n";
    code += "#define _GFXCONF_H\n\n";

    code += "/* =============================================================== */\n";
    code += "/* GOS - Operating System Selection (Exactly ONE must be defined)   */\n";
    code += "/* =============================================================== */\n";
    code += "#if defined(_WIN32) || defined(__WIN32__)\n";
    code += "    #define GFX_USE_OS_WIN32                    GFXON\n";
    code += "    #define GFX_USE_OS_LINUX                    GFXOFF\n";
    code += "    #define GFX_USE_OS_RAW32                    GFXOFF\n";
    code += "#elif defined(__linux__) || defined(__unix__)\n";
    code += "    #define GFX_USE_OS_WIN32                    GFXOFF\n";
    code += "    #define GFX_USE_OS_LINUX                    GFXON\n";
    code += "    #define GFX_USE_OS_RAW32                    GFXOFF\n";
    code += "#else\n";
    code += "    #define GFX_USE_OS_WIN32                    GFXOFF\n";
    code += "    #define GFX_USE_OS_LINUX                    GFXOFF\n";
    code += "    #define GFX_USE_OS_RAW32                    GFXON\n";
    code += "#endif\n\n";

    code += "/* =============================================================== */\n";
    code += "/* GDISP - Graphics Subsystem                                      */\n";
    code += "/* =============================================================== */\n";
    code += "#define GFX_USE_GDISP                           GFXON\n";
    code += "#define GDISP_NEED_VALIDATION                   GFXON\n";
    code += "#define GDISP_NEED_CLIP                         GFXON\n";
    code += "#define GDISP_DEFAULT_ORIENTATION               GDISP_ROTATE_0\n\n";

    code += QString("#define GDISP_NEED_TEXT                         %1\n").arg(bNeedText ? "GFXON" : "GFXOFF");
    code += QString("#define GDISP_NEED_CIRCLE                       %1\n").arg((bCircles || bRounded) ? "GFXON" : "GFXOFF");
    code += QString("#define GDISP_NEED_ARC                          %1\n").arg(bRounded ? "GFXON" : "GFXOFF");
    code += QString("#define GDISP_NEED_ARCSECTORS                   %1\n").arg(bRounded ? "GFXON" : "GFXOFF");
    code += QString("#define GDISP_NEED_IMAGE                        %1\n").arg(bImages ? "GFXON" : "GFXOFF");
    code += QString("#define GDISP_NEED_IMAGE_BMP                    %1\n").arg(bImages ? "GFXON" : "GFXOFF");
    code += QString("#define GDISP_NEED_MULTITHREAD                  %1\n\n").arg(bWidgets ? "GFXON" : "GFXOFF");

    code += "/* Fonts required for labels and widgets */\n";
    code += QString("#define GDISP_INCLUDE_FONT_DEJAVUSANS12         %1\n").arg(bNeedText ? "GFXON" : "GFXOFF");
    code += QString("#define GDISP_INCLUDE_FONT_DEJAVUSANS16         %1\n").arg(bNeedText ? "GFXON" : "GFXOFF");
    code += QString("#define GDISP_INCLUDE_FONT_DEJAVUSANS24         %1\n\n").arg(bNeedText ? "GFXON" : "GFXOFF");

    code += "/* =============================================================== */\n";
    code += "/* GWIN - Window Management & Widgets                              */\n";
    code += "/* =============================================================== */\n";
    code += QString("#define GFX_USE_GWIN                            %1\n").arg(bWidgets ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_WINDOWMANAGER                 %1\n").arg(bWidgets ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_WIDGET                        %1\n").arg(bWidgets ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_FLAT_STYLING                       %1\n").arg(bWidgets ? "GFXON" : "GFXOFF");
    code += QString("#define GFX_USE_GQUEUE                          %1\n").arg(bWidgets ? "GFXON" : "GFXOFF");
    code += QString("#define GQUEUE_NEED_ASYNC                       %1\n").arg(bWidgets ? "GFXON" : "GFXOFF");
    code += QString("#define GFX_USE_GTIMER                          %1\n\n").arg(bWidgets ? "GFXON" : "GFXOFF");

    code += QString("#define GWIN_NEED_BUTTON                        %1\n").arg(bButtons ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_LABEL                         %1\n").arg(bLabels ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_PROGRESSBAR                   %1\n").arg(bProgress ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_SLIDER                        %1\n").arg(bSliders ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_CHECKBOX                      %1\n").arg((bSwitches || bCheckboxes) ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_TEXTEDIT                      %1\n\n").arg(bTextInputs ? "GFXON" : "GFXOFF");

    code += "/* =============================================================== */\n";
    code += "/* GEVENT & GINPUT - Event & Input Drivers (Mouse/Touch/Buttons)   */\n";
    code += "/* =============================================================== */\n";
    code += QString("#define GFX_USE_GEVENT                          %1\n").arg(bNeedEvents ? "GFXON" : "GFXOFF");
    code += QString("#define GFX_USE_GINPUT                          %1\n").arg(bNeedMouse ? "GFXON" : "GFXOFF");
    code += QString("#define GINPUT_NEED_MOUSE                       %1\n").arg(bNeedMouse ? "GFXON" : "GFXOFF");
    code += QString("#define GINPUT_NEED_KEYBOARD                    %1\n").arg(bTextInputs ? "GFXON" : "GFXOFF");
    code += "#define GEVENT_ASSERT_NO_RESOURCE               GFXOFF\n\n";

    code += "#endif /* _GFXCONF_H */\n";
    return code;
}

QString UgfxGenerator::generateUiHeader() {
    QString code;
    code += "#ifndef _UI_H\n";
    code += "#define _UI_H\n\n";
    code += "#include \"gfx.h\"\n\n";
    code += "#ifdef __cplusplus\n";
    code += "extern \"C\" {\n";
    code += "#endif\n\n";

    code += "// Widget handles\n";
    if (m_scene) {
        for (auto comp : m_scene->uiComponents()) {
            QString id = comp->componentId();
            if (dynamic_cast<ButtonComponent*>(comp)) {
                code += QString("extern GHandle ghBtn_%1;\n").arg(id);
            } else if (dynamic_cast<LabelComponent*>(comp)) {
                code += QString("extern GHandle ghLbl_%1;\n").arg(id);
            } else if (dynamic_cast<ProgressBarComponent*>(comp)) {
                code += QString("extern GHandle ghProg_%1;\n").arg(id);
            } else if (dynamic_cast<SliderComponent*>(comp)) {
                code += QString("extern GHandle ghSlider_%1;\n").arg(id);
            } else if (dynamic_cast<SwitchComponent*>(comp)) {
                code += QString("extern GHandle ghSwitch_%1;\n").arg(id);
            } else if (dynamic_cast<CheckboxComponent*>(comp)) {
                code += QString("extern GHandle ghChk_%1;\n").arg(id);
            } else if (dynamic_cast<TextInputComponent*>(comp)) {
                code += QString("extern GHandle ghEdit_%1;\n").arg(id);
            }
        }
    }

    code += "\n// Initialize the UI on the active display\n";
    code += "void ui_init(void);\n\n";

    code += "#ifdef __cplusplus\n";
    code += "}\n";
    code += "#endif\n\n";
    code += "#endif /* _UI_H */\n";
    return code;
}

QString UgfxGenerator::generateUiSource() {
    bool bButtons = hasButtons();
    bool bLabels = hasLabels();
    bool bProgress = hasProgressBars();
    bool bSliders = hasSliders();
    bool bWidgets = bButtons || bLabels || bProgress || bSliders;
    bool bNeedText = bButtons || bLabels;

    QString code;
    code += "#include \"gfx.h\"\n";
    code += "#include \"ui.h\"\n\n";
    code += "/* Prototype interaction callback stub: same-screen actions are not executed by this µGFX generator yet. */\n\n";

    // Define handles
    if (m_scene) {
        for (auto comp : m_scene->uiComponents()) {
            QString id = comp->componentId();
            if (dynamic_cast<ButtonComponent*>(comp)) {
                code += QString("GHandle ghBtn_%1 = 0;\n").arg(id);
            } else if (dynamic_cast<LabelComponent*>(comp)) {
                code += QString("GHandle ghLbl_%1 = 0;\n").arg(id);
            } else if (dynamic_cast<ProgressBarComponent*>(comp)) {
                code += QString("GHandle ghProg_%1 = 0;\n").arg(id);
            } else if (dynamic_cast<SliderComponent*>(comp)) {
                code += QString("GHandle ghSlider_%1 = 0;\n").arg(id);
            } else if (dynamic_cast<SwitchComponent*>(comp)) {
                code += QString("GHandle ghSwitch_%1 = 0;\n").arg(id);
            } else if (dynamic_cast<CheckboxComponent*>(comp)) {
                code += QString("GHandle ghChk_%1 = 0;\n").arg(id);
            } else if (dynamic_cast<TextInputComponent*>(comp)) {
                code += QString("GHandle ghEdit_%1 = 0;\n").arg(id);
            }
        }
    }

    code += "\nvoid ui_init(void) {\n";
    if (bWidgets) {
        code += "    GWidgetInit wi;\n";
        code += "    gwinWidgetClearInit(&wi);\n";
        code += "    wi.g.show = gTrue;\n\n";
    }

    if (bNeedText) {
        code += "    font_t fontNormal = gdispOpenFont(\"DejaVuSans12\");\n";
        code += "    font_t fontLarge = gdispOpenFont(\"DejaVuSans16\");\n\n";
    }

    if (m_scene) {
        // Rectangles / Background shapes first
        for (auto comp : m_scene->uiComponents()) {
            if (auto rect = dynamic_cast<RectangleComponent*>(comp)) {
                code += QString("    // Rectangle Shape: %1\n").arg(rect->componentId());
                int rx = static_cast<int>(rect->pos().x());
                int ry = static_cast<int>(rect->pos().y());
                int rw = static_cast<int>(rect->compWidth());
                int rh = static_cast<int>(rect->compHeight());
                int rad = rect->cornerRadius();
                QString fillHex = rect->fillColor().name().mid(1).toUpper();
                QString strokeHex = rect->strokeColor().name().mid(1).toUpper();

                if (rad > 0) {
                    code += QString("    gdispFillRoundedBox(%1, %2, %3, %4, %5, HTML2COLOR(0x%6));\n")
                        .arg(rx).arg(ry).arg(rw).arg(rh).arg(rad).arg(fillHex);
                    if (rect->strokeWidth() > 0) {
                        code += QString("    gdispDrawRoundedBox(%1, %2, %3, %4, %5, HTML2COLOR(0x%6));\n")
                            .arg(rx).arg(ry).arg(rw).arg(rh).arg(rad).arg(strokeHex);
                    }
                } else {
                    code += QString("    gdispFillArea(%1, %2, %3, %4, HTML2COLOR(0x%5));\n")
                        .arg(rx).arg(ry).arg(rw).arg(rh).arg(fillHex);
                    if (rect->strokeWidth() > 0) {
                        code += QString("    gdispDrawBox(%1, %2, %3, %4, HTML2COLOR(0x%5));\n")
                            .arg(rx).arg(ry).arg(rw).arg(rh).arg(strokeHex);
                    }
                }
                code += "\n";
            }
        }

        // Circles
        for (auto comp : m_scene->uiComponents()) {
            if (auto circ = dynamic_cast<CircleComponent*>(comp)) {
                int dim = static_cast<int>(qMin(circ->compWidth(), circ->compHeight()));
                int radius = dim / 2;
                int cx = static_cast<int>(circ->pos().x()) + radius;
                int cy = static_cast<int>(circ->pos().y()) + radius;

                code += QString("    // Circle Shape: %1\n").arg(circ->componentId());
                if (circ->isFilled()) {
                    code += QString("    gdispFillCircle(%1, %2, %3, HTML2COLOR(0x%4));\n")
                        .arg(cx).arg(cy).arg(radius).arg(circ->fillColor().name().mid(1).toUpper());
                }
                if (circ->strokeWidth() > 0) {
                    code += QString("    gdispDrawCircle(%1, %2, %3, HTML2COLOR(0x%4));\n")
                        .arg(cx).arg(cy).arg(radius).arg(circ->strokeColor().name().mid(1).toUpper());
                }
                code += "\n";
            }
        }

        // Curves are flattened to the PathComponent default subdivision count for µGFX.
        for (auto comp : m_scene->uiComponents()) {
            if (auto path = dynamic_cast<PathComponent*>(comp)) {
                const QPolygonF flattened = path->flattenedPoints();
                const QPointF origin = path->pos();
                const QString color = path->strokeColor().name().mid(1).toUpper();
                code += QString("    // Path %1 flattened to %2 polyline points\n")
                    .arg(path->componentId()).arg(flattened.size());
                for (int index = 0; index < flattened.size(); ++index) {
                    const QPointF& start = flattened.at(index);
                    const QPointF& end = flattened.at((index + 1) % flattened.size());
                    code += QString("    gdispDrawLine(%1, %2, %3, %4, HTML2COLOR(0x%5));\n")
                        .arg(qRound(origin.x() + start.x())).arg(qRound(origin.y() + start.y()))
                        .arg(qRound(origin.x() + end.x())).arg(qRound(origin.y() + end.y()))
                        .arg(color);
                }
                code += "\n";
            }
        }

        // Progress Bars
        for (auto comp : m_scene->uiComponents()) {
            if (auto prog = dynamic_cast<ProgressBarComponent*>(comp)) {
                QString id = prog->componentId();
                code += QString("    // Progress Bar: %1\n").arg(id);
                code += "    gwinWidgetClearInit(&wi);\n";
                code += "    wi.g.show = gTrue;\n";
                code += QString("    wi.g.x = %1; wi.g.y = %2;\n").arg(static_cast<int>(prog->pos().x())).arg(static_cast<int>(prog->pos().y()));
                code += QString("    wi.g.width = %1; wi.g.height = %2;\n").arg(static_cast<int>(prog->compWidth())).arg(static_cast<int>(prog->compHeight()));
                code += QString("    ghProg_%1 = gwinProgressbarCreate(0, &wi);\n").arg(id);
                code += QString("    gwinProgressbarSetPosition(ghProg_%1, %2);\n\n").arg(id).arg(static_cast<int>(prog->value() * 100));
            }
        }

        // Sliders
        for (auto comp : m_scene->uiComponents()) {
            if (auto slider = dynamic_cast<SliderComponent*>(comp)) {
                QString id = slider->componentId();
                code += QString("    // Slider: %1\n").arg(id);
                code += "    gwinWidgetClearInit(&wi);\n";
                code += "    wi.g.show = gTrue;\n";
                code += QString("    wi.g.x = %1; wi.g.y = %2;\n").arg(static_cast<int>(slider->pos().x())).arg(static_cast<int>(slider->pos().y()));
                code += QString("    wi.g.width = %1; wi.g.height = %2;\n").arg(static_cast<int>(slider->compWidth())).arg(static_cast<int>(slider->compHeight()));
                code += QString("    ghSlider_%1 = gwinSliderCreate(0, &wi);\n").arg(id);
                code += QString("    gwinSliderSetRange(ghSlider_%1, %2, %3);\n").arg(id).arg(slider->minimum()).arg(slider->maximum());
                code += QString("    gwinSliderSetPosition(ghSlider_%1, %2);\n\n").arg(id).arg(slider->value());
            }
        }

        // Switches
        for (auto comp : m_scene->uiComponents()) {
            if (auto sw = dynamic_cast<SwitchComponent*>(comp)) {
                QString id = sw->componentId();
                code += QString("    // Switch: %1\n").arg(id);
                code += "    gwinWidgetClearInit(&wi);\n";
                code += "    wi.g.show = gTrue;\n";
                code += QString("    wi.g.x = %1; wi.g.y = %2;\n").arg(static_cast<int>(sw->pos().x())).arg(static_cast<int>(sw->pos().y()));
                code += QString("    wi.g.width = %1; wi.g.height = %2;\n").arg(static_cast<int>(sw->compWidth())).arg(static_cast<int>(sw->compHeight()));
                code += QString("    ghSwitch_%1 = gwinCheckboxCreate(0, &wi);\n").arg(id);
                code += QString("    gwinCheckboxCheck(ghSwitch_%1, %2);\n\n").arg(id).arg(sw->isChecked() ? "gTrue" : "gFalse");
            }
        }

        // Checkboxes
        for (auto comp : m_scene->uiComponents()) {
            if (auto chk = dynamic_cast<CheckboxComponent*>(comp)) {
                QString id = chk->componentId();
                code += QString("    // Checkbox: %1\n").arg(id);
                code += "    gwinWidgetClearInit(&wi);\n";
                code += "    wi.g.show = gTrue;\n";
                code += QString("    wi.g.x = %1; wi.g.y = %2;\n").arg(static_cast<int>(chk->pos().x())).arg(static_cast<int>(chk->pos().y()));
                code += QString("    wi.g.width = %1; wi.g.height = %2;\n").arg(static_cast<int>(chk->compWidth())).arg(static_cast<int>(chk->compHeight()));
                code += QString("    wi.text = \"%1\";\n").arg(chk->text());
                code += QString("    ghChk_%1 = gwinCheckboxCreate(0, &wi);\n").arg(id);
                code += QString("    gwinCheckboxCheck(ghChk_%1, %2);\n\n").arg(id).arg(chk->isChecked() ? "gTrue" : "gFalse");
            }
        }

        // TextInputs
        for (auto comp : m_scene->uiComponents()) {
            if (auto txt = dynamic_cast<TextInputComponent*>(comp)) {
                QString id = txt->componentId();
                code += QString("    // TextInput: %1\n").arg(id);
                code += "    gwinWidgetClearInit(&wi);\n";
                code += "    wi.g.show = gTrue;\n";
                code += QString("    wi.g.x = %1; wi.g.y = %2;\n").arg(static_cast<int>(txt->pos().x())).arg(static_cast<int>(txt->pos().y()));
                code += QString("    wi.g.width = %1; wi.g.height = %2;\n").arg(static_cast<int>(txt->compWidth())).arg(static_cast<int>(txt->compHeight()));
                code += QString("    wi.text = \"%1\";\n").arg(txt->text());
                code += QString("    ghEdit_%1 = gwinTexteditCreate(0, &wi, 100);\n").arg(id);
                code += QString("    gwinSetColor(ghEdit_%1, HTML2COLOR(0x%2));\n")
                    .arg(id, txt->textColor().name().mid(1).toUpper());
                code += QString("    gwinSetBgColor(ghEdit_%1, HTML2COLOR(0x%2));\n\n")
                    .arg(id, txt->backgroundColor().name().mid(1).toUpper());
            }
        }

        // Labels
        for (auto comp : m_scene->uiComponents()) {
            if (auto lbl = dynamic_cast<LabelComponent*>(comp)) {
                QString id = lbl->componentId();
                code += QString("    // Label: %1\n").arg(id);
                code += "    gwinWidgetClearInit(&wi);\n";
                code += "    wi.g.show = gTrue;\n";
                code += QString("    wi.g.x = %1; wi.g.y = %2;\n").arg(static_cast<int>(lbl->pos().x())).arg(static_cast<int>(lbl->pos().y()));
                code += QString("    wi.g.width = %1; wi.g.height = %2;\n").arg(static_cast<int>(lbl->compWidth())).arg(static_cast<int>(lbl->compHeight()));
                code += QString("    wi.text = \"%1\";\n").arg(lbl->text());
                code += "    wi.customDraw = 0;\n";
                code += QString("    ghLbl_%1 = gwinLabelCreate(0, &wi);\n").arg(id);
                code += QString("    gwinSetColor(ghLbl_%1, HTML2COLOR(0x%2));\n")
                    .arg(id, lbl->color().name().mid(1).toUpper());
                if (lbl->pixelSize() >= 16) {
                    code += QString("    gwinSetFont(ghLbl_%1, fontLarge);\n").arg(id);
                } else {
                    code += QString("    gwinSetFont(ghLbl_%1, fontNormal);\n").arg(id);
                }
                code += "\n";
            }
        }

        // Buttons
        for (auto comp : m_scene->uiComponents()) {
            if (auto btn = dynamic_cast<ButtonComponent*>(comp)) {
                QString id = btn->componentId();
                code += QString("    // Button: %1\n").arg(id);
                code += "    gwinWidgetClearInit(&wi);\n";
                code += "    wi.g.show = gTrue;\n";
                code += QString("    wi.g.x = %1; wi.g.y = %2;\n").arg(static_cast<int>(btn->pos().x())).arg(static_cast<int>(btn->pos().y()));
                code += QString("    wi.g.width = %1; wi.g.height = %2;\n").arg(static_cast<int>(btn->compWidth())).arg(static_cast<int>(btn->compHeight()));
                code += QString("    wi.text = \"%1\";\n").arg(btn->text());
                code += "    wi.customDraw = 0;\n";
                code += QString("    ghBtn_%1 = gwinButtonCreate(0, &wi);\n").arg(id);
                code += QString("    gwinSetColor(ghBtn_%1, HTML2COLOR(0x%2));\n")
                    .arg(id, btn->textColor().name().mid(1).toUpper());
                code += QString("    gwinSetBgColor(ghBtn_%1, HTML2COLOR(0x%2));\n")
                    .arg(id, btn->backgroundColor().name().mid(1).toUpper());
                code += QString("    gwinSetFont(ghBtn_%1, fontNormal);\n\n").arg(id);
            }
        }
    }

    code += "}\n";
    return code;
}

QString UgfxGenerator::generateMainSource() {
    bool bButtons = hasButtons();
    bool bSwitches = hasSwitches();
    bool bSliders = hasSliders();
    bool bCheckboxes = hasCheckboxes();
    bool bTextInputs = hasTextInputs();
    bool bNeedEvents = bButtons || bSwitches || bSliders || bCheckboxes || bTextInputs;

    QString code;
    code += "#include \"gfx.h\"\n";
    code += "#include \"ui.h\"\n";
    code += "#include <stdio.h>\n\n";

    code += "int main(int argc, char *argv[]) {\n";
    code += "    (void)argc;\n";
    code += "    (void)argv;\n\n";
    code += "    // 1. Initialize µGFX core and drivers\n";
    code += "    gfxInit();\n\n";
    code += "    // 2. Set background color\n";
    QString bgHex = (m_scene) ? m_scene->screenBackgroundColor().name().mid(1).toUpper() : "FFFFFF";
    code += QString("    gdispClear(HTML2COLOR(0x%1));\n\n").arg(bgHex);
    code += "    // 3. Create visual UI hierarchy\n";
    code += "    ui_init();\n\n";

    if (bNeedEvents) {
        code += "    // 4. Attach event listener for widgets and touch input (PushButton pattern)\n";
        code += "    GListener gl;\n";
        code += "    geventListenerInit(&gl);\n";
        code += "    gwinAttachListener(&gl);\n\n";
        code += "    printf(\"µGFX UI initialized successfully. Entering event loop...\\n\");\n\n";
        code += "    while (gTrue) {\n";
        code += "        GEvent *pe = geventEventWait(&gl, 100);\n";
        code += "        if (pe && pe->type == GEVENT_GWIN_BUTTON) {\n";
        code += "            GEventGWinButton *peb = (GEventGWinButton *)pe;\n";

        if (m_scene) {
            for (auto comp : m_scene->uiComponents()) {
                if (auto btn = dynamic_cast<ButtonComponent*>(comp)) {
                    QString id = btn->componentId();
                    QString handler = btn->onClickedHandler().trimmed();
                    code += QString("            if (peb->gwin == ghBtn_%1) {\n").arg(id);
                    code += QString("                printf(\"[EVENT] Button clicked: %1 (%2)\\n\");\n").arg(id, handler);
                    code += "                // TODO: Add your hardware action or state machine handler here\n";
                    code += "            }\n";
                }
            }
        }

        code += "        }\n";
        code += "    }\n\n";
    } else {
        code += "    printf(\"µGFX UI initialized successfully.\\n\");\n\n";
        code += "    while (gTrue) {\n";
        code += "        gfxSleepMilliseconds(100);\n";
        code += "    }\n\n";
    }

    code += "    return 0;\n";
    code += "}\n";
    return code;
}

QString UgfxGenerator::generateReadme() {
    DisplayConfig cfg = m_project ? m_project->displayConfig() : DisplayConfig();
    QString name = m_project ? m_project->projectName() : "ugfx_app";

    QString md;
    md += QString("# %1 (µGFX C Project)\n\n").arg(name);
    md += "Auto-generated by **Embedded UI Designer**.\n\n";
    md += "## Specifications\n";
    md += QString("- **Target Display**: %1 × %2 (%3-bit)\n").arg(cfg.width).arg(cfg.height).arg(cfg.colorDepth);
    md += "- **Build Method**: Single-file compilation via `src/gfx_mk.c`\n\n";
    md += "## How to Compile & Run on PC (Windows / Linux)\n\n";
    md += "If you have a local clone of µGFX:\n";
    md += "```bash\n";
    md += "mkdir build && cd build\n";
    md += "cmake .. -DUGFX_PATH=/path/to/ugfx\n";
    md += "cmake --build .\n";
    md += "```\n\n";
    md += "Or compile directly with plain gcc/clang:\n";
    md += "```bash\n";
    md += "# On Linux:\n";
    md += "gcc -std=c99 main.c ui.c /path/to/ugfx/src/gfx_mk.c /path/to/ugfx/drivers/multiple/X/gdisp_lld_X.c \\\n";
    md += "  -I. -I/path/to/ugfx -I/path/to/ugfx/src -I/path/to/ugfx/drivers/multiple/X -lX11 -lpthread -o " + name + "\n";
    md += "# On Windows (MinGW):\n";
    md += "gcc -std=c99 main.c ui.c /path/to/ugfx/src/gfx_mk.c /path/to/ugfx/drivers/multiple/Win32/gdisp_lld_Win32.c \\\n";
    md += "  -I. -I/path/to/ugfx -I/path/to/ugfx/src -I/path/to/ugfx/drivers/multiple/Win32 -lgdi32 -luser32 -o " + name + ".exe\n";
    md += "```\n\n";
    md += "If `UGFX_PATH` is not specified, CMake will automatically download µGFX via FetchContent.\n\n";
    md += "## Cross-Compiling for MCUs (STM32, ESP32, etc.)\n";
    md += "To flash onto baremetal microcontroller hardware:\n";
    md += "1. Set `#define GFX_USE_OS_RAW32 GFXON` (or FreeRTOS) in `gfxconf.h`.\n";
    md += "2. Include your board display driver (e.g. `drivers/gdisp/ILI9341` or `drivers/gdisp/ST7789`).\n";
    md += "3. Add `ui.c`, `ui.h`, and `gfxconf.h` into your Keil, STM32CubeIDE, or ESP-IDF build.\n";
    return md;
}
