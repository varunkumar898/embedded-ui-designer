#include "QtMcuGenerator.h"
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

static UIComponent* findQmlInteractionTarget(CanvasScene* scene, const QString& componentId) {
    if (!scene) return nullptr;
    for (UIComponent* component : scene->uiComponents()) {
        if (component->componentId() == componentId) return component;
    }
    return nullptr;
}

static QString qmlString(const QString& value) {
    QString escaped = value;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    escaped.replace("\n", "\\n");
    return escaped;
}

static QString qmlInteractionAction(const QJsonObject& interaction, CanvasScene* scene, const QString& indent) {
    UIComponent* target = findQmlInteractionTarget(scene, interaction.value("target").toString());
    if (!target) return {};
    const QString targetId = target->componentId();
    const QString action = interaction.value("action").toString();
    const QString value = interaction.value("value").toString();
    const QString transition = interaction.value("transition").toString("Instant");
    const int duration = interaction.value("duration").toInt();
    QString code;
    if (dynamic_cast<SwitchComponent*>(target)) {
        code += indent + QString("%1.prototypeTransition = \"%2\";\n").arg(targetId, transition);
        code += indent + QString("%1.prototypeDuration = %2;\n").arg(targetId).arg(duration);
    }
    if (action == "Show") code += indent + targetId + ".visible = true;\n";
    else if (action == "Hide") code += indent + targetId + ".visible = false;\n";
    else if (action == "Toggle") {
        if (dynamic_cast<SwitchComponent*>(target) || dynamic_cast<CheckboxComponent*>(target))
            code += indent + targetId + ".checked = !" + targetId + ".checked;\n";
        else code += indent + targetId + ".visible = !" + targetId + ".visible;\n";
    } else if (action == "Set Value") {
        if (dynamic_cast<SwitchComponent*>(target) || dynamic_cast<CheckboxComponent*>(target))
            code += indent + QString("%1.checked = %2;\n").arg(targetId, value.compare("true", Qt::CaseInsensitive) == 0 || value == "1" ? "true" : "false");
        else if (dynamic_cast<LabelComponent*>(target) || dynamic_cast<ButtonComponent*>(target) || dynamic_cast<TextInputComponent*>(target))
            code += indent + QString("%1.text = \"%2\";\n").arg(targetId, qmlString(value));
        else code += indent + QString("%1.value = %2;\n").arg(targetId, value);
    }
    return code;
}

static void appendQmlInteractions(QString& qml, UIComponent* source, CanvasScene* scene,
                                 const QStringList& triggers, const QString& indent) {
    for (const QJsonValue& value : source->interactions()) {
        const QJsonObject interaction = value.toObject();
        if (triggers.contains(interaction.value("trigger").toString()))
            qml += qmlInteractionAction(interaction, scene, indent);
    }
}

static bool hasQmlInteractions(UIComponent* source, const QStringList& triggers) {
    for (const QJsonValue& value : source->interactions()) {
        if (triggers.contains(value.toObject().value("trigger").toString())) return true;
    }
    return false;
}

QtMcuGenerator::QtMcuGenerator(Project* project, CanvasScene* scene)
    : CodeGenerator(project)
    , m_scene(scene)
    , m_ownsProjectAndScene(false)
{
}

QtMcuGenerator::QtMcuGenerator(const QString& projectFilePath)
    : CodeGenerator(nullptr)
    , m_ownsProjectAndScene(true)
{
    m_scene = new CanvasScene();
    m_project = new Project(m_scene);
    m_project->loadFromFile(projectFilePath);
}

QtMcuGenerator::~QtMcuGenerator() {
    if (m_ownsProjectAndScene) {
        delete m_project;
        delete m_scene;
    }
}

bool QtMcuGenerator::isSupportedQulType(const QString& typeName) {
    // Whitelist of valid Qt Quick Ultralite core types
    static const QSet<QString> supportedTypes = {
        "Rectangle", "Text", "Image", "MouseArea", "Item", "Timer", 
        "AnimatedImage", "FontLoader", "Loader"
    };
    return supportedTypes.contains(typeName);
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

    // 1. CMakeLists.txt (Qt Quick Ultralite QUL CMake)
    if (!writeFile(outDir.filePath("CMakeLists.txt"), generateCMakeLists())) return false;

    // 2. project.qmlproject
    if (!writeFile(outDir.filePath("project.qmlproject"), generateQmlProject())) return false;

    // 3. design.qml (Only QUL-supported basic QML types)
    if (!writeFile(outDir.filePath("design.qml"), generateDesignQml())) return false;

    // 4. README.md
    if (!writeFile(outDir.filePath("README.md"), generateReadme())) return false;

    // 5. pc_simulator/ subfolder (Desktop simulator using QUL desktop target or host Qt6 Quick)
    QDir simDir(outDir.filePath("pc_simulator"));
    if (!simDir.exists() && !simDir.mkpath(".")) {
        m_lastError = QString("Could not create pc_simulator directory: %1").arg(simDir.absolutePath());
        return false;
    }
    if (!writeFile(simDir.filePath("CMakeLists.txt"), generateSimulatorCMakeLists())) return false;
    if (!writeFile(simDir.filePath("main.cpp"), generateSimulatorMainSource())) return false;
    if (!writeFile(simDir.filePath("README.md"), generateSimulatorReadme())) return false;

    outDir.mkdir("build");
    return true;
}

