#include "LvglGenerator.h"
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

static UIComponent* findInteractionTarget(CanvasScene* scene, const QString& componentId) {
    if (!scene) return nullptr;
    for (UIComponent* component : scene->uiComponents()) {
        if (component->componentId() == componentId) return component;
    }
    return nullptr;
}

static QString lvglObjectName(UIComponent* component) {
    if (dynamic_cast<ButtonComponent*>(component)) return "ui_btn_" + component->componentId();
    if (dynamic_cast<LabelComponent*>(component)) return "ui_lbl_" + component->componentId();
    if (dynamic_cast<RectangleComponent*>(component)) return "ui_rect_" + component->componentId();
    if (dynamic_cast<ProgressBarComponent*>(component)) return "ui_bar_" + component->componentId();
    if (dynamic_cast<SliderComponent*>(component)) return "ui_slider_" + component->componentId();
    if (dynamic_cast<SwitchComponent*>(component)) return "ui_sw_" + component->componentId();
    if (dynamic_cast<CheckboxComponent*>(component)) return "ui_cb_" + component->componentId();
    if (dynamic_cast<TextInputComponent*>(component)) return "ui_ta_" + component->componentId();
    if (dynamic_cast<CircleComponent*>(component)) return "ui_circle_" + component->componentId();
    if (dynamic_cast<ImageComponent*>(component)) return "ui_img_" + component->componentId();
    return {};
}

static QString lvglInteractionAction(const QJsonObject& interaction, CanvasScene* scene) {
    UIComponent* target = findInteractionTarget(scene, interaction.value("target").toString());
    const QString object = lvglObjectName(target);
    if (!target || object.isEmpty()) return {};
    const QString action = interaction.value("action").toString();
    const QString value = interaction.value("value").toString();
    QString code;
    if (action == "Show") {
        code = QString("lv_obj_clear_flag(%1, LV_OBJ_FLAG_HIDDEN);\n").arg(object);
    } else if (action == "Hide") {
        code = QString("lv_obj_add_flag(%1, LV_OBJ_FLAG_HIDDEN);\n").arg(object);
    } else if (dynamic_cast<SwitchComponent*>(target) || dynamic_cast<CheckboxComponent*>(target)) {
        const QString checked = action == "Toggle"
            ? QString("!lv_obj_has_state(%1, LV_STATE_CHECKED)").arg(object)
            : (value.compare("true", Qt::CaseInsensitive) == 0 || value == "1" ? "true" : "false");
        code = QString("if (%1) lv_obj_add_state(%2, LV_STATE_CHECKED); else lv_obj_clear_state(%2, LV_STATE_CHECKED);\n")
            .arg(checked, object);
        if (interaction.value("transition").toString() != "Instant") {
            code += QString("/* %1 transition requested for %2 ms; LVGL switch/checkbox state animation remains theme-controlled. */\n")
                .arg(interaction.value("transition").toString())
                .arg(interaction.value("duration").toInt());
        }
    } else if (action == "Toggle") {
        code = QString("if (lv_obj_has_flag(%1, LV_OBJ_FLAG_HIDDEN)) lv_obj_clear_flag(%1, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(%1, LV_OBJ_FLAG_HIDDEN);\n").arg(object);
    } else if (action == "Set Value") {
        if (dynamic_cast<SliderComponent*>(target)) code = QString("lv_slider_set_value(%1, %2, LV_ANIM_ON);\n").arg(object, value);
        else if (dynamic_cast<ProgressBarComponent*>(target)) code = QString("lv_bar_set_value(%1, %2, LV_ANIM_ON);\n").arg(object, QString::number(qRound(value.toDouble() * 100.0)));
        else if (dynamic_cast<LabelComponent*>(target)) code = QString("lv_label_set_text(%1, \"%2\");\n").arg(object, value);
        else if (dynamic_cast<TextInputComponent*>(target)) code = QString("lv_textarea_set_text(%1, \"%2\");\n").arg(object, value);
        else if (dynamic_cast<ButtonComponent*>(target)) code = QString("lv_label_set_text(%1_label, \"%2\");\n").arg(object, value);
    }
    return code;
}

static void appendLvglInteractions(QString& code, UIComponent* source, CanvasScene* scene,
                                  const QStringList& triggers, const QString& indent) {
    for (const QJsonValue& value : source->interactions()) {
        const QJsonObject interaction = value.toObject();
        if (!triggers.contains(interaction.value("trigger").toString())) continue;
        const QString action = lvglInteractionAction(interaction, scene);
        if (!action.isEmpty()) {
            for (const QString& line : action.split('\n', Qt::SkipEmptyParts)) code += indent + line + "\n";
        }
    }
}

LvglGenerator::LvglGenerator(Project* project, CanvasScene* scene)
    : CodeGenerator(project)
    , m_scene(scene)
    , m_ownsProjectAndScene(false)
{
}

LvglGenerator::LvglGenerator(const QString& projectFilePath)
    : CodeGenerator(nullptr)
    , m_ownsProjectAndScene(true)
{
    m_scene = new CanvasScene();
    m_project = new Project(m_scene);
    m_project->loadFromFile(projectFilePath);
}

LvglGenerator::~LvglGenerator() {
    if (m_ownsProjectAndScene) {
        delete m_project;
        delete m_scene;
    }
}

bool LvglGenerator::hasButtons() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<ButtonComponent*>(comp)) return true;
    }
    return false;
}

bool LvglGenerator::hasLabels() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<LabelComponent*>(comp)) return true;
    }
    return false;
}

bool LvglGenerator::hasProgressBars() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<ProgressBarComponent*>(comp)) return true;
    }
    return false;
}

bool LvglGenerator::hasSliders() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<SliderComponent*>(comp)) return true;
    }
    return false;
}

bool LvglGenerator::hasSwitches() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<SwitchComponent*>(comp)) return true;
    }
    return false;
}

bool LvglGenerator::hasCheckboxes() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<CheckboxComponent*>(comp)) return true;
    }
    return false;
}

bool LvglGenerator::hasTextInputs() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<TextInputComponent*>(comp)) return true;
    }
    return false;
}

bool LvglGenerator::hasCircles() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<CircleComponent*>(comp)) return true;
    }
    return false;
}

bool LvglGenerator::hasRectangles() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<RectangleComponent*>(comp)) return true;
    }
    return false;
}

bool LvglGenerator::hasImages() const {
    if (!m_scene) return false;
    for (auto comp : m_scene->uiComponents()) {
        if (dynamic_cast<ImageComponent*>(comp)) return true;
    }
    return false;
}

bool LvglGenerator::generate(const QString& outputDirectory) {
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

    // 2. lv_conf.h
    if (!writeFile(outDir.filePath("lv_conf.h"), generateLvConf())) return false;

    // 3. ui.h
    if (!writeFile(outDir.filePath("ui.h"), generateUiHeader())) return false;

    // 4. ui.c
    if (!writeFile(outDir.filePath("ui.c"), generateUiSource())) return false;

    // 5. main.c
    if (!writeFile(outDir.filePath("main.c"), generateMainSource())) return false;

    // 6. idf_component.yml (ESP-IDF component manager)
    if (!writeFile(outDir.filePath("idf_component.yml"), generateIdfComponentYml())) return false;

    // 7. platformio.ini (PlatformIO for ESP32/Arduino)
    if (!writeFile(outDir.filePath("platformio.ini"), generatePlatformioIni())) return false;

    // 8. README.md
    if (!writeFile(outDir.filePath("README.md"), generateReadme())) return false;

    // 9. pc_simulator/ subfolder (Desktop simulator with SDL2 driver)
    QDir simDir(outDir.filePath("pc_simulator"));
    if (!simDir.exists() && !simDir.mkpath(".")) {
        m_lastError = QString("Could not create pc_simulator directory: %1").arg(simDir.absolutePath());
        return false;
    }
    if (!writeFile(simDir.filePath("CMakeLists.txt"), generateSimulatorCMakeLists())) return false;
    if (!writeFile(simDir.filePath("main.c"), generateSimulatorMainSource())) return false;
    if (!writeFile(simDir.filePath("sdl_driver.h"), generateSimulatorSdlDriverHeader())) return false;
    if (!writeFile(simDir.filePath("sdl_driver.c"), generateSimulatorSdlDriverSource())) return false;
    if (!writeFile(simDir.filePath("README.md"), generateSimulatorReadme())) return false;

    outDir.mkdir("build");
    return true;
}

QString LvglGenerator::generateCMakeLists() {
    QString name = m_project ? m_project->projectName() : "lvgl_app";
    if (name.isEmpty()) name = "lvgl_app";

    QString code;
    code += "cmake_minimum_required(VERSION 3.20)\n";
    code += QString("project(%1 LANGUAGES C CXX)\n\n").arg(name);
    code += "set(CMAKE_C_STANDARD 99)\n";
    code += "set(CMAKE_C_STANDARD_REQUIRED ON)\n\n";

    code += "# Locate or fetch LVGL library (v8.3 LTS)\n";
    code += "if(DEFINED LVGL_PATH)\n";
    code += "    file(TO_CMAKE_PATH \"${LVGL_PATH}\" LVGL_DIR)\n";
    code += "elseif(DEFINED ENV{LVGL_PATH})\n";
    code += "    file(TO_CMAKE_PATH \"$ENV{LVGL_PATH}\" LVGL_DIR)\n";
    code += "endif()\n\n";

    code += "if(LVGL_DIR AND EXISTS \"${LVGL_DIR}\")\n";
    code += "    message(STATUS \"Using local LVGL from: ${LVGL_DIR}\")\n";
    code += "    add_subdirectory(\"${LVGL_DIR}\" lvgl)\n";
    code += "else()\n";
    code += "    message(STATUS \"LVGL_PATH not specified. Fetching LVGL v8.3.11 via FetchContent...\")\n";
    code += "    include(FetchContent)\n";
    code += "    FetchContent_Declare(\n";
    code += "        lvgl\n";
    code += "        GIT_REPOSITORY https://github.com/lvgl/lvgl.git\n";
    code += "        GIT_TAG v8.3.11\n";
    code += "        GIT_SHALLOW TRUE\n";
    code += "    )\n";
    code += "    FetchContent_MakeAvailable(lvgl)\n";
    code += "endif()\n\n";

    code += "set(APP_SOURCES\n";
    code += "    main.c\n";
    code += "    ui.c\n";
    code += ")\n\n";

    code += QString("add_executable(%1 ${APP_SOURCES})\n\n").arg(name);

    code += QString("target_include_directories(%1 PRIVATE\n").arg(name);
    code += "    \"${CMAKE_CURRENT_SOURCE_DIR}\"\n";
    code += ")\n\n";

    code += QString("target_compile_definitions(%1 PRIVATE\n").arg(name);
    code += "    LV_CONF_INCLUDE_SIMPLE\n";
    code += ")\n\n";

    code += QString("target_link_libraries(%1 PRIVATE lvgl lvgl::lvgl)\n\n").arg(name);

    code += "if(WIN32)\n";
    code += QString("    target_link_libraries(%1 PRIVATE winmm)\n").arg(name);
    code += "elseif(UNIX AND NOT APPLE)\n";
    code += QString("    target_link_libraries(%1 PRIVATE pthread m)\n").arg(name);
    code += "endif()\n";

    return code;
}

