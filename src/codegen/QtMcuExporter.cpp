#include "QtMcuExporter.h"
#include "DocumentModel.h"
#include "ScreenModel.h"
#include "WidgetModel.h"

bool QtMcuExporter::exportProject(const DocumentModel* doc, const QString& outDir) {
    if (!doc) {
        m_lastError = "DocumentModel pointer is null.";
        return false;
    }

    QDir dir(outDir);
    if (!dir.exists() && !dir.mkpath(".")) {
        m_lastError = QString("Failed to create output folder: %1").arg(outDir);
        return false;
    }

    QString name = doc->projectName().isEmpty() ? "MyUI" : doc->projectName();

    if (!writeTextFile(dir.filePath("CMakeLists.txt"), generateCMakeLists(doc), &m_lastError)) return false;
    if (!writeTextFile(dir.filePath(name + ".qmlproject"), generateQmlProject(doc), &m_lastError)) return false;

    // Export each screen as an individual QML component file
    for (ScreenModel* screen : doc->screens()) {
        if (!screen) continue;
        QString qmlContent;
        qmlContent += "import Qul\n\n";
        qmlContent += "Rectangle {\n";
        qmlContent += QString("    id: %1_root\n").arg(screen->id().toLower());
        qmlContent += QString("    width: %1\n").arg(screen->width());
        qmlContent += QString("    height: %2\n").arg(screen->height());
        qmlContent += QString("    color: \"%1\"\n\n").arg(screen->backgroundColor().name());

        for (WidgetModel* w : screen->widgets()) {
            if (!w) continue;
            if (w->type() == "Button") {
                qmlContent += "    Rectangle {\n";
                qmlContent += QString("        id: %1\n").arg(w->id());
                qmlContent += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
                qmlContent += QString("        color: %1_mouse.pressed ? \"%2\" : \"%3\"\n")
                    .arg(w->id()).arg(w->color().darker(120).name()).arg(w->color().name());
                qmlContent += "        radius: 4\n";
                qmlContent += "        Text {\n";
                qmlContent += "            anchors.centerIn: parent\n";
                qmlContent += QString("            text: \"%1\"\n").arg(w->text().isEmpty() ? "Button" : w->text());
                qmlContent += "            color: \"#ffffff\"\n";
                qmlContent += "            font.bold: true\n";
                qmlContent += "            font.pixelSize: 14\n";
                qmlContent += "        }\n";
                qmlContent += "        MouseArea {\n";
                qmlContent += QString("            id: %1_mouse\n").arg(w->id());
                qmlContent += "            anchors.fill: parent\n";
                if (!w->targetScreenId().isEmpty()) {
                    qmlContent += QString("            // Target navigation: %1\n").arg(w->targetScreenId());
                }
                qmlContent += "        }\n";
                qmlContent += "    }\n\n";
            } else if (w->type() == "Label" || w->type() == "Text") {
                qmlContent += "    Text {\n";
                qmlContent += QString("        id: %1\n").arg(w->id());
                qmlContent += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
                qmlContent += QString("        text: \"%1\"\n").arg(w->text().isEmpty() ? "Label" : w->text());
                qmlContent += QString("        color: \"%1\"\n").arg(w->color().name());
                qmlContent += "        font.pixelSize: 16\n";
                qmlContent += "    }\n\n";
            } else if (w->type() == "Rectangle") {
                qmlContent += "    Rectangle {\n";
                qmlContent += QString("        id: %1\n").arg(w->id());
                qmlContent += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
                qmlContent += QString("        color: \"%1\"\n").arg(w->color().name());
                qmlContent += "        radius: 4\n";
                qmlContent += "    }\n\n";
            } else if (w->type() == "ProgressBar") {
                qmlContent += "    Rectangle {\n";
                qmlContent += QString("        id: %1\n").arg(w->id());
                qmlContent += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
                qmlContent += "        color: \"#2b2d35\"\n";
                qmlContent += "        radius: 3\n";
                qmlContent += "        Rectangle {\n";
                qmlContent += "            height: parent.height\n";
                qmlContent += "            width: parent.width * 0.5\n";
                qmlContent += QString("            color: \"%1\"\n").arg(w->color().name());
                qmlContent += "            radius: 3\n";
                qmlContent += "        }\n";
                qmlContent += "    }\n\n";
            } else if (w->type() == "Image") {
                qmlContent += "    Image {\n";
                qmlContent += QString("        id: %1\n").arg(w->id());
                qmlContent += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
                qmlContent += QString("        source: \"%1\"\n").arg(w->imagePath().isEmpty() ? "asset.png" : w->imagePath());
                qmlContent += "    }\n\n";
            }
        }
        qmlContent += "}\n";

        QString screenFilename = screen->id() + ".qml";
        if (!writeTextFile(dir.filePath(screenFilename), qmlContent, &m_lastError)) return false;
    }

    if (!writeTextFile(dir.filePath("MainScreen.qml"), generateMainScreenQml(doc), &m_lastError)) return false;
    if (!writeTextFile(dir.filePath("README.md"), generateReadme(doc), &m_lastError)) return false;

    dir.mkdir("build");
    return true;
}