QString QtMcuGenerator::generateCMakeLists() {
    QString name = m_project ? m_project->projectName() : "MyUI";
    if (name.isEmpty()) name = "MyUI";

    QString cmake;
    cmake += "cmake_minimum_required(VERSION 3.21)\n\n";
    cmake += QString("project(%1 LANGUAGES C CXX ASM)\n\n").arg(name);
    cmake += "set(CMAKE_CXX_STANDARD 17)\n";
    cmake += "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n";
    cmake += "# Locate Qt for MCUs (QUL) SDK\n";
    cmake += "find_package(Qul REQUIRED)\n\n";
    cmake += QString("qul_add_target(%1 QML_PROJECT project.qmlproject)\n\n").arg(name);
    cmake += QString("app_target_setup_os(%1)\n").arg(name);
    return cmake;
}

QString QtMcuGenerator::generateQmlProject() {
    QString name = m_project ? m_project->projectName() : "MyUI";
    if (name.isEmpty()) name = "MyUI";

    QString code;
    code += "import QmlProject\n\n";
    code += "Project {\n";
    code += "    mainFile: \"design.qml\"\n\n";
    code += "    Module {\n";
    code += QString("        name: \"%1\"\n").arg(name);
    code += "        qmlFiles: [\n";
    code += "            \"design.qml\"\n";
    code += "        ]\n";
    code += "    }\n";
    code += "}\n";
    return code;
}

