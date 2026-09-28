#include "QtMcuGenerator.h"
#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include <QSet>

QtMcuGenerator::QtMcuGenerator(Project* project, CanvasScene* scene)
    : CodeGenerator(project)
    , m_scene(scene)
{
}

bool QtMcuGenerator::generate(const QString& outputDirectory) {
    if (!m_project || !m_scene) {
        m_lastError = "Project or canvas scene is null.";
        return false;
    }

    QDir outDir(outputDirectory);
    if (!outDir.exists() && !outDir.mkpath(".")) {
        m_lastError = QString("Could not create output directory: %1").arg(outputDirectory);
        return false;
    }

    QString name = m_project->projectName();
    if (name.isEmpty()) name = "MyUI";

    // 1. CMakeLists.txt (Qt Quick Ultralite QUL CMake)
    if (!writeFile(outDir.filePath("CMakeLists.txt"), generateCMakeLists())) return false;

    // 2. <ProjectName>.qmlproject
    if (!writeFile(outDir.filePath(name + ".qmlproject"), generateQmlProject())) return false;

    // 3. MainScreen.qml (Only QUL-supported basic QML types)
    if (!writeFile(outDir.filePath("MainScreen.qml"), generateMainScreenQml())) return false;

    // 4. README.md
    if (!writeFile(outDir.filePath("README.md"), generateReadme())) return false;

    outDir.mkdir("build");
    return true;
}

QString QtMcuGenerator::generateCMakeLists() {
    QString name = m_project->projectName();
    if (name.isEmpty()) name = "MyUI";

    QString cmake;
    cmake += "cmake_minimum_required(VERSION 3.21)\n\n";
    cmake += QString("project(%1 LANGUAGES C CXX ASM)\n\n").arg(name);
    cmake += "set(CMAKE_CXX_STANDARD 17)\n";
    cmake += "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";
    cmake += "# Find Qt Quick Ultralite SDK\n";
    cmake += "find_package(Qul REQUIRED)\n\n";
    cmake += QString("qul_add_target(%1 QML_PROJECT %1.qmlproject)\n\n").arg(name);
    cmake += QString("app_target_setup_os(%1)\n").arg(name);
    return cmake;
}

QString QtMcuGenerator::generateQmlProject() {
    QString name = m_project->projectName();
    if (name.isEmpty()) name = "MyUI";

    QString code;
    code += "import QmlProject\n\n";
    code += "Project {\n";
    code += "    mainFile: \"MainScreen.qml\"\n\n";
    code += "    Module {\n";
    code += QString("        name: \"%1\"\n").arg(name);
    code += "        qmlFiles: [\n";
    code += "            \"MainScreen.qml\"\n";
    code += "        ]\n";
    code += "    }\n";
    code += "}\n";
    return code;
}

