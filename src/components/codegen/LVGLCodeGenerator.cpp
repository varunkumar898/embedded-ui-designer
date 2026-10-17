#include "LVGLCodeGenerator.h"
#include "components/ComponentLibrary.h"
#include "hardware/BoardRegistry.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

void LVGLCodeGenerator::addWidget(const LVGLWidgetInstance& widget) {
    m_widgets.append(widget);
}

void LVGLCodeGenerator::clearWidgets() {
    m_widgets.clear();
}

QString LVGLCodeGenerator::generateHeader(const QString& targetBoard) const {
    QString out;
    QTextStream ts(&out);

    ts << "#ifndef UI_LVGL_GEN_H\n";
    ts << "#define UI_LVGL_GEN_H\n\n";
    ts << "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n";

    BoardDef board = BoardRegistry::instance().activeBoard();
    if (!targetBoard.isEmpty()) {
        BoardDef b = BoardRegistry::instance().board(targetBoard);
        if (b.isValid()) board = b;
    }

    ts << "/* Target Board: " << (board.displayName.isEmpty() ? board.id : board.displayName) << " (" << board.family << ") */\n";
    if (board.family.contains("esp32", Qt::CaseInsensitive)) {
        ts << "/* ESP-IDF / ESP32 Target */\n";
        ts << "#include \"esp_system.h\"\n";
    } else if (board.family.contains("stm32", Qt::CaseInsensitive)) {
        ts << "/* STM32 Target */\n";
        ts << "#include \"stm32f0xx_hal.h\"\n";
    } else if (board.family.contains("rp2040", Qt::CaseInsensitive)) {
        ts << "/* RP2040 Target */\n";
        ts << "#include \"pico/stdlib.h\"\n";
    }

    ts << "\n/* Active Board Pin Definitions */\n";
    for (auto it = board.pinMap.begin(); it != board.pinMap.end(); ++it) {
        ts << QString("#define PIN_%1 \"%1\" /* %2%3 */\n")
            .arg(it.key(), it.value().function,
                 it.value().strapping ? " [Strapping]" : (it.value().reserved ? " [Reserved]" : ""));
    }
    ts << "\n";

    ts << "#include \"lvgl.h\"\n\n";

    ts << "/* Widget References */\n";
    for (const auto& w : m_widgets) {
        ts << "extern lv_obj_t* " << w.name << ";\n";
    }

    ts << "\n/* Lifecycle Initializers */\n";
    ts << "void ui_init(void);\n";
    ts << "void ui_destroy(void);\n\n";

    ts << "#ifdef __cplusplus\n}\n#endif\n\n";
    ts << "#endif /* UI_LVGL_GEN_H */\n";

    return out;
}

QString LVGLCodeGenerator::generateSource(const QString& targetBoard) const {
    Q_UNUSED(targetBoard);
    QString out;
    QTextStream ts(&out);

    ts << "#include \"ui.h\"\n";
    ts << "#if defined(__has_include)\n";
    ts << "#  if __has_include(\"target_hal.h\")\n";
    ts << "#    include \"target_hal.h\"\n";
    ts << "#  endif\n";
    ts << "#endif\n\n";

    ts << "/* Static Widget Allocations */\n";
    for (const auto& w : m_widgets) {
        ts << "lv_obj_t* " << w.name << " = NULL;\n";
    }
    ts << "\n";

    // Callbacks
    ts << "/* Event Callbacks */\n";
    for (const auto& w : m_widgets) {
        QString cbName = w.properties.value("event_callback").toString();
        if (!cbName.isEmpty()) {
            ts << "static void " << cbName << "(lv_event_t* e) {\n";
            ts << "    lv_event_code_t code = lv_event_get_code(e);\n";
            ts << "    lv_obj_t* target = lv_event_get_target(e);\n";
            ts << "    LV_UNUSED(code);\n    LV_UNUSED(target);\n";
            ts << "}\n\n";
        }
    }

    ts << "void ui_init(void) {\n";
    ts << "    /* Target Hardware HAL Initialization */\n";
    ts << "    #if defined(TARGET_HAL_H)\n";
    ts << "    target_hal_init();\n";
    ts << "    #endif\n\n";
    ts << "    lv_obj_t* parent = lv_scr_act();\n\n";

    for (const auto& w : m_widgets) {
        QString widgetCode = ComponentLibrary::instance().generateWidgetCode(
            w.type, w.name, w.x, w.y, w.width, w.height, w.properties, "parent");
        ts << widgetCode << "\n\n";
    }

    ts << "}\n\n";

    ts << "void ui_destroy(void) {\n";
    ts << "    lv_obj_clean(lv_scr_act());\n";
    ts << "}\n";

    return out;
}

bool LVGLCodeGenerator::exportToDirectory(const QString& outputDir, const QString& targetBoard) const {
    QDir dir(outputDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile headerFile(dir.filePath("ui.h"));
    if (headerFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        headerFile.write(generateHeader(targetBoard).toUtf8());
        headerFile.close();
    } else {
        return false;
    }

    QFile sourceFile(dir.filePath("ui.c"));
    if (sourceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        sourceFile.write(generateSource(targetBoard).toUtf8());
        sourceFile.close();
    } else {
        return false;
    }

    return true;
}