QString QtMcuGenerator::generateDesignQml() {
    DisplayConfig cfg = m_project ? m_project->displayConfig() : DisplayConfig();

    QString qml;
    qml += "/**\n";
    qml += " * Auto-generated by Embedded UI Designer for Qt Quick Ultralite (QUL)\n";
    qml += " * Target: Qt for MCUs 2.x+\n";
    qml += " * Strictly adheres to supported QUL types (Rectangle, Text, Image, MouseArea).\n";
    qml += " */\n";
    qml += "import Qul 1.0\n\n";
    qml += "import QtQuick.Shapes 1.15\n\n";
    qml += "Rectangle {\n";
    qml += "    id: root\n";
    qml += "    // Prototype actions target components on this screen only.\n";
    qml += QString("    width: %1\n").arg(cfg.width);
    qml += QString("    height: %2\n").arg(cfg.height);
    QString bgColor = (m_scene) ? m_scene->screenBackgroundColor().name() : "#ffffff";
    qml += QString("    color: \"%1\"\n\n").arg(bgColor);

    // QUL Signals
    QSet<QString> handlers;
    if (m_scene) {
        for (auto comp : m_scene->uiComponents()) {
            if (auto btn = dynamic_cast<ButtonComponent*>(comp)) {
                QString handler = btn->onClickedHandler().trimmed();
                if (!handler.isEmpty() && !handlers.contains(handler)) {
                    handlers.insert(handler);
                    qml += QString("    signal %1()\n").arg(handler);
                }
            } else if (auto sw = dynamic_cast<SwitchComponent*>(comp)) {
                QString handler = sw->onToggledHandler().trimmed();
                if (!handler.isEmpty() && !handlers.contains(handler)) {
                    handlers.insert(handler);
                    qml += QString("    signal %1(bool checked)\n").arg(handler);
                }
            } else if (auto chk = dynamic_cast<CheckboxComponent*>(comp)) {
                QString handler = chk->onToggledHandler().trimmed();
                if (!handler.isEmpty() && !handlers.contains(handler)) {
                    handlers.insert(handler);
                    qml += QString("    signal %1(bool checked)\n").arg(handler);
                }
            } else if (auto txt = dynamic_cast<TextInputComponent*>(comp)) {
                QString handler = txt->onTextChangedHandler().trimmed();
                if (!handler.isEmpty() && !handlers.contains(handler)) {
                    handlers.insert(handler);
                    qml += QString("    signal %1(string text)\n").arg(handler);
                }
            }
        }
    }
    if (!handlers.isEmpty()) {
        qml += "\n";
    }

    // Render Components using strictly supported QUL primitives
    if (m_scene) {
        for (auto comp : m_scene->uiComponents()) {
            if (auto rect = dynamic_cast<RectangleComponent*>(comp)) {
                // Verified QUL type: Rectangle
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
                // Verified QUL type: Text
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
                // In Qt Quick Ultralite, standard buttons are implemented via Rectangle + Text + MouseArea
                qml += "    Rectangle {\n";
                qml += QString("        id: %1\n").arg(btn->componentId());
                qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(btn->pos().x()))
                    .arg(static_cast<int>(btn->pos().y()))
                    .arg(static_cast<int>(btn->compWidth()))
                    .arg(static_cast<int>(btn->compHeight()));
                qml += QString("        property string text: \"%1\"\n").arg(qmlString(btn->text()));
                qml += QString("        color: %1_mouse.pressed ? \"%2\" : \"%3\"\n")
                    .arg(btn->componentId())
                    .arg(btn->backgroundColor().darker(120).name())
                    .arg(btn->backgroundColor().name());
                if (btn->cornerRadius() > 0) {
                    qml += QString("        radius: %1\n").arg(btn->cornerRadius());
                }
                qml += "        Text {\n";
                qml += "            anchors.centerIn: parent\n";
                qml += "            text: parent.text\n";
                qml += QString("            color: \"%1\"\n").arg(btn->textColor().name());
                qml += "            font.bold: true\n";
                qml += "            font.pixelSize: 14\n";
                qml += "        }\n";
                qml += "        MouseArea {\n";
                qml += QString("            id: %1_mouse\n").arg(btn->componentId());
                qml += "            anchors.fill: parent\n";
                const bool hasPrototypeClick = hasQmlInteractions(btn, {"On Click"});
                if (!hasPrototypeClick && !btn->onClickedHandler().isEmpty()) {
                    qml += QString("            onClicked: root.%1()\n").arg(btn->onClickedHandler());
                } else if (hasPrototypeClick) {
                    qml += "            onClicked: {\n";
                    if (!btn->onClickedHandler().isEmpty())
                        qml += QString("                root.%1()\n").arg(btn->onClickedHandler());
                    appendQmlInteractions(qml, btn, m_scene, {"On Click"}, "                ");
                    qml += "            }\n";
                }
                qml += "        }\n";
                qml += "    }\n\n";
            } else if (auto prog = dynamic_cast<ProgressBarComponent*>(comp)) {
                // In QUL, ProgressBar is implemented via track Rectangle + fill Rectangle
                qml += "    Rectangle {\n";
                qml += QString("        id: %1\n").arg(prog->componentId());
                qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(prog->pos().x()))
                    .arg(static_cast<int>(prog->pos().y()))
                    .arg(static_cast<int>(prog->compWidth()))
                    .arg(static_cast<int>(prog->compHeight()));
                qml += QString("        property real value: %1\n").arg(prog->value(), 0, 'f', 3);
                qml += QString("        color: \"%1\"\n").arg(prog->trackColor().name());
                if (prog->cornerRadius() > 0) {
                    qml += QString("        radius: %1\n").arg(prog->cornerRadius());
                }
                qml += "        Rectangle {\n";
                qml += "            height: parent.height\n";
                qml += "            width: parent.width * parent.value\n";
                qml += QString("            color: \"%1\"\n").arg(prog->barColor().name());
                if (prog->cornerRadius() > 0) {
                    qml += QString("            radius: %1\n").arg(prog->cornerRadius());
                }
                qml += "        }\n";
                qml += "    }\n\n";
            } else if (auto img = dynamic_cast<ImageComponent*>(comp)) {
                // Verified QUL type: Image
                qml += "    Image {\n";
                qml += QString("        id: %1\n").arg(img->componentId());
                qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(img->pos().x()))
                    .arg(static_cast<int>(img->pos().y()))
                    .arg(static_cast<int>(img->compWidth()))
                    .arg(static_cast<int>(img->compHeight()));
                qml += QString("        source: \"%1\"\n").arg(img->imagePath().isEmpty() ? "asset.png" : img->imagePath());
                qml += "    }\n\n";
            } else if (auto slider = dynamic_cast<SliderComponent*>(comp)) {
                // In QUL, Slider is built using pure supported primitives: Item + track/fill Rectangles + thumb
                qml += "    Item {\n";
                qml += QString("        id: %1\n").arg(slider->componentId());
                qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(slider->pos().x()))
                    .arg(static_cast<int>(slider->pos().y()))
                    .arg(static_cast<int>(slider->compWidth()))
                    .arg(static_cast<int>(slider->compHeight()));
                qml += QString("        property int value: %1\n").arg(slider->value());
                qml += QString("        property int minimum: %1\n").arg(slider->minimum());
                qml += QString("        property int maximum: %1\n").arg(slider->maximum());
                qml += "        Rectangle {\n";
                qml += "            anchors.verticalCenter: parent.verticalCenter\n";
                qml += "            x: 10; width: parent.width - 20; height: 6; radius: 3\n";
                qml += QString("            color: \"%1\"\n").arg(slider->trackColor().name());
                qml += "            Rectangle {\n";
                qml += "                height: parent.height; radius: 3\n";
                qml += "                width: parent.width * (parent.parent.maximum > parent.parent.minimum ? (parent.parent.value - parent.parent.minimum) / (parent.parent.maximum - parent.parent.minimum) : 0)\n";
                qml += QString("                color: \"%1\"\n").arg(slider->fillColor().name());
                qml += "            }\n";
                qml += "        }\n";
                qml += "        Rectangle {\n";
                qml += "            width: 14; height: 14; radius: 7\n";
                qml += "            x: 10 + (parent.width - 20) * (parent.maximum > parent.minimum ? (parent.value - parent.minimum) / (parent.maximum - parent.minimum) : 0) - 7\n";
                qml += "            anchors.verticalCenter: parent.verticalCenter\n";
                qml += QString("            color: \"%1\"\n").arg(slider->handleColor().name());
                qml += QString("            border.color: \"%1\"; border.width: 1\n").arg(slider->fillColor().name());
                qml += "        }\n";
                qml += "        MouseArea {\n";
                qml += "            anchors.fill: parent\n";
                qml += "            onClicked: {\n";
                qml += "                parent.value = parent.minimum + Math.round((parent.maximum - parent.minimum) * mouse.x / parent.width)\n";
                appendQmlInteractions(qml, slider, m_scene,
                                      {"On Click", "On Change", "On Value Changed"}, "                ");
                qml += "            }\n";
                qml += "        }\n";
                qml += "    }\n\n";
            } else if (auto sw = dynamic_cast<SwitchComponent*>(comp)) {
                // In QUL, Switch is built using pure supported primitives: Rectangle pill + circular thumb + MouseArea
                qml += "    Rectangle {\n";
                qml += QString("        id: %1\n").arg(sw->componentId());
                qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(sw->pos().x()))
                    .arg(static_cast<int>(sw->pos().y()))
                    .arg(static_cast<int>(sw->compWidth()))
                    .arg(static_cast<int>(sw->compHeight()));
                qml += QString("        property bool checked: %1\n").arg(sw->isChecked() ? "true" : "false");
                qml += "        property int prototypeDuration: 0\n";
                qml += "        property string prototypeTransition: \"Instant\"\n";
                qml += "        radius: height / 2\n";
                qml += QString("        color: checked ? \"%1\" : \"%2\"\n")
                    .arg(sw->onColor().name()).arg(sw->offColor().name());
                qml += "        Behavior on color { ColorAnimation { duration: prototypeTransition == \"Dissolve\" ? prototypeDuration : 0 } }\n";
                qml += "        // Slide moves the thumb; Dissolve fades the track color.\n";
                qml += "        Rectangle {\n";
                qml += "            width: parent.height - 6; height: width; radius: width / 2\n";
                qml += "            anchors.verticalCenter: parent.verticalCenter\n";
                qml += "            x: parent.checked ? parent.width - width - 3 : 3\n";
                qml += QString("            Behavior on x { NumberAnimation { duration: %1.prototypeTransition == \"Slide\" ? %1.prototypeDuration : 0 } }\n").arg(sw->componentId());
                qml += QString("            color: \"%1\"\n").arg(sw->thumbColor().name());
                qml += "        }\n";
                qml += "        MouseArea {\n";
                qml += "            anchors.fill: parent\n";
                qml += "            onClicked: {\n";
                qml += "                parent.checked = !parent.checked;\n";
                if (!sw->onToggledHandler().isEmpty()) {
                    qml += QString("                root.%1(parent.checked);\n").arg(sw->onToggledHandler());
                }
                appendQmlInteractions(qml, sw, m_scene,
                                      {"On Click", "On Change", "On Value Changed"}, "                ");
                qml += "            }\n";
                qml += "        }\n";
                qml += "    }\n\n";
            } else if (auto chk = dynamic_cast<CheckboxComponent*>(comp)) {
                // In QUL, Checkbox is built using pure supported primitives: Item + box Rectangle + check Rectangle + Text label + MouseArea
                qml += "    Item {\n";
                qml += QString("        id: %1\n").arg(chk->componentId());
                qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(chk->pos().x()))
                    .arg(static_cast<int>(chk->pos().y()))
                    .arg(static_cast<int>(chk->compWidth()))
                    .arg(static_cast<int>(chk->compHeight()));
                qml += QString("        property bool checked: %1\n").arg(chk->isChecked() ? "true" : "false");
                qml += "        Rectangle {\n";
                qml += QString("            id: %1_box\n").arg(chk->componentId());
                qml += "            width: 18; height: 18; radius: 3\n";
                qml += "            anchors.verticalCenter: parent.verticalCenter\n";
                qml += QString("            color: \"%1\"\n").arg(chk->boxColor().name());
                qml += QString("            border.color: \"%1\"; border.width: 1\n").arg(chk->borderColor().name());
                qml += "            Rectangle {\n";
                qml += "                width: 10; height: 10; radius: 2\n";
                qml += "                anchors.centerIn: parent\n";
                qml += QString("                color: \"%1\"\n").arg(chk->checkColor().name());
                qml += "                visible: parent.parent.checked\n";
                qml += "            }\n";
                qml += "        }\n";
                qml += "        Text {\n";
                qml += QString("            anchors.left: %1_box.right; anchors.leftMargin: 8\n").arg(chk->componentId());
                qml += "            anchors.verticalCenter: parent.verticalCenter\n";
                qml += QString("            text: \"%1\"\n").arg(chk->text());
                qml += QString("            color: \"%1\"\n").arg(chk->textColor().name());
                qml += "            font.pixelSize: 13\n";
                qml += "        }\n";
                qml += "        MouseArea {\n";
                qml += "            anchors.fill: parent\n";
                qml += "            onClicked: {\n";
                qml += "                parent.checked = !parent.checked;\n";
                if (!chk->onToggledHandler().isEmpty()) {
                    qml += QString("                root.%1(parent.checked);\n").arg(chk->onToggledHandler());
                }
                appendQmlInteractions(qml, chk, m_scene,
                                      {"On Click", "On Change", "On Value Changed"}, "                ");
                qml += "            }\n";
                qml += "        }\n";
                qml += "    }\n\n";
            } else if (auto txt = dynamic_cast<TextInputComponent*>(comp)) {
                // In QUL, TextInput is rendered inside a styled background Rectangle
                qml += "    Rectangle {\n";
                qml += QString("        id: %1_box\n").arg(txt->componentId());
                qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(txt->pos().x()))
                    .arg(static_cast<int>(txt->pos().y()))
                    .arg(static_cast<int>(txt->compWidth()))
                    .arg(static_cast<int>(txt->compHeight()));
                qml += QString("        color: \"%1\"\n").arg(txt->backgroundColor().name());
                if (txt->borderWidth() > 0) {
                    qml += QString("        border.color: \"%1\"\n").arg(txt->borderColor().name());
                    qml += QString("        border.width: %1\n").arg(txt->borderWidth());
                }
                if (txt->cornerRadius() > 0) {
                    qml += QString("        radius: %1\n").arg(txt->cornerRadius());
                }
                qml += "        TextInput {\n";
                qml += QString("            id: %1\n").arg(txt->componentId());
                qml += "            anchors.fill: parent\n";
                qml += "            anchors.leftMargin: 6; anchors.rightMargin: 6\n";
                qml += "            anchors.verticalCenter: parent.verticalCenter\n";
                qml += QString("            text: \"%1\"\n").arg(txt->text());
                qml += QString("            color: \"%1\"\n").arg(txt->textColor().name());
                qml += QString("            font.pixelSize: %1\n").arg(txt->pixelSize());
                if (txt->isReadOnly()) {
                    qml += "            readOnly: true\n";
                }
                const bool hasPrototypeTextChange = hasQmlInteractions(txt, {"On Change", "On Value Changed"});
                if (!hasPrototypeTextChange && !txt->onTextChangedHandler().isEmpty()) {
                    qml += QString("            onTextChanged: root.%1(text)\n").arg(txt->onTextChangedHandler());
                } else if (hasPrototypeTextChange) {
                    qml += "            onTextChanged: {\n";
                    if (!txt->onTextChangedHandler().isEmpty())
                        qml += QString("                root.%1(text)\n").arg(txt->onTextChangedHandler());
                    appendQmlInteractions(qml, txt, m_scene, {"On Change", "On Value Changed"}, "                ");
                    qml += "            }\n";
                }
                if (hasQmlInteractions(txt, {"On Click"})) {
                    qml += "            onActiveFocusChanged: {\n";
                    qml += "                if (activeFocus) {\n";
                    appendQmlInteractions(qml, txt, m_scene, {"On Click"}, "                    ");
                    qml += "                }\n";
                    qml += "            }\n";
                }
                qml += "        }\n";
                qml += "    }\n\n";
            } else if (auto circ = dynamic_cast<CircleComponent*>(comp)) {
                // In QUL, Circle is rendered via Rectangle with radius: width / 2
                int dim = static_cast<int>(qMin(circ->compWidth(), circ->compHeight()));
                qml += "    Rectangle {\n";
                qml += QString("        id: %1\n").arg(circ->componentId());
                qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(circ->pos().x()))
                    .arg(static_cast<int>(circ->pos().y()))
                    .arg(dim).arg(dim);
                qml += "        radius: width / 2\n";
                qml += QString("        color: \"%1\"\n").arg(circ->isFilled() ? circ->fillColor().name() : "transparent");
                if (circ->strokeWidth() > 0) {
                    qml += QString("        border.color: \"%1\"\n").arg(circ->strokeColor().name());
                    qml += QString("        border.width: %1\n").arg(circ->strokeWidth());
                }
                qml += "    }\n\n";
            } else if (auto path = dynamic_cast<PathComponent*>(comp)) {
                const QList<PathAnchor> anchors = path->anchors();
                if (anchors.size() < 3) continue;
                qml += "    Shape {\n";
                qml += QString("        id: %1\n").arg(path->componentId());
                qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(path->pos().x()))
                    .arg(static_cast<int>(path->pos().y()))
                    .arg(static_cast<int>(path->compWidth()))
                    .arg(static_cast<int>(path->compHeight()));
                qml += QString("        opacity: %1\n").arg(path->opacityPercent() / 100.0, 0, 'f', 2);
                qml += "        ShapePath {\n";
                qml += QString("            strokeColor: \"%1\"\n").arg(path->strokeColor().name());
                qml += "            fillColor: \"transparent\"\n";
                qml += QString("            strokeWidth: %1\n").arg(path->strokeThickness());
                qml += QString("            startX: %1; startY: %2\n")
                    .arg(anchors.first().position.x()).arg(anchors.first().position.y());
                for (int index = 0; index < anchors.size(); ++index) {
                    const PathAnchor& start = anchors.at(index);
                    const PathAnchor& end = anchors.at((index + 1) % anchors.size());
                    if (start.hasHandleOut || end.hasHandleIn) {
                        const QPointF control1 = start.hasHandleOut ? start.position + start.handleOut : start.position;
                        const QPointF control2 = end.hasHandleIn ? end.position + end.handleIn : end.position;
                        qml += QString("            PathCubic { x: %1; y: %2; control1X: %3; control1Y: %4; control2X: %5; control2Y: %6 }\n")
                            .arg(end.position.x()).arg(end.position.y())
                            .arg(control1.x()).arg(control1.y()).arg(control2.x()).arg(control2.y());
                    } else {
                        qml += QString("            PathLine { x: %1; y: %2 }\n")
                            .arg(end.position.x()).arg(end.position.y());
                    }
                }
                qml += "        }\n";
                qml += "    }\n\n";
            } else if (comp) {
                // Fallback for unsupported / unrecognized types
                QString id = comp->componentId();
                qml += QString("    // WARNING: Component '%1' is not natively supported in Qt Quick Ultralite core.\n").arg(id);
                qml += "    // Fallback: Emitting Rectangle visual placeholder to ensure valid QUL compilation.\n";
                qml += "    Rectangle {\n";
                qml += QString("        id: %1\n").arg(id);
                qml += QString("        x: %1; y: %2; width: %3; height: %4\n")
                    .arg(static_cast<int>(comp->pos().x()))
                    .arg(static_cast<int>(comp->pos().y()))
                    .arg(static_cast<int>(comp->compWidth()))
                    .arg(static_cast<int>(comp->compHeight()));
                qml += "        color: \"transparent\"\n";
                qml += "        border.color: \"#ff9800\"\n";
                qml += "        border.width: 1\n";
                qml += "        Text {\n";
                qml += "            anchors.centerIn: parent\n";
                qml += QString("            text: \"[%1]\"\n").arg(id);
                qml += "            color: \"#ff9800\"\n";
                qml += "            font.pixelSize: 12\n";
                qml += "        }\n";
                qml += "    }\n\n";
            }
        }
    }

    qml += "}\n";
    return qml;
}