QString QtMcuGenerator::generateMainScreenQml() {
    DisplayConfig cfg = m_project->displayConfig();

    QString qml;
    qml += "import Qul\n\n";
    qml += "Rectangle {\n";
    qml += "    id: root\n";
    qml += QString("    width: %1\n").arg(cfg.width);
    qml += QString("    height: %2\n").arg(cfg.height);
    qml += QString("    color: \"%1\"\n\n").arg(m_scene->screenBackgroundColor().name());

    // QUL Signals
    QSet<QString> handlers;
    for (auto comp : m_scene->uiComponents()) {
        if (auto btn = dynamic_cast<ButtonComponent*>(comp)) {
            QString handler = btn->onClickedHandler().trimmed();
            if (!handler.isEmpty() && !handlers.contains(handler)) {
                handlers.insert(handler);
                qml += QString("    signal %1()\n").arg(handler);
            }
        }
    }
    if (!handlers.isEmpty()) {
        qml += "\n";
    }

    // Components rendered using strictly supported QUL types:
    for (auto comp : m_scene->uiComponents()) {
        if (auto rect = dynamic_cast<RectangleComponent*>(comp)) {
            qml += "    Rectangle {\n";
            qml += QString("        id: %1\n").arg(rect->componentId());
            qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                .arg(static_cast<int>(rect->pos().x()))
                .arg(static_cast<int>(rect->pos().y()))
                .arg(static_cast<int>(rect->compWidth()))
                .arg(static_cast<int>(rect->compHeight()));
            qml += QString("        color: \"%1\"\n").arg(rect->fillColor().name());
            if (rect->cornerRadius() > 0) {
                qml += QString("        radius: %1\n").arg(rect->cornerRadius());
            }
            if (rect->strokeWidth() > 0) {
                qml += QString("        border.color: \"%1\"\n").arg(rect->strokeColor().name());
                qml += QString("        border.width: %1\n").arg(rect->strokeWidth());
            }
            qml += "    }\n\n";
        } else if (auto lbl = dynamic_cast<LabelComponent*>(comp)) {
            qml += "    Text {\n";
            qml += QString("        id: %1\n").arg(lbl->componentId());
            qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                .arg(static_cast<int>(lbl->pos().x()))
                .arg(static_cast<int>(lbl->pos().y()))
                .arg(static_cast<int>(lbl->compWidth()))
                .arg(static_cast<int>(lbl->compHeight()));
            qml += QString("        text: \"%1\"\n").arg(lbl->text());
            qml += QString("        color: \"%1\"\n").arg(lbl->color().name());
            qml += QString("        font.pixelSize: %1\n").arg(lbl->pixelSize());
            if (lbl->bold()) qml += "        font.bold: true\n";
            if (lbl->italic()) qml += "        font.italic: true\n";
            qml += "    }\n\n";
        } else if (auto btn = dynamic_cast<ButtonComponent*>(comp)) {
            // In Qt Quick Ultralite, Buttons are built via Rectangle + Text + MouseArea
            qml += "    Rectangle {\n";
            qml += QString("        id: %1\n").arg(btn->componentId());
            qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                .arg(static_cast<int>(btn->pos().x()))
                .arg(static_cast<int>(btn->pos().y()))
                .arg(static_cast<int>(btn->compWidth()))
                .arg(static_cast<int>(btn->compHeight()));
            qml += QString("        color: %1_mouse.pressed ? \"%2\" : \"%3\"\n")
                .arg(btn->componentId())
                .arg(btn->backgroundColor().darker(120).name())
                .arg(btn->backgroundColor().name());
            if (btn->cornerRadius() > 0) {
                qml += QString("        radius: %1\n").arg(btn->cornerRadius());
            }
            qml += "        Text {\n";
            qml += "            anchors.centerIn: parent\n";
            qml += QString("            text: \"%1\"\n").arg(btn->text());
            qml += QString("            color: \"%1\"\n").arg(btn->textColor().name());
            qml += "            font.bold: true\n";
            qml += "            font.pixelSize: 14\n";
            qml += "        }\n";
            qml += "        MouseArea {\n";
            qml += QString("            id: %1_mouse\n").arg(btn->componentId());
            qml += "            anchors.fill: parent\n";
            if (!btn->onClickedHandler().isEmpty()) {
                qml += QString("            onClicked: root.%1()\n").arg(btn->onClickedHandler());
            }
            qml += "        }\n";
            qml += "    }\n\n";
        } else if (auto prog = dynamic_cast<ProgressBarComponent*>(comp)) {
            // In QUL, ProgressBar is built via track Rectangle + fill Rectangle
            qml += "    Rectangle {\n";
            qml += QString("        id: %1\n").arg(prog->componentId());
            qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                .arg(static_cast<int>(prog->pos().x()))
                .arg(static_cast<int>(prog->pos().y()))
                .arg(static_cast<int>(prog->compWidth()))
                .arg(static_cast<int>(prog->compHeight()));
            qml += QString("        color: \"%1\"\n").arg(prog->trackColor().name());
            if (prog->cornerRadius() > 0) {
                qml += QString("        radius: %1\n").arg(prog->cornerRadius());
            }
            qml += "        Rectangle {\n";
            qml += "            height: parent.height\n";
            qml += QString("            width: parent.width * %1\n").arg(prog->value(), 0, 'f', 2);
            qml += QString("            color: \"%1\"\n").arg(prog->barColor().name());
            if (prog->cornerRadius() > 0) {
                qml += QString("            radius: %1\n").arg(prog->cornerRadius());
            }
            qml += "        }\n";
            qml += "    }\n\n";
        }
    }

    qml += "}\n";
    return qml;
}

QString QtMcuGenerator::generateReadme() {
    DisplayConfig cfg = m_project->displayConfig();
    QString name = m_project->projectName();

    QString md;
    md += QString("# %1 (Qt for MCUs - Qt Quick Ultralite Project)\n\n").arg(name);
    md += "Auto-generated by **Embedded UI Designer**.\n\n";
    md += "## Target Specifications\n";
    md += QString("- **Display Resolution**: %1 × %2 (%3-bit)\n").arg(cfg.width).arg(cfg.height).arg(cfg.colorDepth);
    md += "- **Framework**: Qt Quick Ultralite (QUL 2.x+)\n\n";
    md += "## Licensing & Board Notice\n";
    md += "Qt for MCUs is a licensed commercial/evaluation product supporting select hardware:\n";
    md += "- STMicroelectronics (STM32F4/F7/H7)\n";
    md += "- NXP (i.MX RT1050/1060/1170)\n";
    md += "- Renesas (RH850, RA6M3)\n";
    md += "- Infineon (TRAVEO T2G)\n";
    md += "- Espressif (ESP32-S3)\n\n";
    md += "## Building with CMake & QUL SDK\n\n";
    md += "Configure for your target MCU platform:\n";
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
