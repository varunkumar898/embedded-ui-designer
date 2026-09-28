#include "UgfxExporter.h"
#include "DocumentModel.h"
#include "ScreenModel.h"
#include "WidgetModel.h"
#include "assets/ImageAssetProcessor.h"
#include <QFileInfo>
#include <QImage>

bool UgfxExporter::exportProject(const DocumentModel* doc, const QString& outDir) {
    if (!doc) {
        m_lastError = "DocumentModel pointer is null.";
        return false;
    }

    QDir dir(outDir);
    if (!dir.exists() && !dir.mkpath(".")) {
        m_lastError = QString("Failed to create output folder: %1").arg(outDir);
        return false;
    }

    if (!writeTextFile(dir.filePath("CMakeLists.txt"), generateCMakeLists(doc), &m_lastError)) return false;
    if (!writeTextFile(dir.filePath("gfxconf.h"), generateGfxConf(doc), &m_lastError)) return false;
    if (!writeTextFile(dir.filePath("ui.h"), generateUiHeader(doc), &m_lastError)) return false;
    if (!writeTextFile(dir.filePath("ui.c"), generateUiSource(doc), &m_lastError)) return false;
    if (!writeTextFile(dir.filePath("main.c"), generateMainSource(doc), &m_lastError)) return false;

    dir.mkdir("build");
    return true;
}

QString UgfxExporter::generateCMakeLists(const DocumentModel* doc) {
    QString name = doc->projectName().isEmpty() ? "ugfx_app" : doc->projectName();
    QString code;
    code += "cmake_minimum_required(VERSION 3.20)\n";
    code += QString("project(%1 LANGUAGES C CXX)\n\n").arg(name);
    code += "set(CMAKE_C_STANDARD 99)\n\n";
    code += "if(DEFINED UGFX_PATH)\n";
    code += "    set(UGFX_DIR \"${UGFX_PATH}\")\n";
    code += "else()\n";
    code += "    include(FetchContent)\n";
    code += "    FetchContent_Declare(ugfx_src GIT_REPOSITORY https://git.ugfx.io/uGFX/ugfx.git GIT_TAG master)\n";
    code += "    FetchContent_MakeAvailable(ugfx_src)\n";
    code += "    set(UGFX_DIR \"${ugfx_src_SOURCE_DIR}\")\n";
    code += "endif()\n\n";
    code += QString("add_executable(%1 main.c ui.c ${UGFX_DIR}/src/gfx_mk.c)\n").arg(name);
    code += QString("target_include_directories(%1 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${UGFX_DIR} ${UGFX_DIR}/src)\n").arg(name);
    code += "if(WIN32)\n";
    code += QString("    target_include_directories(%1 PRIVATE ${UGFX_DIR}/drivers/multiple/Win32)\n").arg(name);
    code += QString("    target_sources(%1 PRIVATE ${UGFX_DIR}/drivers/multiple/Win32/gdisp_lld_Win32.c)\n").arg(name);
    code += QString("    target_link_libraries(%1 PRIVATE gdi32 user32)\n").arg(name);
    code += "elseif(UNIX AND NOT APPLE)\n";
    code += QString("    target_include_directories(%1 PRIVATE ${UGFX_DIR}/drivers/multiple/X)\n").arg(name);
    code += QString("    target_sources(%1 PRIVATE ${UGFX_DIR}/drivers/multiple/X/gdisp_lld_X.c)\n").arg(name);
    code += QString("    target_link_libraries(%1 PRIVATE pthread X11)\n").arg(name);
    code += "endif()\n";
    return code;
}