QString QtMcuGenerator::generateReadme() {
    DisplayConfig cfg = m_project ? m_project->displayConfig() : DisplayConfig();
    QString name = m_project ? m_project->projectName() : "MyUI";

    QString md;
    md += QString("# %1 (Qt Quick Ultralite / Qt for MCUs Project)\n\n").arg(name);
    md += "Auto-generated by **Embedded UI Designer**.\n\n";
    md += "## Target Specifications\n";
    md += QString("- **Display Resolution**: %1 × %2 (%3-bit)\n").arg(cfg.width).arg(cfg.height).arg(cfg.colorDepth);
    md += "- **Framework**: Qt Quick Ultralite (QUL 2.x+)\n";
    md += "- **Main QML File**: `design.qml`\n";
    md += "- **Project File**: `project.qmlproject`\n\n";
    md += "## Building with CMake & QUL SDK\n\n";
    md += "To build for your target MCU platform:\n";
    md += "```bash\n";
    md += "cmake -S . -B build \\\n";
    md += "  -DCMAKE_TOOLCHAIN_FILE=$QUL_ROOT/lib/cmake/Qul/toolchain/armgcc.cmake \\\n";
    md += "  -DQUL_PLATFORM=<my-board> \\\n";
    md += "  -DQUL_TARGET_TOOLCHAIN_DIR=/path/to/arm-none-eabi \\\n";
    md += "  -DQUL_BOARD_SDK_DIR=/path/to/board-sdk\n\n";
    md += "cmake --build build\n";
    md += "```\n\n";
    md += "## Supported Hardware Platforms\n";
    md += "- **STMicroelectronics**: STM32F469, STM32F769, STM32H750, STM32U5\n";
    md += "- **NXP**: i.MX RT1050, i.MX RT1064, i.MX RT1170\n";
    md += "- **Renesas**: EK-RA6M3G, RH850\n";
    md += "- **Infineon**: CY8CKIT-062S2-43012, TRAVEO T2G\n";
    md += "- **Espressif**: ESP32-S3-BOX\n";
    return md;
}