QString LvglGenerator::generateLvConf() {
    DisplayConfig cfg = m_project ? m_project->displayConfig() : DisplayConfig();
    int colorDepth = (cfg.colorDepth > 0) ? cfg.colorDepth : 16;

    QString code;
    code += "/**\n";
    code += " * Auto-generated lv_conf.h by Embedded UI Designer\n";
    code += " * Optimized configuration for LVGL v8/v9 on Embedded MCUs (STM32, ESP32, etc.)\n";
    code += " */\n";
    code += "#ifndef LV_CONF_H\n";
    code += "#define LV_CONF_H\n\n";
    code += "#include <stdint.h>\n\n";

    code += "/*====================\n";
    code += "   COLOR SETTINGS\n";
    code += " *====================*/\n";
    code += QString("#define LV_COLOR_DEPTH %1\n").arg(colorDepth);
    code += "#define LV_COLOR_16_SWAP 0\n";
    code += "#define LV_COLOR_SCREEN_TRANSP 0\n\n";

    code += "/*====================\n";
    code += "   MEMORY SETTINGS\n";
    code += " *====================*/\n";
    code += "#define LV_MEM_CUSTOM 0\n";
    code += "#define LV_MEM_SIZE (64 * 1024U)  /* 64 KB internal memory pool */\n";
    code += "#define LV_MEM_ADR 0\n";
    code += "#define LV_MEM_POOL_INCLUDE <stdint.h>\n\n";

    code += "/*====================\n";
    code += "   HAL SETTINGS\n";
    code += " *====================*/\n";
    code += "#define LV_TICK_CUSTOM 0\n";
    code += "#define LV_DPI_DEF 130\n\n";

    code += "/*====================\n";
    code += "   WIDGET USAGE\n";
    code += " *====================*/\n";
    code += QString("#define LV_USE_BTN %1\n").arg(hasButtons() ? 1 : 1);
    code += QString("#define LV_USE_LABEL %1\n").arg(hasLabels() || hasButtons() || hasCheckboxes() ? 1 : 1);
    code += QString("#define LV_USE_BAR %1\n").arg(hasProgressBars() ? 1 : 1);
    code += QString("#define LV_USE_SLIDER %1\n").arg(hasSliders() ? 1 : 1);
    code += QString("#define LV_USE_SWITCH %1\n").arg(hasSwitches() ? 1 : 1);
    code += QString("#define LV_USE_CHECKBOX %1\n").arg(hasCheckboxes() ? 1 : 1);
    code += QString("#define LV_USE_TEXTAREA %1\n").arg(hasTextInputs() ? 1 : 1);
    code += QString("#define LV_USE_IMG %1\n").arg(hasImages() ? 1 : 1);
    code += "#define LV_USE_LINE 1\n";
    code += "#define LV_USE_ARC 1\n\n";

    code += "/*====================\n";
    code += "   FONT USAGE\n";
    code += " *====================*/\n";
    code += "#define LV_FONT_MONTSERRAT_12 1\n";
    code += "#define LV_FONT_MONTSERRAT_14 1\n";
    code += "#define LV_FONT_MONTSERRAT_16 1\n";
    code += "#define LV_FONT_MONTSERRAT_18 1\n";
    code += "#define LV_FONT_MONTSERRAT_24 1\n";
    code += "#define LV_FONT_DEFAULT &lv_font_montserrat_14\n\n";

    code += "/*====================\n";
    code += "   THEME SETTINGS\n";
    code += " *====================*/\n";
    code += "#define LV_USE_THEME_DEFAULT 1\n";
    code += "#define LV_THEME_DEFAULT_DARK 1\n";
    code += "#define LV_THEME_DEFAULT_GROW 1\n";
    code += "#define LV_THEME_DEFAULT_TRANSITION_TIME 80\n\n";

    code += "#endif /* LV_CONF_H */\n";
    return code;
}

QString LvglGenerator::generateUiHeader() {
    QString code;
    code += "#ifndef _UI_LVGL_H\n";
    code += "#define _UI_LVGL_H\n\n";
    code += "#ifdef __has_include\n";
    code += "  #if __has_include(\"lvgl.h\")\n";
    code += "    #include \"lvgl.h\"\n";
    code += "  #elif __has_include(\"lvgl/lvgl.h\")\n";
    code += "    #include \"lvgl/lvgl.h\"\n";
    code += "  #endif\n";
    code += "#else\n";
    code += "  #include \"lvgl.h\"\n";
    code += "#endif\n\n";

    code += "#ifdef __cplusplus\n";
    code += "extern \"C\" {\n";
    code += "#endif\n\n";

    code += "/* Screen & Component Handles */\n";
    code += "extern lv_obj_t *ui_Screen;\n";

    if (m_scene) {
        for (auto comp : m_scene->uiComponents()) {
            QString id = comp->componentId();
            if (dynamic_cast<ButtonComponent*>(comp)) {
                code += QString("extern lv_obj_t *ui_btn_%1;\n").arg(id);
                code += QString("extern lv_obj_t *ui_btn_%1_label;\n").arg(id);
            } else if (dynamic_cast<LabelComponent*>(comp)) {
                code += QString("extern lv_obj_t *ui_lbl_%1;\n").arg(id);
            } else if (dynamic_cast<RectangleComponent*>(comp)) {
                code += QString("extern lv_obj_t *ui_rect_%1;\n").arg(id);
            } else if (dynamic_cast<ProgressBarComponent*>(comp)) {
                code += QString("extern lv_obj_t *ui_bar_%1;\n").arg(id);
            } else if (dynamic_cast<SliderComponent*>(comp)) {
                code += QString("extern lv_obj_t *ui_slider_%1;\n").arg(id);
            } else if (dynamic_cast<SwitchComponent*>(comp)) {
                code += QString("extern lv_obj_t *ui_sw_%1;\n").arg(id);
            } else if (dynamic_cast<CheckboxComponent*>(comp)) {
                code += QString("extern lv_obj_t *ui_cb_%1;\n").arg(id);
            } else if (dynamic_cast<TextInputComponent*>(comp)) {
                code += QString("extern lv_obj_t *ui_ta_%1;\n").arg(id);
            } else if (dynamic_cast<CircleComponent*>(comp)) {
                code += QString("extern lv_obj_t *ui_circle_%1;\n").arg(id);
            } else if (dynamic_cast<PathComponent*>(comp)) {
                code += QString("extern lv_obj_t *ui_path_%1;\n").arg(id);
            } else if (dynamic_cast<ImageComponent*>(comp)) {
                code += QString("extern lv_obj_t *ui_img_%1;\n").arg(id);
            }
        }
    }

    code += "\n/* Event Callback Declarations */\n";
    if (m_scene) {
        for (auto comp : m_scene->uiComponents()) {
            QString id = comp->componentId();
            if (dynamic_cast<ButtonComponent*>(comp)) {
                code += QString("void ui_event_btn_%1(lv_event_t *e);\n").arg(id);
            } else if (dynamic_cast<SwitchComponent*>(comp)) {
                code += QString("void ui_event_sw_%1(lv_event_t *e);\n").arg(id);
            } else if (dynamic_cast<CheckboxComponent*>(comp)) {
                code += QString("void ui_event_cb_%1(lv_event_t *e);\n").arg(id);
            } else if (dynamic_cast<SliderComponent*>(comp)) {
                code += QString("void ui_event_slider_%1(lv_event_t *e);\n").arg(id);
            } else if (dynamic_cast<TextInputComponent*>(comp)) {
                code += QString("void ui_event_ta_%1(lv_event_t *e);\n").arg(id);
            }
        }
    }

    code += "\n/* Main Initialization Function */\n";
    code += "void ui_init(void);\n\n";

    code += "#ifdef __cplusplus\n";
    code += "}\n";
    code += "#endif\n\n";
    code += "#endif /* _UI_LVGL_H */\n";
    return code;
}