QString UgfxExporter::generateGfxConf(const DocumentModel* doc) {
    bool hasButtons = false;
    bool hasLabels = false;
    bool hasProgressBars = false;
    bool hasImages = false;

    for (ScreenModel* screen : doc->screens()) {
        if (!screen) continue;
        for (WidgetModel* w : screen->widgets()) {
            if (!w) continue;
            if (w->type() == "Button") hasButtons = true;
            else if (w->type() == "Label" || w->type() == "Text") hasLabels = true;
            else if (w->type() == "ProgressBar") hasProgressBars = true;
            else if (w->type() == "Image") hasImages = true;
        }
    }

    QString code;
    code += "#ifndef _GFXCONF_H\n#define _GFXCONF_H\n\n";
    code += "#if defined(_WIN32) || defined(__WIN32__)\n";
    code += "    #define GFX_USE_OS_WIN32 GFXON\n";
    code += "#elif defined(__linux__) || defined(__unix__)\n";
    code += "    #define GFX_USE_OS_LINUX GFXON\n";
    code += "#else\n";
    code += "    #define GFX_USE_OS_RAW32 GFXON\n";
    code += "#endif\n\n";
    code += "#define GFX_USE_GDISP GFXON\n";
    code += "#define GDISP_NEED_VALIDATION GFXON\n";
    code += "#define GDISP_NEED_CLIP GFXON\n";
    code += "#define GDISP_NEED_TEXT GFXON\n";
    code += "#define GDISP_NEED_CIRCLE GFXON\n";
    code += QString("#define GDISP_NEED_IMAGE %1\n").arg(hasImages ? "GFXON" : "GFXOFF");
    code += QString("#define GDISP_NEED_BITMAP %1\n").arg(hasImages ? "GFXON" : "GFXOFF");
    code += "#define GDISP_INCLUDE_FONT_DEJAVUSANS12 GFXON\n";
    code += "#define GDISP_INCLUDE_FONT_DEJAVUSANS16 GFXON\n";
    code += "#define GFX_USE_GWIN GFXON\n";
    code += "#define GWIN_NEED_WINDOWMANAGER GFXON\n";
    code += "#define GWIN_NEED_WIDGET GFXON\n";
    code += QString("#define GWIN_NEED_BUTTON %1\n").arg(hasButtons ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_LABEL %1\n").arg(hasLabels ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_PROGRESSBAR %1\n").arg(hasProgressBars ? "GFXON" : "GFXOFF");
    code += QString("#define GWIN_NEED_IMAGE %1\n").arg(hasImages ? "GFXON" : "GFXOFF");
    code += "#define GFX_USE_GEVENT GFXON\n";
    code += "#define GFX_USE_GINPUT GFXON\n";
    code += "#define GINPUT_NEED_MOUSE GFXON\n\n";
    code += "#endif /* _GFXCONF_H */\n";
    return code;
}

QString UgfxExporter::generateUiHeader(const DocumentModel* doc) {
    QString code;
    code += "#ifndef _UI_H\n#define _UI_H\n\n";
    code += "#include \"gfx.h\"\n\n";
    code += "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n";
    for (ScreenModel* screen : doc->screens()) {
        if (!screen) continue;
        for (WidgetModel* w : screen->widgets()) {
            if (!w) continue;
            if (w->type() == "Button") {
                code += QString("extern GHandle ghBtn_%1;\n").arg(w->id());
            } else if (w->type() == "Label" || w->type() == "Text") {
                code += QString("extern GHandle ghLbl_%1;\n").arg(w->id());
            } else if (w->type() == "ProgressBar") {
                code += QString("extern GHandle ghProg_%1;\n").arg(w->id());
            } else if (w->type() == "Image") {
                code += QString("extern const uint8_t %1_data[];\n").arg(w->id());
                code += QString("extern const uint32_t %1_data_width;\n").arg(w->id());
                code += QString("extern const uint32_t %1_data_height;\n").arg(w->id());
            }
        }
    }
    code += "\nvoid ui_init(void);\n\n";
    code += "#ifdef __cplusplus\n}\n#endif\n\n";
    code += "#endif /* _UI_H */\n";
    return code;
}