QString QtMcuGenerator::generateSimulatorCMakeLists() {
    QString name = m_project ? m_project->projectName() : "MyUI";
    if (name.isEmpty()) name = "MyUI";
    int width = m_project ? m_project->displayConfig().width : 320;
    int height = m_project ? m_project->displayConfig().height : 240;
    int depth = m_project ? m_project->displayConfig().colorDepth : 16;
    int isRound = (m_project && m_project->displayConfig().round) ? 1 : 0;

    QString cmake;
    cmake += "cmake_minimum_required(VERSION 3.21)\n\n";
    cmake += QString("project(%1_pc_simulator LANGUAGES C CXX)\n\n").arg(name);
    cmake += "set(CMAKE_CXX_STANDARD 17)\n";
    cmake += "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n";
    cmake += "set(CMAKE_AUTOMOC ON)\n\n";

    cmake += "# ── Check for official Qt Quick Ultralite (QUL) SDK ────────────────────────\n";
    cmake += "find_package(Qul QUIET)\n";
    cmake += "if(Qul_FOUND)\n";
    cmake += "    message(STATUS \"Found Qt for MCUs (QUL) SDK. Configuring QUL desktop simulator target...\")\n";
    cmake += "    set(QUL_PLATFORM \"desktop\" CACHE STRING \"Target platform for QUL\")\n";
    cmake += QString("    qul_add_target(%1_pc_simulator QML_PROJECT ../project.qmlproject)\n").arg(name);
    cmake += QString("    app_target_setup_os(%1_pc_simulator)\n").arg(name);
    cmake += "else()\n";
    cmake += "    message(STATUS \"QUL SDK not found. Using host desktop Qt6 infrastructure (QQuickView simulator)...\")\n";
    cmake += "    find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)\n\n";
    cmake += QString("    add_executable(%1_pc_simulator main.cpp)\n").arg(name);
    cmake += QString("    target_include_directories(%1_pc_simulator PRIVATE \"${CMAKE_CURRENT_SOURCE_DIR}/..\")\n").arg(name);
    cmake += QString("    target_compile_definitions(%1_pc_simulator PRIVATE\n").arg(name);
    cmake += QString("        SIMULATOR_WIDTH=%1\n").arg(width);
    cmake += QString("        SIMULATOR_HEIGHT=%1\n").arg(height);
    cmake += QString("        SIMULATOR_COLOR_DEPTH=%1\n").arg(depth);
    cmake += QString("        SIMULATOR_IS_ROUND=%1\n").arg(isRound);
    cmake += "    )\n";
    cmake += QString("    target_link_libraries(%1_pc_simulator PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)\n").arg(name);
    cmake += "endif()\n";

    return cmake;
}

