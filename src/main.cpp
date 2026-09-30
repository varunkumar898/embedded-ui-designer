#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFileInfo>
#include <QDir>
#include <iostream>

#include "MainWindow.h"
#include "Project.h"
#include "CanvasScene.h"
#include "UgfxGenerator.h"
#include "QtMcuGenerator.h"
#include "LvglGenerator.h"
#include "ButtonComponent.h"
#include "CustomComponentInstance.h"
#include "CustomComponentDefinition.h"
#include "ColorStyle.h"

int main(int argc, char *argv[])
{
    // Check if CLI export or verification mode is requested before creating QApplication
    bool isHeadless = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--export") == 0 || strcmp(argv[i], "-e") == 0 ||
            strcmp(argv[i], "--verify-feature1") == 0 || strcmp(argv[i], "--verify-feature2") == 0) {
            isHeadless = true;
            break;
        }
    }

    // Set offscreen platform in headless CLI mode
    if (isHeadless) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QApplication app(argc, argv);
    app.setApplicationName("EmbeddedUIDesigner");
    app.setApplicationDisplayName("Embedded UI Designer");
    app.setOrganizationName("EmbeddedDev");
    app.setOrganizationDomain("embeddeddev.org");
    app.setApplicationVersion("1.0.0");
    app.setWindowIcon(QIcon(":/packaging/embedded-ui-designer.png"));

    QCommandLineParser parser;
    parser.setApplicationDescription("Embedded UI Designer - Visual UI Designer and Code Generator for Microcontrollers");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption exportOption(
        QStringList() << "e" << "export",
        "Export target framework ('ugfx', 'qul', or 'lvgl')",
        "target"
    );
    parser.addOption(exportOption);

    QCommandLineOption projectOption(
        QStringList() << "p" << "project",
        "Path to input .euiproj project file",
        "file"
    );
    parser.addOption(projectOption);

    QCommandLineOption outOption(
        QStringList() << "o" << "out",
        "Output directory for generated project files",
        "dir"
    );
    parser.addOption(outOption);

    QCommandLineOption verifyF1Option(
        "verify-feature1",
        "Run Feature 1 verification (Named Color Styles) and save screenshot to output path",
        "output_png"
    );
    parser.addOption(verifyF1Option);

    QCommandLineOption verifyF2Option(
        "verify-feature2",
        "Run Feature 2 verification (Custom Components) and save screenshot to output path",
        "output_png"
    );
    parser.addOption(verifyF2Option);

    parser.process(app);

    // -------------------------------------------------------------------------
    // 1. Headless CLI Export Mode
    // -------------------------------------------------------------------------
    if (parser.isSet(exportOption)) {
        QString target = parser.value(exportOption).toLower().trimmed();
        QString projectPath = parser.value(projectOption);
        QString outDir = parser.value(outOption);

        if (projectPath.isEmpty()) {
            std::cerr << "Error: --project <file> is required for export mode." << std::endl;
            return 1;
        }
        if (outDir.isEmpty()) {
            std::cerr << "Error: --out <dir> is required for export mode." << std::endl;
            return 1;
        }

        if (target != "ugfx" && target != "qul" && target != "lvgl") {
            std::cerr << "Error: Unknown export target '" << target.toStdString() 
                      << "'. Supported targets: 'ugfx', 'qul', 'lvgl'." << std::endl;
            return 1;
        }

        CanvasScene scene;
        Project project(&scene);
        if (!project.loadFromFile(projectPath)) {
            std::cerr << "Error: Failed to load project from: " << projectPath.toStdString() << std::endl;
            return 1;
        }

        std::cout << "Loaded project: " << project.projectName().toStdString() 
                  << " (" << project.displayConfig().width << "x" << project.displayConfig().height << ")" << std::endl;

        if (target == "ugfx") {
            std::cout << "Generating uGFX C project in: " << outDir.toStdString() << std::endl;
            UgfxGenerator generator(&project, &scene);
            if (!generator.generate(outDir)) {
                std::cerr << "Export Error: " << generator.lastError().toStdString() << std::endl;
                return 1;
            }
        } else if (target == "qul") {
            std::cout << "Generating Qt for MCUs (QUL) project in: " << outDir.toStdString() << std::endl;
            QtMcuGenerator generator(&project, &scene);
            if (!generator.generate(outDir)) {
                std::cerr << "Export Error: " << generator.lastError().toStdString() << std::endl;
                return 1;
            }
        } else if (target == "lvgl") {
            std::cout << "Generating LVGL C project in: " << outDir.toStdString() << std::endl;
            LvglGenerator generator(&project, &scene);
            if (!generator.generate(outDir)) {
                std::cerr << "Export Error: " << generator.lastError().toStdString() << std::endl;
                return 1;
            }
        }

        std::cout << "Successfully exported " << target.toStdString() << " project to: " 
                  << QDir(outDir).absolutePath().toStdString() << std::endl;
        return 0;
    }

    // -------------------------------------------------------------------------
    // 2. Feature 1 Verification Mode (Named Color Styles)
    // -------------------------------------------------------------------------
    if (parser.isSet(verifyF1Option)) {
        QString outPath = parser.value(verifyF1Option);
        MainWindow window;
        window.resize(1380, 880);
        window.show();
        app.processEvents();

        // Step 1: Ensure "Primary" style is created with #2196F3
        window.project()->addColorStyle({ "Primary", QColor("#2196F3") });

        // Step 2: Apply to two different buttons' fill color
        auto btn1 = new ButtonComponent("btn_save");
        btn1->setText("SAVE SETTINGS");
        btn1->setCompPos(70, 60);
        btn1->setCompSize(180, 48);
        btn1->setColorStyleRef("backgroundColor", "Primary");
        btn1->setBackgroundColor(window.project()->resolveColor("Primary"));
        window.canvasScene()->addUIComponent(btn1);

        auto btn2 = new ButtonComponent("btn_submit");
        btn2->setText("SUBMIT ACTION");
        btn2->setCompPos(70, 130);
        btn2->setCompSize(180, 48);
        btn2->setColorStyleRef("backgroundColor", "Primary");
        btn2->setBackgroundColor(window.project()->resolveColor("Primary"));
        window.canvasScene()->addUIComponent(btn2);

        btn1->setSelected(true);
        window.propertiesPanel()->setTargetComponent(btn1);
        app.processEvents();

        // Step 3: Change "Primary" style to #FF5722 in Styles panel
        window.project()->updateColorStyle("Primary", QColor("#FF5722"));
        window.stylesPanel()->refreshStyles();
        window.propertiesPanel()->refreshValues();
        app.processEvents();

        // Step 4: Confirm both buttons turn orange immediately on canvas
        bool btn1Orange = (btn1->backgroundColor().name().toUpper() == "#FF5722");
        bool btn2Orange = (btn2->backgroundColor().name().toUpper() == "#FF5722");
        std::cout << "[VERIFY 1] btn1 color: " << btn1->backgroundColor().name().toStdString() 
                  << " (" << (btn1Orange ? "OK" : "FAIL") << ")" << std::endl;
        std::cout << "[VERIFY 1] btn2 color: " << btn2->backgroundColor().name().toStdString() 
                  << " (" << (btn2Orange ? "OK" : "FAIL") << ")" << std::endl;

        app.processEvents();
        QPixmap pixmap = window.grab();
        if (pixmap.save(outPath)) {
            std::cout << "[VERIFY 1] Screenshot saved to: " << outPath.toStdString() << std::endl;
            return (btn1Orange && btn2Orange) ? 0 : 1;
        } else {
            std::cerr << "[VERIFY 1] Failed to save screenshot to: " << outPath.toStdString() << std::endl;
            return 1;
        }
    }

    // -------------------------------------------------------------------------
    // 3. Feature 2 Verification Mode (Custom Components)
    // -------------------------------------------------------------------------
    if (parser.isSet(verifyF2Option)) {
        QString outPath = parser.value(verifyF2Option);
        MainWindow window;
        window.resize(1380, 880);
        window.show();
        app.processEvents();

        // Step 1: Define "MySlider" with rounded track + round thumb
        CustomComponentDefinition def;
        def.id = "custom_myslider_def";
        def.name = "MySlider";
        def.behaviorRole = "Slider";
        def.width = 240;
        def.height = 36;
        def.minValue = 0.0;
        def.maxValue = 100.0;
        def.defaultValue = 20.0;

        // Rounded track primitive
        PrimitiveShapeData trackPrim;
        trackPrim.shapeType = "Rectangle";
        trackPrim.role = "track";
        trackPrim.relX = 0;
        trackPrim.relY = 10;
        trackPrim.relWidth = 240;
        trackPrim.relHeight = 16;
        trackPrim.cornerRadius = 8;
        trackPrim.fillColor = QColor("#232936");
        trackPrim.strokeColor = QColor("#3b4455");
        trackPrim.strokeWidth = 1;
        def.primitives.append(trackPrim);

        // Round thumb primitive
        PrimitiveShapeData thumbPrim;
        thumbPrim.shapeType = "Circle";
        thumbPrim.role = "thumb";
        thumbPrim.relX = 20;
        thumbPrim.relY = 3;
        thumbPrim.relWidth = 30;
        thumbPrim.relHeight = 30;
        thumbPrim.cornerRadius = 15;
        thumbPrim.fillColor = QColor("#00E5FF");
        thumbPrim.strokeColor = QColor("#FFFFFF");
        thumbPrim.strokeWidth = 2;
        def.primitives.append(thumbPrim);

        // Add to project -> updates palette under "My Components"
        window.project()->addCustomComponentDefinition(def);
        window.palette()->setCustomComponents(window.project()->customComponentDefinitions());

        // Step 2: Drag/place two instances onto canvas
        auto inst1 = new CustomComponentInstance("slider_inst_1");
        inst1->setDefinition(def);
        inst1->setCompPos(70, 70);
        inst1->setCompSize(240, 36);
        inst1->setValue(25.0);
        window.canvasScene()->addUIComponent(inst1);

        auto inst2 = new CustomComponentInstance("slider_inst_2");
        inst2->setDefinition(def);
        inst2->setCompPos(70, 150);
        inst2->setCompSize(240, 36);
        inst2->setValue(80.0);
        window.canvasScene()->addUIComponent(inst2);

        // Step 3: Confirm value updates independently
        bool independent = (inst1->value() == 25.0 && inst2->value() == 80.0);
        std::cout << "[VERIFY 2] Instance 1 value: " << inst1->value() << std::endl;
        std::cout << "[VERIFY 2] Instance 2 value: " << inst2->value() << std::endl;
        std::cout << "[VERIFY 2] Independence check: " << (independent ? "OK" : "FAIL") << std::endl;

        inst2->setSelected(true);
        window.propertiesPanel()->setTargetComponent(inst2);
        app.processEvents();

        QPixmap pixmap = window.grab();
        if (pixmap.save(outPath)) {
            std::cout << "[VERIFY 2] Screenshot saved to: " << outPath.toStdString() << std::endl;
            return independent ? 0 : 1;
        } else {
            std::cerr << "[VERIFY 2] Failed to save screenshot to: " << outPath.toStdString() << std::endl;
            return 1;
        }
    }

    // -------------------------------------------------------------------------
    // 4. Interactive GUI Mode (QtWidgets MainWindow)
    // -------------------------------------------------------------------------
    MainWindow window;
    window.show();
    return app.exec();
}