QString QtMcuExporter::generateCMakeLists(const DocumentModel* doc) {
    QString name = doc->projectName().isEmpty() ? "MyUI" : doc->projectName();
    QString cmake;
    cmake += "cmake_minimum_required(VERSION 3.21)\n\n";
    cmake += QString("project(%1 LANGUAGES C CXX ASM)\n\n").arg(name);
    cmake += "set(CMAKE_CXX_STANDARD 17)\n";
    cmake += "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";
    cmake += "find_package(Qul REQUIRED)\n\n";
    cmake += QString("qul_add_target(%1 QML_PROJECT %1.qmlproject)\n\n").arg(name);
    cmake += QString("app_target_setup_os(%1)\n").arg(name);
    return cmake;
}

QString QtMcuExporter::generateQmlProject(const DocumentModel* doc) {
    QString name = doc->projectName().isEmpty() ? "MyUI" : doc->projectName();
    QString code;
    code += "import QmlProject\n\n";
    code += "Project {\n";
    code += "    mainFile: \"MainScreen.qml\"\n\n";
    code += "    Module {\n";
    code += QString("        name: \"%1\"\n").arg(name);
    code += "        qmlFiles: [\n";
    code += "            \"MainScreen.qml\"";
    for (ScreenModel* screen : doc->screens()) {
        if (!screen || screen->id() == "MainScreen") continue;
        code += QString(",\n            \"%1.qml\"").arg(screen->id());
    }
    code += "\n        ]\n";
    code += "    }\n";
    code += "}\n";
    return code;
}

QString QtMcuExporter::generateMainScreenQml(const DocumentModel* doc) {
    QString qml;
    qml += "import Qul\n\n";
    qml += "Rectangle {\n";
    qml += "    id: root\n";
    qml += QString("    width: %1\n").arg(doc->screenWidth());
    qml += QString("    height: %2\n").arg(doc->screenHeight());
    qml += "    color: \"#1a1c22\"\n\n";

    for (WidgetModel* w : doc->widgets()) {
        if (!w) continue;
        if (w->type() == "Button") {
            qml += "    Rectangle {\n";
            qml += QString("        id: %1\n").arg(w->id());
            qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
            qml += QString("        color: %1_mouse.pressed ? \"%2\" : \"%3\"\n")
                .arg(w->id()).arg(w->color().darker(120).name()).arg(w->color().name());
            qml += "        radius: 4\n";
            qml += "        Text {\n";
            qml += "            anchors.centerIn: parent\n";
            qml += QString("            text: \"%1\"\n").arg(w->text().isEmpty() ? "Button" : w->text());
            qml += "            color: \"#ffffff\"\n";
            qml += "            font.bold: true\n";
            qml += "            font.pixelSize: 14\n";
            qml += "        }\n";
            qml += "        MouseArea {\n";
            qml += QString("            id: %1_mouse\n").arg(w->id());
            qml += "            anchors.fill: parent\n";
            qml += "        }\n";
            qml += "    }\n\n";
        } else if (w->type() == "Label" || w->type() == "Text") {
            qml += "    Text {\n";
            qml += QString("        id: %1\n").arg(w->id());
            qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
            qml += QString("        text: \"%1\"\n").arg(w->text().isEmpty() ? "Label" : w->text());
            qml += QString("        color: \"%1\"\n").arg(w->color().name());
            qml += "        font.pixelSize: 16\n";
            qml += "    }\n\n";
        } else if (w->type() == "Rectangle") {
            qml += "    Rectangle {\n";
            qml += QString("        id: %1\n").arg(w->id());
            qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
            qml += QString("        color: \"%1\"\n").arg(w->color().name());
            qml += "        radius: 4\n";
            qml += "    }\n\n";
        } else if (w->type() == "ProgressBar") {
            qml += "    Rectangle {\n";
            qml += QString("        id: %1\n").arg(w->id());
            qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                .arg(static_cast<int>(w->x())).arg(static_cast<int>(w->y())).arg(static_cast<int>(w->width())).arg(static_cast<int>(w->height()));
            qml += "        color: \"#2b2d35\"\n";
            qml += "        radius: 3\n";
            qml += "        Rectangle {\n";
            qml += "            height: parent.height\n";
            qml += "            width: parent.width * 0.5\n";
            qml += QString("            color: \"%1\"\n").arg(w->color().name());
            qml += "            radius: 3\n";
            qml += "        }\n";
            qml += "    }\n\n";
        }
    }

    qml += "}\n";
    return qml;
}

QString QtMcuExporter::generateReadme(const DocumentModel* doc) {
    QString name = doc->projectName().isEmpty() ? "MyUI" : doc->projectName();
    QString md;
    md += QString("# %1 (Qt for MCUs - Qt Quick Ultralite Project)\n\n").arg(name);
    md += "Auto-generated by **Embedded UI Designer** MVVM Exporter.\n\n";
    md += "## Target Specifications\n";
    md += QString("- **Display Resolution**: %1 × %2 (%3-bit)\n").arg(doc->screenWidth()).arg(doc->screenHeight()).arg(doc->colorDepth());
    md += "- **Framework**: Qt Quick Ultralite (QUL 2.x+)\n\n";
    md += "## Building with CMake & QUL SDK\n\n";
    md += "```bash\n";
    md += "cmake -S . -B build \\\n";
    md += "  -DCMAKE_TOOLCHAIN_FILE=$QUL_ROOT/lib/cmake/Qul/toolchain/armgcc.cmake \\\n";
    md += "  -DQUL_PLATFORM=<your-board-id> \\\n";
    md += "  -DQUL_TARGET_TOOLCHAIN_DIR=/path/to/arm-none-eabi \\\n";
    md += "  -DQUL_BOARD_SDK_DIR=/path/to/board-sdk\n\n";
    md += "cmake --build build\n";
    md += "```\n";
    return md;
}