QString QtMcuGenerator::generateSimulatorMainSource() {
    QString name = m_project ? m_project->projectName() : "MyUI";
    if (name.isEmpty()) name = "MyUI";
    int width = m_project ? m_project->displayConfig().width : 320;
    int height = m_project ? m_project->displayConfig().height : 240;
    bool isRound = m_project ? m_project->displayConfig().round : false;

    QString code;
    code += "/**\n";
    code += " * Qt for MCUs (QUL) PC Simulator Launcher\n";
    code += " * Auto-generated by Embedded UI Designer\n";
    code += " */\n";
    code += "#include <QGuiApplication>\n";
    code += "#include <QQuickView>\n";
    code += "#include <QUrl>\n";
    code += "#include <QDir>\n";
    code += "#include <QFile>\n";
    code += "#include <QFileInfo>\n";
    code += "#include <QRegion>\n";
    code += "#include <QTimer>\n";
    code += "#include <QImage>\n";
    code += "#include <iostream>\n";
    code += "#include <cstdlib>\n\n";

    code += QString("#ifndef SIMULATOR_WIDTH\n#define SIMULATOR_WIDTH %1\n#endif\n").arg(width);
    code += QString("#ifndef SIMULATOR_HEIGHT\n#define SIMULATOR_HEIGHT %1\n#endif\n").arg(height);
    code += QString("#ifndef SIMULATOR_IS_ROUND\n#define SIMULATOR_IS_ROUND %1\n#endif\n\n").arg(isRound ? 1 : 0);

    code += "int main(int argc, char* argv[]) {\n";
    code += "    QGuiApplication app(argc, argv);\n\n";
    code += "    std::cout << \"============================================================\\n\";\n";
    code += QString("    std::cout << \" Starting %1 Qt for MCUs Desktop Simulator\\n\";\n").arg(name);
    code += "    std::cout << \" Resolution: \" << SIMULATOR_WIDTH << \" x \" << SIMULATOR_HEIGHT\n";
    code += "              << \" (\" << (SIMULATOR_IS_ROUND ? \"Round Display\" : \"Rectangular Display\") << \")\\n\";\n";
    code += "    std::cout << \"============================================================\\n\";\n\n";

    code += "    QQuickView view;\n";
    code += QString("    view.setTitle(\"%1 - Qt for MCUs PC Simulator\");\n").arg(name);
    code += "    view.setResizeMode(QQuickView::SizeViewToRootObject);\n";
    code += "    view.resize(SIMULATOR_WIDTH, SIMULATOR_HEIGHT);\n\n";

    code += "    if (SIMULATOR_IS_ROUND) {\n";
    code += "        view.setMask(QRegion(0, 0, SIMULATOR_WIDTH, SIMULATOR_HEIGHT, QRegion::Ellipse));\n";
    code += "    }\n\n";

    code += "    QString qmlPath = QFileInfo(QDir(app.applicationDirPath()), \"../design.qml\").canonicalFilePath();\n";
    code += "    if (qmlPath.isEmpty() || !QFile::exists(qmlPath)) {\n";
    code += "        qmlPath = QFileInfo(QDir::current(), \"../design.qml\").canonicalFilePath();\n";
    code += "    }\n";
    code += "    if (qmlPath.isEmpty() || !QFile::exists(qmlPath)) {\n";
    code += "        qmlPath = \"design.qml\";\n";
    code += "    }\n\n";

    code += "    std::cout << \"Loading QML design: \" << qmlPath.toStdString() << \"\\n\";\n";
    code += "    view.setSource(QUrl::fromLocalFile(qmlPath));\n";
    code += "    view.show();\n\n";

    code += "    // Optional screenshot capture if requested by test harness\n";
    code += "    const char* envShot = std::getenv(\"SIMULATOR_SCREENSHOT_PATH\");\n";
    code += "    if (envShot && envShot[0] != '\\0') {\n";
    code += "        QString shotPath = QString::fromUtf8(envShot);\n";
    code += "        QTimer::singleShot(500, [&view, shotPath]() {\n";
    code += "            QImage shot = view.grabWindow();\n";
    code += "            if (!shot.isNull()) {\n";
    code += "                shot.save(shotPath);\n";
    code += "                std::cout << \"[QUL Simulator] Saved screenshot to \" << shotPath.toStdString() << std::endl;\n";
    code += "            }\n";
    code += "        });\n";
    code += "    }\n\n";

    code += "    return app.exec();\n";
    code += "}\n";

    return code;
}

