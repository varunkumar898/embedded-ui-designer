#include "UgfxGenerator.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include <QSet>

UgfxGenerator::UgfxGenerator(Project* project, CanvasScene* scene)
    : CodeGenerator(project)
    , m_scene(scene)
{
}

bool UgfxGenerator::hasButtons() const {
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<ButtonComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasLabels() const {
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<LabelComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasProgressBars() const {
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<ProgressBarComponent*>(comp)) return true;
    }
    return false;
}

bool UgfxGenerator::hasRectangles() const {
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<RectangleComponent*>(comp)) return true;
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
    QString name = m_project->projectName();
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
    code += QString("    target_link_libraries(%1 PRIVATE pthread X11)\n").arg(name);
    code += "endif()\n";

    return code;
}

QString UgfxGenerator::generateGfxConf() {
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
    code += "#elif defined(__linux__) || defined(__unix__)\n";
    code += "    #define GFX_USE_OS_LINUX                    GFXON\n";
    code += "#elif defined(GFX_USE_OS_FREERTOS_CONFIG)\n";
    code += "    #define GFX_USE_OS_FREERTOS                 GFXON\n";
    code += "#else\n";
    code += "    #define GFX_USE_OS_RAW32                    GFXON\n";
    code += "#endif\n\n";

    code += "/* =============================================================== */\n";
    code += "/* GDISP - Graphics Subsystem                                      */\n";
    code += "/* =============================================================== */\n";
    code += "#define GFX_USE_GDISP                           GFXON\n";
    code += "#define GDISP_NEED_VALIDATION                   GFXON\n";
    code += "#define GDISP_NEED_CLIP                         GFXON\n";
    code += "#define GDISP_NEED_TEXT                         GFXON\n";
    code += "#define GDISP_NEED_CIRCLE                       GFXON\n";
    code += "#define GDISP_NEED_MULTITHREAD                  GFXON\n";
    code += "#define GDISP_DEFAULT_ORIENTATION               GDISP_ROTATE_0\n\n";

    code += "/* Fonts required for labels and widgets */\n";
    code += "#define GDISP_INCLUDE_FONT_DEJAVUSANS12         GFXON\n";
    code += "#define GDISP_INCLUDE_FONT_DEJAVUSANS16         GFXON\n";
    code += "#define GDISP_INCLUDE_FONT_DEJAVUSANS24         GFXON\n\n";

    code += "/* =============================================================== */\n";
    code += "/* GWIN - Window Management & Widgets                              */\n";
    code += "/* =============================================================== */\n";
    code += "#define GFX_USE_GWIN                            GFXON\n";
    code += "#define GWIN_NEED_WINDOWMANAGER                 GFXON\n";
    code += "#define GWIN_NEED_WIDGET                        GFXON\n";
    code += "#define GWIN_FLAT_STYLING                       GFXON\n";

    code += QString("#define GWIN_NEED_BUTTON                        %1\n").arg(hasButtons() ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_LABEL                         %1\n").arg(hasLabels() ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_PROGRESSBAR                   %1\n\n").arg(hasProgressBars() ? "GFXON" : "GFXOFF");

    code += "/* =============================================================== */\n";
    code += "/* GEVENT & GINPUT - Event & Input Drivers (Mouse/Touch/Buttons)   */\n";
    code += "/* =============================================================== */\n";
    code += "#define GFX_USE_GEVENT                          GFXON\n";
    code += "#define GFX_USE_GINPUT                          GFXON\n";
    code += "#define GINPUT_NEED_MOUSE                       GFXON\n";
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
    for (auto comp : m_scene->uiComponents()) {
        QString id = comp->componentId();
        if (dynamic_cast<ButtonComponent*>(comp)) {
            code += QString("extern GHandle ghBtn_%1;\n").arg(id);
        } else if (dynamic_cast<LabelComponent*>(comp)) {
            code += QString("extern GHandle ghLbl_%1;\n").arg(id);
        } else if (dynamic_cast<ProgressBarComponent*>(comp)) {
            code += QString("extern GHandle ghProg_%1;\n").arg(id);
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
    DisplayConfig cfg = m_project->displayConfig();

    QString code;
    code += "#include \"gfx.h\"\n";
    code += "#include \"ui.h\"\n\n";

    // Define handles
    for (auto comp : m_scene->uiComponents()) {
        QString id = comp->componentId();
        if (dynamic_cast<ButtonComponent*>(comp)) {
            code += QString("GHandle ghBtn_%1 = 0;\n").arg(id);
        } else if (dynamic_cast<LabelComponent*>(comp)) {
            code += QString("GHandle ghLbl_%1 = 0;\n").arg(id);
        } else if (dynamic_cast<ProgressBarComponent*>(comp)) {
            code += QString("GHandle ghProg_%1 = 0;\n").arg(id);
        }
    }

    code += "\nvoid ui_init(void) {\n";
    code += "    GWidgetInit wi;\n";
    code += "    gwinWidgetClearInit(&wi);\n";
    code += "    wi.g.show = gTrue;\n\n";

    // Set font
    code += "    font_t fontNormal = gdispOpenFont(\"DejaVuSans12\");\n";
    code += "    font_t fontLarge = gdispOpenFont(\"DejaVuSans16\");\n\n";

    // Rectangles / Panels first
    for (auto comp : m_scene->uiComponents()) {
        if (auto rect = dynamic_cast<RectangleComponent*>(comp)) {
            code += QString("    // Rectangle Shape: %1\n").arg(rect->componentId());
            code += QString("    gdispFillArea(%1, %2, %3, %4, HTML2COLOR(0x%5));\n")
                .arg(static_cast<int>(rect->pos().x()))
                .arg(static_cast<int>(rect->pos().y()))
                .arg(static_cast<int>(rect->compWidth()))
                .arg(static_cast<int>(rect->compHeight()))
                .arg(rect->fillColor().name().mid(1).toUpper());
            if (rect->strokeWidth() > 0) {
                code += QString("    gdispDrawBox(%1, %2, %3, %4, HTML2COLOR(0x%5));\n")
                    .arg(static_cast<int>(rect->pos().x()))
                    .arg(static_cast<int>(rect->pos().y()))
                    .arg(static_cast<int>(rect->compWidth()))
                    .arg(static_cast<int>(rect->compHeight()))
                    .arg(rect->strokeColor().name().mid(1).toUpper());
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
            code += QString("    ghProg_%1 = gwinProgressbarCreate(NULL, &wi);\n").arg(id);
            code += QString("    gwinProgressbarSetPosition(ghProg_%1, %2);\n\n").arg(id).arg(static_cast<int>(prog->value() * 100));
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
            code += "    wi.customDraw = NULL;\n";
            code += QString("    ghLbl_%1 = gwinLabelCreate(NULL, &wi);\n").arg(id);
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
            code += "    wi.customDraw = NULL;\n";
            code += QString("    ghBtn_%1 = gwinButtonCreate(NULL, &wi);\n").arg(id);
            code += QString("    gwinSetFont(ghBtn_%1, fontNormal);\n\n").arg(id);
        }
    }

    code += "}\n";
    return code;
}

QString UgfxGenerator::generateMainSource() {
    DisplayConfig cfg = m_project->displayConfig();

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
    code += QString("    gdispClear(HTML2COLOR(0x%1));\n\n").arg(m_scene->screenBackgroundColor().name().mid(1).toUpper());
    code += "    // 3. Create visual UI hierarchy\n";
    code += "    ui_init();\n\n";
    code += "    // 4. Attach event listener for buttons and touch input\n";
    code += "    GListener gl;\n";
    code += "    geventListenerInit(&gl);\n";
    code += "    gwinAttachListener(&gl);\n\n";
    code += "    printf(\"µGFX UI initialized successfully. Entering event loop...\\n\");\n\n";
    code += "    while (gTrue) {\n";
    code += "        GEvent *pe = geventEventWait(&gl, 100);\n";
    code += "        if (pe && pe->type == GEVENT_GWIN_BUTTON) {\n";
    code += "            GEventGWinButton *peb = (GEventGWinButton *)pe;\n";

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

    code += "        }\n";
    code += "    }\n\n";
    code += "    return 0;\n";
    code += "}\n";
    return code;
}

QString UgfxGenerator::generateReadme() {
    DisplayConfig cfg = m_project->displayConfig();
    QString name = m_project->projectName();

    QString md;
    md += QString("# %1 (µGFX C Project)\n\n").arg(name);
    md += "Auto-generated by **Embedded UI Designer**.\n\n";
    md += "## Specifications\n";
    md += QString("- **Target Display**: %1 × %2 (%3-bit)\n").arg(cfg.width).arg(cfg.height).arg(cfg.colorDepth);
    md += "- **Build Method**: Single-file compilation via `src/gfx_mk.c`\n\n";
    md += "## How to Compile & Run on PC (Windows / Linux)\n\n";
    md += "If you already have a local clone of µGFX:\n";
    md += "```bash\n";
    md += "mkdir build && cd build\n";
    md += "cmake .. -DUGFX_PATH=/path/to/ugfx\n";
    md += "cmake --build .\n";
    md += "```\n\n";
    md += "If `UGFX_PATH` is not specified, CMake will automatically download µGFX via FetchContent.\n\n";
    md += "## Cross-Compiling for MCUs (STM32, ESP32, etc.)\n";
    md += "To flash onto baremetal microcontroller hardware:\n";
    md += "1. Set `#define GFX_USE_OS_RAW32 GFXON` (or FreeRTOS) in `gfxconf.h`.\n";
    md += "2. Include your board display driver (e.g. `drivers/gdisp/ILI9341` or `drivers/gdisp/ST7789`).\n";
    md += "3. Add `ui.c`, `ui.h`, and `gfxconf.h` into your Keil, STM32CubeIDE, or ESP-IDF build.\n";
    return md;
}