QString UgfxExporter::generateUiSource(const DocumentModel* doc) {
    QString code;
    code += "#include \"gfx.h\"\n#include \"ui.h\"\n\n";

    // 1. Asset Compilation Pipeline: Process and emit C byte arrays for all Image / ImageComponent widgets
    for (ScreenModel* screen : doc->screens()) {
        if (!screen) continue;
        for (WidgetModel* w : screen->widgets()) {
            if (!w || (w->type() != "Image" && w->type() != "ImageComponent")) continue;

            QImage img;
            if (!w->imagePath().isEmpty() && QFileInfo::exists(w->imagePath())) {
                img.load(w->imagePath());
            } else {
                // Generate fallback test pattern image
                int iw = qMax(16, static_cast<int>(w->width()));
                int ih = qMax(16, static_cast<int>(w->height()));
                img = QImage(iw, ih, QImage::Format_RGB32);
                img.fill(QColor(w->color().isValid() ? w->color() : QColor("#2196F3")));
            }

            ImageFormat fmt = (doc->colorDepth() == 1) ? ImageFormat::Monochrome : ImageFormat::RGB565;
            QString fmtStr = w->getCustomProperty("format").toString();
            if (fmtStr.compare("Monochrome", Qt::CaseInsensitive) == 0 || fmtStr == "1") {
                fmt = ImageFormat::Monochrome;
            } else if (fmtStr.compare("RGB565", Qt::CaseInsensitive) == 0 || fmtStr == "16") {
                fmt = ImageFormat::RGB565;
            }

            // Compile into embedded C array using ImageAssetProcessor
            code += ImageAssetProcessor::generateCArray(img, fmt, w->id() + "_data");
            code += "\n";
        }
    }

    // 2. Global widget handles
    for (ScreenModel* screen : doc->screens()) {
        if (!screen) continue;
        for (WidgetModel* w : screen->widgets()) {
            if (!w) continue;
            if (w->type() == "Button") code += QString("GHandle ghBtn_%1 = 0;\n").arg(w->id());
            else if (w->type() == "Label" || w->type() == "Text") code += QString("GHandle ghLbl_%1 = 0;\n").arg(w->id());
            else if (w->type() == "ProgressBar") code += QString("GHandle ghProg_%1 = 0;\n").arg(w->id());
        }
    }

    code += "\nvoid ui_init(void) {\n";
    code += "    GWidgetInit wi;\n";
    code += "    gwinWidgetClearInit(&wi);\n";
    code += "    wi.g.show = gTrue;\n\n";
    code += "    font_t fontDefault = gdispOpenFont(\"DejaVuSans12\");\n\n";

    for (ScreenModel* screen : doc->screens()) {
        if (!screen) continue;
        code += QString("    // === Screen: %1 (%2) ===\n").arg(screen->id(), screen->name());
        for (WidgetModel* w : screen->widgets()) {
            if (!w) continue;
            if (w->type() == "Rectangle") {
                code += QString("    // Rectangle: %1\n").arg(w->id());
                code += QString("    gdispFillArea(%1, %2, %3, %4, HTML2COLOR(0x%5));\n")
                    .arg(static_cast<int>(w->x()))
                    .arg(static_cast<int>(w->y()))
                    .arg(static_cast<int>(w->width()))
                    .arg(static_cast<int>(w->height()))
                    .arg(w->color().name().mid(1).toUpper());
            } else if (w->type() == "Button") {
                code += QString("    // Button: %1 (Target: %2)\n").arg(w->id(), w->targetScreenId());
                code += "    gwinWidgetClearInit(&wi); wi.g.show = gTrue;\n";
                code += QString("    wi.g.x = %1; wi.g.y = %2; wi.g.width = %3; wi.g.height = %4;\n")
                    .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
                code += QString("    wi.text = \"%1\";\n").arg(w->text().isEmpty() ? "Button" : w->text());
                code += QString("    ghBtn_%1 = gwinButtonCreate(NULL, &wi);\n").arg(w->id());
                code += QString("    gwinSetFont(ghBtn_%1, fontDefault);\n\n").arg(w->id());
            } else if (w->type() == "Label" || w->type() == "Text") {
                code += QString("    // Label: %1\n").arg(w->id());
                code += "    gwinWidgetClearInit(&wi); wi.g.show = gTrue;\n";
                code += QString("    wi.g.x = %1; wi.g.y = %2; wi.g.width = %3; wi.g.height = %4;\n")
                    .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
                code += QString("    wi.text = \"%1\";\n").arg(w->text().isEmpty() ? "Label" : w->text());
                code += QString("    ghLbl_%1 = gwinLabelCreate(NULL, &wi);\n").arg(w->id());
                code += QString("    gwinSetFont(ghLbl_%1, fontDefault);\n\n").arg(w->id());
            } else if (w->type() == "ProgressBar") {
                code += QString("    // Progress Bar: %1\n").arg(w->id());
                code += "    gwinWidgetClearInit(&wi); wi.g.show = gTrue;\n";
                code += QString("    wi.g.x = %1; wi.g.y = %2; wi.g.width = %3; wi.g.height = %4;\n")
                    .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
                code += QString("    ghProg_%1 = gwinProgressbarCreate(NULL, &wi);\n").arg(w->id());
                code += QString("    gwinProgressbarSetPosition(ghProg_%1, 50);\n\n").arg(w->id());
            } else if (w->type() == "Image") {
                code += QString("    // Image: %1\n").arg(w->id());
                code += QString("    gdispDrawBitmap(%1, %2, %3_data_width, %3_data_height, (const gdisp_pixel_t*)%3_data);\n\n")
                    .arg(static_cast<int>(w->x()))
                    .arg(static_cast<int>(w->y()))
                    .arg(w->id());
            }
        }
    }
    code += "}\n";
    return code;
}

QString UgfxExporter::generateMainSource(const DocumentModel* doc) {
    Q_UNUSED(doc);
    QString code;
    code += "#include \"gfx.h\"\n#include \"ui.h\"\n#include <stdio.h>\n\n";
    code += "int main(int argc, char *argv[]) {\n";
    code += "    (void)argc; (void)argv;\n";
    code += "    gfxInit();\n";
    code += "    gdispClear(Black);\n";
    code += "    ui_init();\n\n";
    code += "    GListener gl;\n";
    code += "    geventListenerInit(&gl);\n";
    code += "    gwinAttachListener(&gl);\n\n";
    code += "    while (gTrue) {\n";
    code += "        GEvent *pe = geventEventWait(&gl, 100);\n";
    code += "        if (pe && pe->type == GEVENT_GWIN_BUTTON) {\n";
    code += "            printf(\"Button Event Triggered\\n\");\n";
    code += "        }\n";
    code += "    }\n";
    code += "    return 0;\n";
    code += "}\n";
    return code;
}