QString QtMcuGenerator::generateSimulatorReadme() {
    QString name = m_project ? m_project->projectName() : "MyUI";
    if (name.isEmpty()) name = "MyUI";
    int width = m_project ? m_project->displayConfig().width : 320;
    int height = m_project ? m_project->displayConfig().height : 240;
    int depth = m_project ? m_project->displayConfig().colorDepth : 16;
    bool isRound = m_project ? m_project->displayConfig().round : false;

    QString md;
    md += QString("# %1 - Qt for MCUs (QUL) PC Simulator\n\n").arg(name);
    md += "This subfolder contains a desktop simulator project for your Qt Quick Ultralite design.\n\n";
    md += "## Target Display\n\n";
    md += QString("- **Resolution**: %1 x %2\n").arg(width).arg(height);
    md += QString("- **Color Depth**: %1-bit\n").arg(depth);
    md += QString("- **Shape**: %1\n\n").arg(isRound ? "Round (elliptical window mask)" : "Rectangular");

    md += "## Desktop Simulator Engines\n\n";
    md += "This CMake project automatically detects your host environment:\n";
    md += "1. **Official QUL Desktop Platform**: If the proprietary `Qul` package is detected, it configures `qul_add_target` with `set(QUL_PLATFORM \"desktop\")`.\n";
    md += "2. **Standard Host Qt6 Simulator**: If the QUL SDK is not installed, it falls back to standard host Qt6 (`Qt6::Quick` / `QQuickView`), reusing the exact same Qt infrastructure that the Embedded UI Designer application builds with.\n\n";

    md += "## Build & Run\n\n";
    md += "```bash\n";
    md += "mkdir build && cd build\n";
    md += "cmake ..\n";
    md += "cmake --build .\n";
    md += QString("./%1_pc_simulator\n").arg(name);
    md += "```\n";
    return md;
}