QString LvglGenerator::generateUiSource() {
    DisplayConfig cfg = m_project ? m_project->displayConfig() : DisplayConfig();
    int scrWidth = (cfg.width > 0) ? cfg.width : 320;
    int scrHeight = (cfg.height > 0) ? cfg.height : 240;

    QString code;
    code += "#include \"ui.h\"\n";
    code += "#include <stdio.h>\n\n";

    code += "/* Screen & Component Handles */\n";
    code += "lv_obj_t *ui_Screen = NULL;\n";

    if (m_scene) {
        for (auto comp : m_scene->uiComponents()) {
            QString id = comp->componentId();
            if (dynamic_cast<ButtonComponent*>(comp)) {
                code += QString("lv_obj_t *ui_btn_%1 = NULL;\n").arg(id);
                code += QString("lv_obj_t *ui_btn_%1_label = NULL;\n").arg(id);
            } else if (dynamic_cast<LabelComponent*>(comp)) {
                code += QString("lv_obj_t *ui_lbl_%1 = NULL;\n").arg(id);
            } else if (dynamic_cast<RectangleComponent*>(comp)) {
                code += QString("lv_obj_t *ui_rect_%1 = NULL;\n").arg(id);
            } else if (dynamic_cast<ProgressBarComponent*>(comp)) {
                code += QString("lv_obj_t *ui_bar_%1 = NULL;\n").arg(id);
            } else if (dynamic_cast<SliderComponent*>(comp)) {
                code += QString("lv_obj_t *ui_slider_%1 = NULL;\n").arg(id);
            } else if (dynamic_cast<SwitchComponent*>(comp)) {
                code += QString("lv_obj_t *ui_sw_%1 = NULL;\n").arg(id);
            } else if (dynamic_cast<CheckboxComponent*>(comp)) {
                code += QString("lv_obj_t *ui_cb_%1 = NULL;\n").arg(id);
            } else if (dynamic_cast<TextInputComponent*>(comp)) {
                code += QString("lv_obj_t *ui_ta_%1 = NULL;\n").arg(id);
            } else if (dynamic_cast<CircleComponent*>(comp)) {
                code += QString("lv_obj_t *ui_circle_%1 = NULL;\n").arg(id);
            } else if (dynamic_cast<PathComponent*>(comp)) {
                code += QString("lv_obj_t *ui_path_%1 = NULL;\n").arg(id);
            } else if (dynamic_cast<ImageComponent*>(comp)) {
                code += QString("lv_obj_t *ui_img_%1 = NULL;\n").arg(id);
            }
        }
    }

    code += "\n/* =============================================================== */\n";
    code += "/* Event Callbacks; LVGL same-screen Prototype interactions are wired below. */\n";
    code += "/* =============================================================== */\n";

    if (m_scene) {
        for (auto comp : m_scene->uiComponents()) {
            QString id = comp->componentId();
            if (auto btn = dynamic_cast<ButtonComponent*>(comp)) {
                QString handler = btn->onClickedHandler().trimmed();
                code += QString("void ui_event_btn_%1(lv_event_t *e) {\n").arg(id);
                code += "    lv_event_code_t event_code = lv_event_get_code(e);\n";
                code += "    if (event_code == LV_EVENT_CLICKED) {\n";
                code += QString("        printf(\"[LVGL EVENT] Button clicked: %1 (handler: %2)\\n\");\n").arg(id, handler);
                code += "        /* TODO: Trigger GPIO, state machine transition, or hardware routine */\n";
                appendLvglInteractions(code, btn, m_scene, {"On Click"}, "        ");
                code += "    }\n";
                code += "}\n\n";
            } else if (auto sw = dynamic_cast<SwitchComponent*>(comp)) {
                QString handler = sw->onToggledHandler().trimmed();
                code += QString("void ui_event_sw_%1(lv_event_t *e) {\n").arg(id);
                code += "    lv_event_code_t event_code = lv_event_get_code(e);\n";
                code += "    lv_obj_t *target = lv_event_get_target(e);\n";
                code += "    if (event_code == LV_EVENT_VALUE_CHANGED) {\n";
                code += "        bool is_checked = lv_obj_has_state(target, LV_STATE_CHECKED);\n";
                code += QString("        printf(\"[LVGL EVENT] Switch %1 changed state: %2\\n\", is_checked ? \"ON\" : \"OFF\");\n").arg(id, "%s");
                code += "        /* TODO: Switch relay, LED, or system flag */\n";
                appendLvglInteractions(code, sw, m_scene, {"On Click", "On Change", "On Value Changed"}, "        ");
                code += "    }\n";
                code += "}\n\n";
            } else if (dynamic_cast<CheckboxComponent*>(comp)) {
                code += QString("void ui_event_cb_%1(lv_event_t *e) {\n").arg(id);
                code += "    lv_event_code_t event_code = lv_event_get_code(e);\n";
                code += "    lv_obj_t *target = lv_event_get_target(e);\n";
                code += "    if (event_code == LV_EVENT_VALUE_CHANGED) {\n";
                code += "        bool is_checked = lv_obj_has_state(target, LV_STATE_CHECKED);\n";
                code += QString("        printf(\"[LVGL EVENT] Checkbox %1 toggled: %2\\n\", is_checked ? \"CHECKED\" : \"UNCHECKED\");\n").arg(id, "%s");
                appendLvglInteractions(code, dynamic_cast<CheckboxComponent*>(comp), m_scene,
                                       {"On Click", "On Change", "On Value Changed"}, "        ");
                code += "    }\n";
                code += "}\n\n";
            } else if (dynamic_cast<SliderComponent*>(comp)) {
                code += QString("void ui_event_slider_%1(lv_event_t *e) {\n").arg(id);
                code += "    lv_event_code_t event_code = lv_event_get_code(e);\n";
                code += "    lv_obj_t *target = lv_event_get_target(e);\n";
                code += "    if (event_code == LV_EVENT_VALUE_CHANGED) {\n";
                code += "        int32_t val = (int32_t)lv_slider_get_value(target);\n";
                code += QString("        printf(\"[LVGL EVENT] Slider %1 value: %2\\n\", (int)val);\n").arg(id, "%d");
                code += "        /* TODO: Update PWM brightness, volume, or target setpoint */\n";
                appendLvglInteractions(code, dynamic_cast<SliderComponent*>(comp), m_scene,
                                       {"On Change", "On Value Changed"}, "        ");
                code += "    }\n";
                code += "    if (event_code == LV_EVENT_CLICKED) {\n";
                appendLvglInteractions(code, dynamic_cast<SliderComponent*>(comp), m_scene,
                                       {"On Click"}, "        ");
                code += "    }\n";
                code += "}\n\n";
            } else if (dynamic_cast<TextInputComponent*>(comp)) {
                code += QString("void ui_event_ta_%1(lv_event_t *e) {\n").arg(id);
                code += "    lv_event_code_t event_code = lv_event_get_code(e);\n";
                code += "    lv_obj_t *target = lv_event_get_target(e);\n";
                code += "    if (event_code == LV_EVENT_VALUE_CHANGED) {\n";
                code += "        const char *txt = lv_textarea_get_text(target);\n";
                code += QString("        printf(\"[LVGL EVENT] Text input %1 changed: %2\\n\", txt);\n").arg(id, "%s");
                appendLvglInteractions(code, dynamic_cast<TextInputComponent*>(comp), m_scene,
                                       {"On Change", "On Value Changed"}, "        ");
                code += "    }\n";
                code += "    if (event_code == LV_EVENT_CLICKED) {\n";
                appendLvglInteractions(code, dynamic_cast<TextInputComponent*>(comp), m_scene,
                                       {"On Click"}, "        ");
                code += "    }\n";
                code += "}\n\n";
            }
        }
    }

    code += "/* =============================================================== */\n";
    code += "/* Screen & Component Construction                                 */\n";
    code += "/* =============================================================== */\n";
    code += "void ui_init(void) {\n";

    // 1. Create Base Screen
    QString bgHex = (m_scene) ? m_scene->screenBackgroundColor().name().mid(1).toUpper() : "1A1C22";
    code += "    // Create Main Screen\n";
    code += "    ui_Screen = lv_obj_create(NULL);\n";
    code += "    lv_obj_remove_style_all(ui_Screen);\n";
    code += QString("    lv_obj_set_size(ui_Screen, %1, %2);\n").arg(scrWidth).arg(scrHeight);
    code += QString("    lv_obj_set_style_bg_color(ui_Screen, lv_color_hex(0x%1), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(bgHex);
    code += "    lv_obj_set_style_bg_opa(ui_Screen, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);\n";
    code += "    lv_scr_load(ui_Screen);\n\n";

    if (m_scene) {
        // 2. Rectangles (Background Panels)
        for (auto comp : m_scene->uiComponents()) {
            if (auto rect = dynamic_cast<RectangleComponent*>(comp)) {
                QString id = rect->componentId();
                int rx = static_cast<int>(rect->pos().x());
                int ry = static_cast<int>(rect->pos().y());
                int rw = static_cast<int>(rect->compWidth());
                int rh = static_cast<int>(rect->compHeight());
                int rad = rect->cornerRadius();
                QString fillHex = rect->fillColor().name().mid(1).toUpper();
                QString strokeHex = rect->strokeColor().name().mid(1).toUpper();
                int strokeW = rect->strokeWidth();

                code += QString("    // Rectangle: %1\n").arg(id);
                code += QString("    ui_rect_%1 = lv_obj_create(ui_Screen);\n").arg(id);
                code += QString("    lv_obj_remove_style_all(ui_rect_%1);\n").arg(id);
                code += QString("    lv_obj_set_pos(ui_rect_%1, %2, %3);\n").arg(id).arg(rx).arg(ry);
                code += QString("    lv_obj_set_size(ui_rect_%1, %2, %3);\n").arg(id).arg(rw).arg(rh);
                code += QString("    lv_obj_set_style_bg_color(ui_rect_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, fillHex);
                code += QString("    lv_obj_set_style_bg_opa(ui_rect_%1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id);
                if (rad > 0) {
                    code += QString("    lv_obj_set_style_radius(ui_rect_%1, %2, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id).arg(rad);
                }
                if (strokeW > 0) {
                    code += QString("    lv_obj_set_style_border_color(ui_rect_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, strokeHex);
                    code += QString("    lv_obj_set_style_border_width(ui_rect_%1, %2, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id).arg(strokeW);
                }
                code += "\n";
            }
        }

        // 3. Circles
        for (auto comp : m_scene->uiComponents()) {
            if (auto circ = dynamic_cast<CircleComponent*>(comp)) {
                QString id = circ->componentId();
                int dim = static_cast<int>(qMin(circ->compWidth(), circ->compHeight()));
                int cx = static_cast<int>(circ->pos().x());
                int cy = static_cast<int>(circ->pos().y());
                QString fillHex = circ->fillColor().name().mid(1).toUpper();
                QString strokeHex = circ->strokeColor().name().mid(1).toUpper();
                int strokeW = circ->strokeWidth();

                code += QString("    // Circle: %1\n").arg(id);
                code += QString("    ui_circle_%1 = lv_obj_create(ui_Screen);\n").arg(id);
                code += QString("    lv_obj_remove_style_all(ui_circle_%1);\n").arg(id);
                code += QString("    lv_obj_set_pos(ui_circle_%1, %2, %3);\n").arg(id).arg(cx).arg(cy);
                code += QString("    lv_obj_set_size(ui_circle_%1, %2, %2);\n").arg(id).arg(dim);
                code += QString("    lv_obj_set_style_radius(ui_circle_%1, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id);
                if (circ->isFilled()) {
                    code += QString("    lv_obj_set_style_bg_color(ui_circle_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, fillHex);
                    code += QString("    lv_obj_set_style_bg_opa(ui_circle_%1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id);
                } else {
                    code += QString("    lv_obj_set_style_bg_opa(ui_circle_%1, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id);
                }
                if (strokeW > 0) {
                    code += QString("    lv_obj_set_style_border_color(ui_circle_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, strokeHex);
                    code += QString("    lv_obj_set_style_border_width(ui_circle_%1, %2, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id).arg(strokeW);
                }
                code += "\n";
            }
        }

        // LVGL has no cubic path primitive; feed it a closed flattened polyline.
        for (auto comp : m_scene->uiComponents()) {
            if (auto path = dynamic_cast<PathComponent*>(comp)) {
                const QString id = path->componentId();
                const QPolygonF flattened = path->flattenedPoints();
                if (flattened.size() < 2) continue;
                const int pointCount = flattened.size() + 1;
                code += QString("    // Path %1 flattened to %2 polyline points\n").arg(id).arg(flattened.size());
                code += QString("    static lv_point_t ui_path_%1_points[%2] = {\n").arg(id).arg(pointCount);
                for (const QPointF& point : flattened) {
                    code += QString("        {%1, %2},\n").arg(qRound(point.x())).arg(qRound(point.y()));
                }
                code += QString("        {%1, %2}\n    };\n")
                    .arg(qRound(flattened.first().x())).arg(qRound(flattened.first().y()));
                code += QString("    ui_path_%1 = lv_line_create(ui_Screen);\n").arg(id);
                code += QString("    lv_obj_set_pos(ui_path_%1, %2, %3);\n")
                    .arg(id).arg(qRound(path->pos().x())).arg(qRound(path->pos().y()));
                code += QString("    lv_obj_set_size(ui_path_%1, %2, %3);\n")
                    .arg(id).arg(qRound(path->compWidth())).arg(qRound(path->compHeight()));
                code += QString("    lv_line_set_points(ui_path_%1, ui_path_%1_points, %2);\n").arg(id).arg(pointCount);
                code += QString("    lv_obj_set_style_line_color(ui_path_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n")
                    .arg(id, path->strokeColor().name().mid(1).toUpper());
                code += QString("    lv_obj_set_style_line_width(ui_path_%1, %2, LV_PART_MAIN | LV_STATE_DEFAULT);\n")
                    .arg(id).arg(qRound(path->strokeThickness()));
                code += QString("    lv_obj_set_style_line_opa(ui_path_%1, %2, LV_PART_MAIN | LV_STATE_DEFAULT);\n\n")
                    .arg(id).arg(path->opacityPercent() * 255 / 100);
            }
        }

        // 4. Progress Bars
        for (auto comp : m_scene->uiComponents()) {
            if (auto prog = dynamic_cast<ProgressBarComponent*>(comp)) {
                QString id = prog->componentId();
                int px = static_cast<int>(prog->pos().x());
                int py = static_cast<int>(prog->pos().y());
                int pw = static_cast<int>(prog->compWidth());
                int ph = static_cast<int>(prog->compHeight());
                int val = static_cast<int>(prog->value() * 100);
                QString barHex = prog->barColor().name().mid(1).toUpper();
                QString trackHex = prog->trackColor().name().mid(1).toUpper();
                int rad = prog->cornerRadius();

                code += QString("    // Progress Bar: %1\n").arg(id);
                code += QString("    ui_bar_%1 = lv_bar_create(ui_Screen);\n").arg(id);
                code += QString("    lv_obj_set_pos(ui_bar_%1, %2, %3);\n").arg(id).arg(px).arg(py);
                code += QString("    lv_obj_set_size(ui_bar_%1, %2, %3);\n").arg(id).arg(pw).arg(ph);
                code += QString("    lv_bar_set_range(ui_bar_%1, 0, 100);\n").arg(id);
                code += QString("    lv_bar_set_value(ui_bar_%1, %2, LV_ANIM_OFF);\n").arg(id).arg(val);
                code += QString("    lv_obj_set_style_bg_color(ui_bar_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, trackHex);
                code += QString("    lv_obj_set_style_bg_color(ui_bar_%1, lv_color_hex(0x%2), LV_PART_INDICATOR | LV_STATE_DEFAULT);\n").arg(id, barHex);
                if (rad > 0) {
                    code += QString("    lv_obj_set_style_radius(ui_bar_%1, %2, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id).arg(rad);
                    code += QString("    lv_obj_set_style_radius(ui_bar_%1, %2, LV_PART_INDICATOR | LV_STATE_DEFAULT);\n").arg(id).arg(rad);
                }
                code += "\n";
            }
        }

        // 5. Sliders
        for (auto comp : m_scene->uiComponents()) {
            if (auto slider = dynamic_cast<SliderComponent*>(comp)) {
                QString id = slider->componentId();
                int sx = static_cast<int>(slider->pos().x());
                int sy = static_cast<int>(slider->pos().y());
                int sw = static_cast<int>(slider->compWidth());
                int sh = static_cast<int>(slider->compHeight());
                QString trackHex = slider->trackColor().name().mid(1).toUpper();
                QString fillHex = slider->fillColor().name().mid(1).toUpper();
                QString knobHex = slider->handleColor().name().mid(1).toUpper();

                code += QString("    // Slider: %1\n").arg(id);
                code += QString("    ui_slider_%1 = lv_slider_create(ui_Screen);\n").arg(id);
                code += QString("    lv_obj_set_pos(ui_slider_%1, %2, %3);\n").arg(id).arg(sx).arg(sy);
                code += QString("    lv_obj_set_size(ui_slider_%1, %2, %3);\n").arg(id).arg(sw).arg(sh);
                code += QString("    lv_slider_set_range(ui_slider_%1, %2, %3);\n").arg(id).arg(slider->minimum()).arg(slider->maximum());
                code += QString("    lv_slider_set_value(ui_slider_%1, %2, LV_ANIM_OFF);\n").arg(id).arg(slider->value());
                code += QString("    lv_obj_set_style_bg_color(ui_slider_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, trackHex);
                code += QString("    lv_obj_set_style_bg_color(ui_slider_%1, lv_color_hex(0x%2), LV_PART_INDICATOR | LV_STATE_DEFAULT);\n").arg(id, fillHex);
                code += QString("    lv_obj_set_style_bg_color(ui_slider_%1, lv_color_hex(0x%2), LV_PART_KNOB | LV_STATE_DEFAULT);\n").arg(id, knobHex);
                code += QString("    lv_obj_add_event_cb(ui_slider_%1, ui_event_slider_%1, LV_EVENT_VALUE_CHANGED, NULL);\n").arg(id);
                code += QString("    lv_obj_add_event_cb(ui_slider_%1, ui_event_slider_%1, LV_EVENT_CLICKED, NULL);\n\n").arg(id);
            }
        }

        // 6. Switches
        for (auto comp : m_scene->uiComponents()) {
            if (auto sw = dynamic_cast<SwitchComponent*>(comp)) {
                QString id = sw->componentId();
                int x = static_cast<int>(sw->pos().x());
                int y = static_cast<int>(sw->pos().y());
                int w = static_cast<int>(sw->compWidth());
                int h = static_cast<int>(sw->compHeight());
                QString onHex = sw->onColor().name().mid(1).toUpper();
                QString offHex = sw->offColor().name().mid(1).toUpper();
                QString thumbHex = sw->thumbColor().name().mid(1).toUpper();

                code += QString("    // Switch: %1\n").arg(id);
                code += QString("    ui_sw_%1 = lv_switch_create(ui_Screen);\n").arg(id);
                code += QString("    lv_obj_set_pos(ui_sw_%1, %2, %3);\n").arg(id).arg(x).arg(y);
                code += QString("    lv_obj_set_size(ui_sw_%1, %2, %3);\n").arg(id).arg(w).arg(h);
                if (sw->isChecked()) {
                    code += QString("    lv_obj_add_state(ui_sw_%1, LV_STATE_CHECKED);\n").arg(id);
                }
                code += QString("    lv_obj_set_style_bg_color(ui_sw_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, offHex);
                code += QString("    lv_obj_set_style_bg_color(ui_sw_%1, lv_color_hex(0x%2), LV_PART_INDICATOR | LV_STATE_CHECKED);\n").arg(id, onHex);
                code += QString("    lv_obj_set_style_bg_color(ui_sw_%1, lv_color_hex(0x%2), LV_PART_KNOB | LV_STATE_DEFAULT);\n").arg(id, thumbHex);
                code += QString("    lv_obj_add_event_cb(ui_sw_%1, ui_event_sw_%1, LV_EVENT_VALUE_CHANGED, NULL);\n\n").arg(id);
            }
        }

        // 7. Checkboxes
        for (auto comp : m_scene->uiComponents()) {
            if (auto chk = dynamic_cast<CheckboxComponent*>(comp)) {
                QString id = chk->componentId();
                int x = static_cast<int>(chk->pos().x());
                int y = static_cast<int>(chk->pos().y());
                QString textHex = chk->textColor().name().mid(1).toUpper();
                QString checkHex = chk->checkColor().name().mid(1).toUpper();

                code += QString("    // Checkbox: %1\n").arg(id);
                code += QString("    ui_cb_%1 = lv_checkbox_create(ui_Screen);\n").arg(id);
                code += QString("    lv_obj_set_pos(ui_cb_%1, %2, %3);\n").arg(id).arg(x).arg(y);
                code += QString("    lv_checkbox_set_text(ui_cb_%1, \"%2\");\n").arg(id, chk->text());
                if (chk->isChecked()) {
                    code += QString("    lv_obj_add_state(ui_cb_%1, LV_STATE_CHECKED);\n").arg(id);
                }
                code += QString("    lv_obj_set_style_text_color(ui_cb_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, textHex);
                code += QString("    lv_obj_set_style_bg_color(ui_cb_%1, lv_color_hex(0x%2), LV_PART_INDICATOR | LV_STATE_CHECKED);\n").arg(id, checkHex);
                code += QString("    lv_obj_add_event_cb(ui_cb_%1, ui_event_cb_%1, LV_EVENT_VALUE_CHANGED, NULL);\n\n").arg(id);
            }
        }

        // 8. TextInputs
        for (auto comp : m_scene->uiComponents()) {
            if (auto ta = dynamic_cast<TextInputComponent*>(comp)) {
                QString id = ta->componentId();
                int x = static_cast<int>(ta->pos().x());
                int y = static_cast<int>(ta->pos().y());
                int w = static_cast<int>(ta->compWidth());
                int h = static_cast<int>(ta->compHeight());
                QString textHex = ta->textColor().name().mid(1).toUpper();
                QString bgHex = ta->backgroundColor().name().mid(1).toUpper();
                QString borderHex = ta->borderColor().name().mid(1).toUpper();

                code += QString("    // TextInput: %1\n").arg(id);
                code += QString("    ui_ta_%1 = lv_textarea_create(ui_Screen);\n").arg(id);
                code += QString("    lv_obj_set_pos(ui_ta_%1, %2, %3);\n").arg(id).arg(x).arg(y);
                code += QString("    lv_obj_set_size(ui_ta_%1, %2, %3);\n").arg(id).arg(w).arg(h);
                code += QString("    lv_textarea_set_placeholder_text(ui_ta_%1, \"%2\");\n").arg(id, ta->placeholder());
                if (!ta->text().isEmpty()) {
                    code += QString("    lv_textarea_set_text(ui_ta_%1, \"%2\");\n").arg(id, ta->text());
                }
                code += QString("    lv_textarea_set_one_line(ui_ta_%1, true);\n").arg(id);
                code += QString("    lv_obj_set_style_bg_color(ui_ta_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, bgHex);
                code += QString("    lv_obj_set_style_text_color(ui_ta_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, textHex);
                code += QString("    lv_obj_set_style_border_color(ui_ta_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, borderHex);
                code += QString("    lv_obj_set_style_border_width(ui_ta_%1, %2, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id).arg(ta->borderWidth());
                code += QString("    lv_obj_set_style_radius(ui_ta_%1, %2, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id).arg(ta->cornerRadius());
                code += QString("    lv_obj_add_event_cb(ui_ta_%1, ui_event_ta_%1, LV_EVENT_VALUE_CHANGED, NULL);\n").arg(id);
                code += QString("    lv_obj_add_event_cb(ui_ta_%1, ui_event_ta_%1, LV_EVENT_CLICKED, NULL);\n\n").arg(id);
            }
        }

        // 9. Labels
        for (auto comp : m_scene->uiComponents()) {
            if (auto lbl = dynamic_cast<LabelComponent*>(comp)) {
                QString id = lbl->componentId();
                int lx = static_cast<int>(lbl->pos().x());
                int ly = static_cast<int>(lbl->pos().y());
                int lw = static_cast<int>(lbl->compWidth());
                int lh = static_cast<int>(lbl->compHeight());
                QString textHex = lbl->color().name().mid(1).toUpper();

                code += QString("    // Label: %1\n").arg(id);
                code += QString("    ui_lbl_%1 = lv_label_create(ui_Screen);\n").arg(id);
                code += QString("    lv_obj_set_pos(ui_lbl_%1, %2, %3);\n").arg(id).arg(lx).arg(ly);
                code += QString("    lv_obj_set_size(ui_lbl_%1, %2, %3);\n").arg(id).arg(lw).arg(lh);
                code += QString("    lv_label_set_text(ui_lbl_%1, \"%2\");\n").arg(id, lbl->text());
                code += QString("    lv_obj_set_style_text_color(ui_lbl_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, textHex);

                if (lbl->pixelSize() >= 24) {
                    code += QString("    lv_obj_set_style_text_font(ui_lbl_%1, &lv_font_montserrat_24, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id);
                } else if (lbl->pixelSize() >= 18) {
                    code += QString("    lv_obj_set_style_text_font(ui_lbl_%1, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id);
                } else if (lbl->pixelSize() >= 16) {
                    code += QString("    lv_obj_set_style_text_font(ui_lbl_%1, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id);
                } else {
                    code += QString("    lv_obj_set_style_text_font(ui_lbl_%1, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id);
                }

                if (lbl->alignment().testFlag(Qt::AlignHCenter)) {
                    code += QString("    lv_obj_set_style_text_align(ui_lbl_%1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id);
                } else if (lbl->alignment().testFlag(Qt::AlignRight)) {
                    code += QString("    lv_obj_set_style_text_align(ui_lbl_%1, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id);
                }
                // Task 5: Letter spacing and line height
                if (lbl->letterSpacing() != 0.0) {
                    code += QString("    lv_obj_set_style_text_letter_space(ui_lbl_%1, %2, LV_PART_MAIN | LV_STATE_DEFAULT);\n")
                            .arg(id).arg(static_cast<int>(lbl->letterSpacing()));
                }
                if (lbl->lineHeight() > 0) {
                    int lineSpace = static_cast<int>(lbl->pixelSize() * (lbl->lineHeight() / 100.0 - 1.0));
                    if (lineSpace > 0)
                        code += QString("    lv_obj_set_style_text_line_space(ui_lbl_%1, %2, LV_PART_MAIN | LV_STATE_DEFAULT);\n")
                                .arg(id).arg(lineSpace);
                }
                code += "\n";
            }
        }

        // 10. Buttons
        for (auto comp : m_scene->uiComponents()) {
            if (auto btn = dynamic_cast<ButtonComponent*>(comp)) {
                QString id = btn->componentId();
                int bx = static_cast<int>(btn->pos().x());
                int by = static_cast<int>(btn->pos().y());
                int bw = static_cast<int>(btn->compWidth());
                int bh = static_cast<int>(btn->compHeight());
                QString bgHex = btn->backgroundColor().name().mid(1).toUpper();
                QString textHex = btn->textColor().name().mid(1).toUpper();
                int rad = btn->cornerRadius();

                code += QString("    // Button: %1\n").arg(id);
                code += QString("    ui_btn_%1 = lv_btn_create(ui_Screen);\n").arg(id);
                code += QString("    lv_obj_set_pos(ui_btn_%1, %2, %3);\n").arg(id).arg(bx).arg(by);
                code += QString("    lv_obj_set_size(ui_btn_%1, %2, %3);\n").arg(id).arg(bw).arg(bh);
                code += QString("    lv_obj_set_style_bg_color(ui_btn_%1, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, bgHex);
                if (rad > 0) {
                    code += QString("    lv_obj_set_style_radius(ui_btn_%1, %2, LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id).arg(rad);
                }
                code += QString("    lv_obj_add_event_cb(ui_btn_%1, ui_event_btn_%1, LV_EVENT_CLICKED, NULL);\n").arg(id);

                code += QString("    ui_btn_%1_label = lv_label_create(ui_btn_%1);\n").arg(id);
                code += QString("    lv_label_set_text(ui_btn_%1_label, \"%2\");\n").arg(id, btn->text());
                code += QString("    lv_obj_set_style_text_color(ui_btn_%1_label, lv_color_hex(0x%2), LV_PART_MAIN | LV_STATE_DEFAULT);\n").arg(id, textHex);
                code += QString("    lv_obj_center(ui_btn_%1_label);\n\n").arg(id);
            }
        }
    }

    code += "}\n";
    return code;
}

QString LvglGenerator::generateMainSource() {
    DisplayConfig cfg = m_project ? m_project->displayConfig() : DisplayConfig();
    int scrWidth = (cfg.width > 0) ? cfg.width : 320;
    int scrHeight = (cfg.height > 0) ? cfg.height : 240;

    QString code;
    code += "/**\n";
    code += " * Main Desktop Simulator / Hardware Entry Point for LVGL\n";
    code += " * Auto-generated by Embedded UI Designer\n";
    code += " */\n";
    code += "#define _DEFAULT_SOURCE\n";
    code += "#define _XOPEN_SOURCE 700\n\n";
    code += "#ifdef __has_include\n";
    code += "  #if __has_include(\"lvgl.h\")\n";
    code += "    #include \"lvgl.h\"\n";
    code += "  #elif __has_include(\"lvgl/lvgl.h\")\n";
    code += "    #include \"lvgl/lvgl.h\"\n";
    code += "  #endif\n";
    code += "#else\n";
    code += "  #include \"lvgl.h\"\n";
    code += "#endif\n";
    code += "#include \"ui.h\"\n";
    code += "#include <stdio.h>\n";
    code += "#include <stdlib.h>\n\n";

    code += "#if defined(_WIN32)\n";
    code += "#include <windows.h>\n";
    code += "#define sleep_ms(x) Sleep(x)\n";
    code += "#else\n";
    code += "#include <unistd.h>\n";
    code += "#define sleep_ms(x) usleep((x) * 1000)\n";
    code += "#endif\n\n";

    code += QString("#define DISP_HOR_RES %1\n").arg(scrWidth);
    code += QString("#define DISP_VER_RES %2\n").arg(scrHeight);
    code += "#define DISP_BUF_SIZE (DISP_HOR_RES * 40)\n\n";

    code += "static lv_color_t buf_1[DISP_BUF_SIZE];\n";
    code += "static lv_disp_draw_buf_t disp_buf;\n";
    code += "static lv_disp_drv_t disp_drv;\n\n";

    code += "/* Display flush callback: sends rendered visual scanlines to display */\n";
    code += "static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {\n";
    code += "    (void)area;\n";
    code += "    (void)color_p;\n";
    code += "    /* Hardware Implementation: transmit pixel buffer via SPI / I2C / parallel bus */\n";
    code += "    lv_disp_flush_ready(drv);\n";
    code += "}\n\n";

    code += "int main(int argc, char *argv[]) {\n";
    code += "    (void)argc;\n";
    code += "    (void)argv;\n\n";
    code += "    printf(\"Initializing LVGL UI runtime...\\n\");\n";
    code += "    lv_init();\n\n";
    code += "    // Initialize display buffer\n";
    code += "    lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, DISP_BUF_SIZE);\n\n";
    code += "    // Register display driver\n";
    code += "    lv_disp_drv_init(&disp_drv);\n";
    code += "    disp_drv.hor_res = DISP_HOR_RES;\n";
    code += "    disp_drv.ver_res = DISP_VER_RES;\n";
    code += "    disp_drv.flush_cb = disp_flush;\n";
    code += "    disp_drv.draw_buf = &disp_buf;\n";
    code += "    lv_disp_drv_register(&disp_drv);\n\n";
    code += "    // Build UI hierarchy created in Designer\n";
    code += "    printf(\"Loading UI components...\\n\");\n";
    code += "    ui_init();\n\n";
    code += "    printf(\"LVGL initialized successfully. Dispatch loop running...\\n\");\n";
    code += "    for (int i = 0; i < 50; ++i) {\n";
    code += "        lv_timer_handler();\n";
    code += "        sleep_ms(5);\n";
    code += "    }\n\n";
    code += "    printf(\"Demo completed successfully.\\n\");\n";
    code += "    return 0;\n";
    code += "}\n";

    return code;
}

QString LvglGenerator::generateIdfComponentYml() {
    QString yaml;
    yaml += "## ESP-IDF Component Manager Manifest\n";
    yaml += "## Automatically pulls official LVGL v8/v9 component for ESP32 builds\n";
    yaml += "dependencies:\n";
    yaml += "  lvgl/lvgl: \"^8.3.11\"\n";
    return yaml;
}

QString LvglGenerator::generatePlatformioIni() {
    QString ini;
    ini += "; PlatformIO Project Configuration for ESP32 + LVGL\n";
    ini += "[env:esp32dev]\n";
    ini += "platform = espressif32\n";
    ini += "board = esp32dev\n";
    ini += "framework = arduino\n";
    ini += "monitor_speed = 115200\n";
    ini += "lib_deps =\n";
    ini += "    lvgl/lvgl@^8.3.11\n";
    ini += "build_flags =\n";
    ini += "    -I.\n";
    ini += "    -DLV_CONF_INCLUDE_SIMPLE\n";
    return ini;
}

QString LvglGenerator::generateReadme() {
    DisplayConfig cfg = m_project ? m_project->displayConfig() : DisplayConfig();
    QString name = m_project ? m_project->projectName() : "lvgl_app";

    QString md;
    md += QString("# %1 (LVGL C Project)\n\n").arg(name);
    md += "Auto-generated by **Embedded UI Designer** for **LVGL (v8 / v9)**.\n\n";
    md += "## Specifications\n";
    md += QString("- **Target Display**: %1 × %2 (%3-bit)\n").arg(cfg.width).arg(cfg.height).arg(cfg.colorDepth);
    md += "- **Library**: LVGL v8.3 LTS (Light and Versatile Graphics Library)\n";
    md += "- **Target Platforms**: ESP32, STM32, Raspberry Pi Pico, NXP, Linux / Windows Simulator\n\n";
    md += "## Project Structure\n";
    md += "```\n";
    md += "├── CMakeLists.txt        # PC Simulator / standalone build script\n";
    md += "├── lv_conf.h            # Tuned LVGL configuration header (memory, color depth, fonts)\n";
    md += "├── ui.h                 # Component handles & event declarations\n";
    md += "├── ui.c                 # Visual layout, widget instantiation & styles\n";
    md += "├── main.c               # Standalone entry point & tick dispatch loop\n";
    md += "├── idf_component.yml    # ESP-IDF component registry manifest for ESP32\n";
    md += "└── platformio.ini       # PlatformIO configuration for Arduino / ESP32\n";
    md += "```\n\n";
    md += "## 1. Running on PC (CMake Simulator)\n\n";
    md += "If you have a local copy of LVGL:\n";
    md += "```bash\n";
    md += "mkdir build && cd build\n";
    md += "cmake .. -DLVGL_PATH=/path/to/lvgl\n";
    md += "cmake --build .\n";
    md += "```\n\n";
    md += "If `LVGL_PATH` is not set, CMake will automatically download LVGL v8.3.11 via `FetchContent`.\n\n";
    md += "## 2. Using with ESP32 (ESP-IDF)\n\n";
    md += "1. Copy `ui.c`, `ui.h`, and `lv_conf.h` into your project's `main/` directory.\n";
    md += "2. In `main/main.c`, add:\n";
    md += "   ```c\n";
    md += "   #include \"ui.h\"\n";
    md += "   // After initializing your display driver (e.g. esp_lcd or LovyanGFX):\n";
    md += "   ui_init();\n";
    md += "   ```\n";
    md += "3. The included `idf_component.yml` automatically registers the official LVGL component with `idf.py build`.\n\n";
    md += "## 3. Using with STM32 (STM32CubeIDE)\n\n";
    md += "1. Copy `ui.c`, `ui.h`, and `lv_conf.h` into `Core/Src` and `Core/Inc`.\n";
    md += "2. In your display refresh task or `main()` after LCD init:\n";
    md += "   ```c\n";
    md += "   lv_init();\n";
    md += "   // register your DMA/SPI display flush callback\n";
    md += "   ui_init();\n";
    md += "   ```\n";
    md += "3. Implement your hardware actions inside the event stubs in `ui.c` (e.g., `ui_event_btn_*`).\n";
    return md;
}

QString LvglGenerator::generateSimulatorCMakeLists() {
    QString name = m_project ? m_project->projectName() : "lvgl_app";
    if (name.isEmpty()) name = "lvgl_app";
    int width = m_project ? m_project->displayConfig().width : 320;
    int height = m_project ? m_project->displayConfig().height : 240;
    int depth = m_project ? m_project->displayConfig().colorDepth : 16;
    int isRound = (m_project && m_project->displayConfig().round) ? 1 : 0;

    QString code;
    code += "cmake_minimum_required(VERSION 3.20)\n";
    code += QString("project(%1_pc_simulator LANGUAGES C CXX)\n\n").arg(name);
    code += "set(CMAKE_C_STANDARD 99)\n";
    code += "set(CMAKE_C_STANDARD_REQUIRED ON)\n";
    code += "set(CMAKE_CXX_STANDARD 17)\n";
    code += "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";

    code += "# ── Locate SDL2 for desktop window & input driver ─────────────────────────\n";
    code += "find_package(SDL2 QUIET)\n";
    code += "if(NOT SDL2_FOUND)\n";
    code += "    find_package(PkgConfig QUIET)\n";
    code += "    if(PKG_CONFIG_FOUND)\n";
    code += "        pkg_check_modules(SDL2 QUIET sdl2)\n";
    code += "    endif()\n";
    code += "endif()\n\n";

    code += "if(NOT SDL2_FOUND AND NOT TARGET SDL2::SDL2)\n";
    code += "    message(STATUS \"SDL2 not found locally. Fetching SDL2 via FetchContent...\")\n";
    code += "    include(FetchContent)\n";
    code += "    FetchContent_Declare(\n";
    code += "        SDL2\n";
    code += "        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git\n";
    code += "        GIT_TAG release-2.30.2\n";
    code += "        GIT_SHALLOW TRUE\n";
    code += "    )\n";
    code += "    FetchContent_MakeAvailable(SDL2)\n";
    code += "endif()\n\n";

    code += "# ── Locate LVGL Library (v8.3 LTS) ────────────────────────────────────────\n";
    code += "if(DEFINED LVGL_PATH)\n";
    code += "    file(TO_CMAKE_PATH \"${LVGL_PATH}\" LVGL_DIR)\n";
    code += "elseif(DEFINED ENV{LVGL_PATH})\n";
    code += "    file(TO_CMAKE_PATH \"$ENV{LVGL_PATH}\" LVGL_DIR)\n";
    code += "elseif(EXISTS \"${CMAKE_CURRENT_SOURCE_DIR}/../build/_deps/lvgl-src\")\n";
    code += "    set(LVGL_DIR \"${CMAKE_CURRENT_SOURCE_DIR}/../build/_deps/lvgl-src\")\n";
    code += "endif()\n\n";

    code += "if(LVGL_DIR AND EXISTS \"${LVGL_DIR}\")\n";
    code += "    message(STATUS \"Using local LVGL from: ${LVGL_DIR}\")\n";
    code += "    add_subdirectory(\"${LVGL_DIR}\" lvgl)\n";
    code += "else()\n";
    code += "    message(STATUS \"Fetching LVGL v8.3.11 via FetchContent...\")\n";
    code += "    include(FetchContent)\n";
    code += "    FetchContent_Declare(\n";
    code += "        lvgl\n";
    code += "        GIT_REPOSITORY https://github.com/lvgl/lvgl.git\n";
    code += "        GIT_TAG v8.3.11\n";
    code += "        GIT_SHALLOW TRUE\n";
    code += "    )\n";
    code += "    FetchContent_MakeAvailable(lvgl)\n";
    code += "endif()\n\n";

    code += "set(SIMULATOR_SOURCES\n";
    code += "    main.c\n";
    code += "    sdl_driver.c\n";
    code += "    ../ui.c\n";
    code += ")\n\n";

    code += QString("add_executable(%1_pc_simulator ${SIMULATOR_SOURCES})\n\n").arg(name);

    code += QString("target_include_directories(%1_pc_simulator PRIVATE\n").arg(name);
    code += "    \"${CMAKE_CURRENT_SOURCE_DIR}\"\n";
    code += "    \"${CMAKE_CURRENT_SOURCE_DIR}/..\"\n";
    code += ")\n\n";

    code += QString("target_compile_definitions(%1_pc_simulator PRIVATE\n").arg(name);
    code += "    LV_CONF_INCLUDE_SIMPLE\n";
    code += QString("    SIMULATOR_WIDTH=%1\n").arg(width);
    code += QString("    SIMULATOR_HEIGHT=%1\n").arg(height);
    code += QString("    SIMULATOR_COLOR_DEPTH=%1\n").arg(depth);
    code += QString("    SIMULATOR_IS_ROUND=%1\n").arg(isRound);
    code += ")\n\n";

    code += "if(TARGET SDL2::SDL2)\n";
    code += QString("    target_link_libraries(%1_pc_simulator PRIVATE SDL2::SDL2)\n").arg(name);
    code += "elseif(TARGET SDL2)\n";
    code += QString("    target_link_libraries(%1_pc_simulator PRIVATE SDL2)\n").arg(name);
    code += "else()\n";
    code += QString("    target_link_libraries(%1_pc_simulator PRIVATE ${SDL2_LIBRARIES})\n").arg(name);
    code += "    if(SDL2_INCLUDE_DIRS)\n";
    code += QString("        target_include_directories(%1_pc_simulator PRIVATE ${SDL2_INCLUDE_DIRS})\n").arg(name);
    code += "    endif()\n";
    code += "endif()\n\n";

    code += QString("target_link_libraries(%1_pc_simulator PRIVATE lvgl lvgl::lvgl m)\n\n").arg(name);

    code += "if(WIN32)\n";
    code += QString("    target_link_libraries(%1_pc_simulator PRIVATE winmm)\n").arg(name);
    code += "elseif(UNIX AND NOT APPLE)\n";
    code += QString("    target_link_libraries(%1_pc_simulator PRIVATE pthread)\n").arg(name);
    code += "endif()\n";

    return code;
}

QString LvglGenerator::generateSimulatorMainSource() {
    QString name = m_project ? m_project->projectName() : "LVGL Application";
    if (name.isEmpty()) name = "LVGL Application";
    int width = m_project ? m_project->displayConfig().width : 320;
    int height = m_project ? m_project->displayConfig().height : 240;
    int depth = m_project ? m_project->displayConfig().colorDepth : 16;
    bool isRound = m_project ? m_project->displayConfig().round : false;

    QString code;
    code += "/**\n";
    code += " * LVGL PC Simulator Entry Point\n";
    code += " * Auto-generated by Embedded UI Designer\n";
    code += QString(" * Target Display: %1x%2, %3-bit color, %4\n").arg(width).arg(height).arg(depth).arg(isRound ? "Round Shape" : "Rectangular Shape");
    code += " */\n";
    code += "#define _DEFAULT_SOURCE\n";
    code += "#define _XOPEN_SOURCE 700\n\n";
    code += "#ifdef __has_include\n";
    code += "  #if __has_include(\"lvgl.h\")\n";
    code += "    #include \"lvgl.h\"\n";
    code += "  #elif __has_include(\"lvgl/lvgl.h\")\n";
    code += "    #include \"lvgl/lvgl.h\"\n";
    code += "  #endif\n";
    code += "#else\n";
    code += "  #include \"lvgl.h\"\n";
    code += "#endif\n";
    code += "#include \"sdl_driver.h\"\n";
    code += "#include \"ui.h\"\n";
    code += "#include <stdio.h>\n";
    code += "#include <stdlib.h>\n\n";

    code += "#if defined(_WIN32)\n";
    code += "#include <windows.h>\n";
    code += "#define sleep_ms(x) Sleep(x)\n";
    code += "#else\n";
    code += "#include <unistd.h>\n";
    code += "#define sleep_ms(x) usleep((x) * 1000)\n";
    code += "#endif\n\n";

    code += QString("#ifndef SIMULATOR_WIDTH\n#define SIMULATOR_WIDTH %1\n#endif\n").arg(width);
    code += QString("#ifndef SIMULATOR_HEIGHT\n#define SIMULATOR_HEIGHT %1\n#endif\n").arg(height);
    code += QString("#ifndef SIMULATOR_COLOR_DEPTH\n#define SIMULATOR_COLOR_DEPTH %1\n#endif\n").arg(depth);
    code += QString("#ifndef SIMULATOR_IS_ROUND\n#define SIMULATOR_IS_ROUND %1\n#endif\n\n").arg(isRound ? 1 : 0);

    code += "int main(int argc, char* argv[]) {\n";
    code += "    (void)argc;\n";
    code += "    (void)argv;\n\n";
    code += "    printf(\"============================================================\\n\");\n";
    code += QString("    printf(\" Starting %1 PC Simulator (LVGL)\\n\");\n").arg(name);
    code += "    printf(\" Display: %d x %d (%s), %d-bit color depth\\n\",\n";
    code += "           SIMULATOR_WIDTH, SIMULATOR_HEIGHT,\n";
    code += "           SIMULATOR_IS_ROUND ? \"Round Display\" : \"Rectangular Display\",\n";
    code += "           SIMULATOR_COLOR_DEPTH);\n";
    code += "    printf(\"============================================================\\n\");\n\n";

    code += "    lv_init();\n\n";
    code += QString("    if (!sdl_driver_init(\"%1 - LVGL PC Simulator\",\n").arg(name);
    code += "                         SIMULATOR_WIDTH, SIMULATOR_HEIGHT,\n";
    code += "                         SIMULATOR_COLOR_DEPTH, SIMULATOR_IS_ROUND)) {\n";
    code += "        fprintf(stderr, \"Error: Failed to initialize SDL2 display driver!\\n\");\n";
    code += "        return 1;\n";
    code += "    }\n\n";

    code += "    printf(\"Building UI components...\\n\");\n";
    code += "    ui_init();\n\n";

    code += "    printf(\"Simulator running. Click/drag on window to interact; close window or press ESC to exit.\\n\");\n";
    code += "    while (sdl_driver_handle_events()) {\n";
    code += "        lv_timer_handler();\n";
    code += "        sleep_ms(5);\n";
    code += "        lv_tick_inc(5);\n";
    code += "    }\n\n";

    code += "    sdl_driver_cleanup();\n";
    code += "    printf(\"LVGL Simulator terminated cleanly.\\n\");\n";
    code += "    return 0;\n";
    code += "}\n";

    return code;
}

QString LvglGenerator::generateSimulatorSdlDriverHeader() {
    QString code;
    code += "#ifndef SDL_DRIVER_H\n";
    code += "#define SDL_DRIVER_H\n\n";
    code += "#include <stdbool.h>\n";
    code += "#include <stdint.h>\n\n";
    code += "#ifdef __cplusplus\n";
    code += "extern \"C\" {\n";
    code += "#endif\n\n";
    code += "bool sdl_driver_init(const char* title, int width, int height, int color_depth, bool is_round);\n";
    code += "bool sdl_driver_handle_events(void);\n";
    code += "void sdl_driver_cleanup(void);\n\n";
    code += "#ifdef __cplusplus\n";
    code += "}\n";
    code += "#endif\n\n";
    code += "#endif /* SDL_DRIVER_H */\n";
    return code;
}

QString LvglGenerator::generateSimulatorSdlDriverSource() {
    QString code;
    code += "/**\n";
    code += " * SDL2 Display & Mouse Input Driver for LVGL PC Simulator\n";
    code += " * Auto-generated by Embedded UI Designer\n";
    code += " */\n";
    code += "#include \"sdl_driver.h\"\n";
    code += "#include <SDL2/SDL.h>\n";
    code += "#include <stdio.h>\n";
    code += "#include <stdlib.h>\n";
    code += "#include <stdbool.h>\n";
    code += "#include <math.h>\n\n";

    code += "#ifdef __has_include\n";
    code += "  #if __has_include(\"lvgl.h\")\n";
    code += "    #include \"lvgl.h\"\n";
    code += "  #elif __has_include(\"lvgl/lvgl.h\")\n";
    code += "    #include \"lvgl/lvgl.h\"\n";
    code += "  #endif\n";
    code += "#else\n";
    code += "  #include \"lvgl.h\"\n";
    code += "#endif\n\n";

    code += "static SDL_Window* window = NULL;\n";
    code += "static SDL_Renderer* renderer = NULL;\n";
    code += "static SDL_Texture* texture = NULL;\n";
    code += "static int sim_width = 320;\n";
    code += "static int sim_height = 240;\n";
    code += "static int sim_depth = 16;\n";
    code += "static bool sim_round = false;\n";
    code += "static bool mouse_pressed = false;\n";
    code += "static int mouse_x = 0;\n";
    code += "static int mouse_y = 0;\n\n";

    code += "static lv_color_t* disp_buf_data = NULL;\n";
    code += "static lv_disp_draw_buf_t draw_buf;\n";
    code += "static lv_disp_drv_t disp_drv;\n";
    code += "static lv_indev_drv_t indev_drv;\n\n";

    code += "static void sdl_mouse_read(lv_indev_drv_t* drv, lv_indev_data_t* data) {\n";
    code += "    (void)drv;\n";
    code += "    data->point.x = mouse_x;\n";
    code += "    data->point.y = mouse_y;\n";
    code += "    data->state = mouse_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;\n";
    code += "}\n\n";

    code += "static void check_save_screenshot(void) {\n";
    code += "    const char* env_path = getenv(\"SIMULATOR_SCREENSHOT_PATH\");\n";
    code += "    if (!env_path || env_path[0] == '\\0') return;\n";
    code += "    static int frame_count = 0;\n";
    code += "    frame_count++;\n";
    code += "    if (frame_count == 5) {\n";
    code += "        SDL_Surface* sshot = SDL_CreateRGBSurfaceWithFormat(0, sim_width, sim_height, 32, SDL_PIXELFORMAT_ARGB8888);\n";
    code += "        if (sshot) {\n";
    code += "            SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);\n";
    code += "            SDL_SaveBMP(sshot, env_path);\n";
    code += "            SDL_FreeSurface(sshot);\n";
    code += "            printf(\"[LVGL PC Simulator] Saved simulator window screenshot to %s\\n\", env_path);\n";
    code += "        }\n";
    code += "    }\n";
    code += "}\n\n";

    code += "static void sdl_display_flush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {\n";
    code += "    int32_t w = area->x2 - area->x1 + 1;\n";
    code += "    int32_t h = area->y2 - area->y1 + 1;\n\n";
    code += "    SDL_Rect rect;\n";
    code += "    rect.x = area->x1;\n";
    code += "    rect.y = area->y1;\n";
    code += "    rect.w = w;\n";
    code += "    rect.h = h;\n\n";
    code += "    SDL_UpdateTexture(texture, &rect, color_p, w * sizeof(lv_color_t));\n\n";
    code += "    if (lv_disp_flush_is_last(drv)) {\n";
    code += "        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);\n";
    code += "        SDL_RenderClear(renderer);\n";
    code += "        SDL_RenderCopy(renderer, texture, NULL, NULL);\n\n";
    code += "        // If display is circular, render a bezel mask outside the active radius\n";
    code += "        if (sim_round) {\n";
    code += "            int cx = sim_width / 2;\n";
    code += "            int cy = sim_height / 2;\n";
    code += "            int r = (sim_width < sim_height ? sim_width : sim_height) / 2;\n";
    code += "            int r2 = r * r;\n\n";
    code += "            SDL_SetRenderDrawColor(renderer, 18, 18, 24, 255);\n";
    code += "            for (int y = 0; y < sim_height; y++) {\n";
    code += "                int dy = y - cy;\n";
    code += "                int dy2 = dy * dy;\n";
    code += "                for (int x = 0; x < sim_width; x++) {\n";
    code += "                    int dx = x - cx;\n";
    code += "                    if (dx * dx + dy2 > r2) {\n";
    code += "                        SDL_RenderDrawPoint(renderer, x, y);\n";
    code += "                    }\n";
    code += "                }\n";
    code += "            }\n\n";
    code += "            // Distinct circular bezel ring\n";
    code += "            SDL_SetRenderDrawColor(renderer, 55, 65, 80, 255);\n";
    code += "            for (int angle = 0; angle < 360; angle++) {\n";
    code += "                double rad = angle * 3.141592653589793 / 180.0;\n";
    code += "                int px = cx + (int)((r - 1) * cos(rad));\n";
    code += "                int py = cy + (int)((r - 1) * sin(rad));\n";
    code += "                SDL_RenderDrawPoint(renderer, px, py);\n";
    code += "            }\n";
    code += "        }\n\n";
    code += "        check_save_screenshot();\n";
    code += "        SDL_RenderPresent(renderer);\n";
    code += "    }\n";
    code += "    lv_disp_flush_ready(drv);\n";
    code += "}\n\n";

    code += "bool sdl_driver_init(const char* title, int width, int height, int color_depth, bool is_round) {\n";
    code += "    sim_width = width;\n";
    code += "    sim_height = height;\n";
    code += "    sim_depth = color_depth;\n";
    code += "    sim_round = is_round;\n\n";
    code += "    if (SDL_Init(SDL_INIT_VIDEO) != 0) {\n";
    code += "        fprintf(stderr, \"SDL_Init Error: %s\\n\", SDL_GetError());\n";
    code += "        return false;\n";
    code += "    }\n\n";
    code += "    window = SDL_CreateWindow(title ? title : \"LVGL PC Simulator\",\n";
    code += "                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,\n";
    code += "                              width, height, SDL_WINDOW_SHOWN);\n";
    code += "    if (!window) {\n";
    code += "        fprintf(stderr, \"SDL_CreateWindow Error: %s\\n\", SDL_GetError());\n";
    code += "        return false;\n";
    code += "    }\n\n";
    code += "    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);\n";
    code += "    if (!renderer) {\n";
    code += "        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);\n";
    code += "    }\n";
    code += "    if (!renderer) {\n";
    code += "        fprintf(stderr, \"SDL_CreateRenderer Error: %s\\n\", SDL_GetError());\n";
    code += "        return false;\n";
    code += "    }\n\n";
    code += "    Uint32 pixel_format = (color_depth == 16) ? SDL_PIXELFORMAT_RGB565 : SDL_PIXELFORMAT_ARGB8888;\n";
    code += "    texture = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, width, height);\n";
    code += "    if (!texture) {\n";
    code += "        fprintf(stderr, \"SDL_CreateTexture Error: %s\\n\", SDL_GetError());\n";
    code += "        return false;\n";
    code += "    }\n\n";
    code += "    size_t buf_size = width * height;\n";
    code += "    disp_buf_data = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));\n";
    code += "    if (!disp_buf_data) {\n";
    code += "        fprintf(stderr, \"Failed to allocate display buffer\\n\");\n";
    code += "        return false;\n";
    code += "    }\n\n";
    code += "    lv_disp_draw_buf_init(&draw_buf, disp_buf_data, NULL, buf_size);\n\n";
    code += "    lv_disp_drv_init(&disp_drv);\n";
    code += "    disp_drv.hor_res = width;\n";
    code += "    disp_drv.ver_res = height;\n";
    code += "    disp_drv.flush_cb = sdl_display_flush;\n";
    code += "    disp_drv.draw_buf = &draw_buf;\n";
    code += "    lv_disp_drv_register(&disp_drv);\n\n";
    code += "    lv_indev_drv_init(&indev_drv);\n";
    code += "    indev_drv.type = LV_INDEV_TYPE_POINTER;\n";
    code += "    indev_drv.read_cb = sdl_mouse_read;\n";
    code += "    lv_indev_drv_register(&indev_drv);\n\n";
    code += "    return true;\n";
    code += "}\n\n";

    code += "bool sdl_driver_handle_events(void) {\n";
    code += "    SDL_Event event;\n";
    code += "    while (SDL_PollEvent(&event)) {\n";
    code += "        switch (event.type) {\n";
    code += "            case SDL_QUIT:\n";
    code += "                return false;\n";
    code += "            case SDL_KEYDOWN:\n";
    code += "                if (event.key.keysym.sym == SDLK_ESCAPE) return false;\n";
    code += "                break;\n";
    code += "            case SDL_MOUSEBUTTONDOWN:\n";
    code += "                if (event.button.button == SDL_BUTTON_LEFT) {\n";
    code += "                    mouse_pressed = true;\n";
    code += "                    mouse_x = event.button.x;\n";
    code += "                    mouse_y = event.button.y;\n";
    code += "                }\n";
    code += "                break;\n";
    code += "            case SDL_MOUSEBUTTONUP:\n";
    code += "                if (event.button.button == SDL_BUTTON_LEFT) {\n";
    code += "                    mouse_pressed = false;\n";
    code += "                }\n";
    code += "                break;\n";
    code += "            case SDL_MOUSEMOTION:\n";
    code += "                mouse_x = event.motion.x;\n";
    code += "                mouse_y = event.motion.y;\n";
    code += "                break;\n";
    code += "            default:\n";
    code += "                break;\n";
    code += "        }\n";
    code += "    }\n";
    code += "    return true;\n";
    code += "}\n\n";

    code += "void sdl_driver_cleanup(void) {\n";
    code += "    if (disp_buf_data) {\n";
    code += "        free(disp_buf_data);\n";
    code += "        disp_buf_data = NULL;\n";
    code += "    }\n";
    code += "    if (texture) {\n";
    code += "        SDL_DestroyTexture(texture);\n";
    code += "        texture = NULL;\n";
    code += "    }\n";
    code += "    if (renderer) {\n";
    code += "        SDL_DestroyRenderer(renderer);\n";
    code += "        renderer = NULL;\n";
    code += "    }\n";
    code += "    if (window) {\n";
    code += "        SDL_DestroyWindow(window);\n";
    code += "        window = NULL;\n";
    code += "    }\n";
    code += "    SDL_Quit();\n";
    code += "}\n";

    return code;
}

QString LvglGenerator::generateSimulatorReadme() {
    QString name = m_project ? m_project->projectName() : "LVGL Application";
    if (name.isEmpty()) name = "LVGL Application";
    int width = m_project ? m_project->displayConfig().width : 320;
    int height = m_project ? m_project->displayConfig().height : 240;
    int depth = m_project ? m_project->displayConfig().colorDepth : 16;
    bool isRound = m_project ? m_project->displayConfig().round : false;

    QString md;
    md += QString("# %1 - LVGL PC Simulator\n\n").arg(name);
    md += "This subproject allows you to build and run your exact LVGL embedded GUI on your desktop computer (Linux, macOS, Windows) without needing physical MCU hardware.\n\n";
    md += "## Target Display Configuration\n\n";
    md += QString("- **Resolution**: %1 x %2\n").arg(width).arg(height);
    md += QString("- **Color Depth**: %1-bit\n").arg(depth);
    md += QString("- **Screen Shape**: %1\n\n").arg(isRound ? "Round (rendered with circular bezel overlay)" : "Rectangular");

    md += "## Prerequisites\n\n";
    md += "1. **CMake** 3.20 or newer\n";
    md += "2. **C/C++ Compiler** (GCC, Clang, or MSVC)\n";
    md += "3. **SDL2 Development Library**:\n";
    md += "   - Debian/Ubuntu: `sudo apt install libsdl2-dev`\n";
    md += "   - Fedora/RHEL: `sudo dnf install SDL2-devel`\n";
    md += "   - macOS: `brew install sdl2`\n";
    md += "   - Windows (vcpkg): `vcpkg install sdl2`\n";
    md += "   *(If SDL2 is not found on your system, CMake will automatically download and build SDL2 via `FetchContent`)*\n\n";

    md += "## Build & Run\n\n";
    md += "```bash\n";
    md += "mkdir build && cd build\n";
    md += "cmake ..\n";
    md += "cmake --build .\n";
    md += QString("./%1_pc_simulator\n").arg(name);
    md += "```\n";
    return md;
}
