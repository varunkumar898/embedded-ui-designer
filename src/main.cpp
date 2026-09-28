#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFileInfo>
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <iostream>

#include "MainWindow.h"
#include "Project.h"
#include "CanvasScene.h"
#include "UgfxGenerator.h"
#include "QtMcuGenerator.h"

// Core MVVM Scaffolding
#include "WidgetModel.h"
#include "ScreenModel.h"
#include "DocumentModel.h"
#include "DeviceManager.h"
#include "FlashController.h"
#include "UgfxExporter.h"
#include "QtMcuExporter.h"

int main(int argc, char *argv[])
{
    // Check if CLI export mode is requested before creating QApplication
    bool isCliExport = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--export") == 0 || strcmp(argv[i], "-e") == 0) {
            isCliExport = true;
            break;
        }
    }

    // Set offscreen platform in headless CLI mode if no display environment is set
    if (isCliExport) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QApplication app(argc, argv);
    app.setApplicationName("EmbeddedUIDesigner");
    app.setApplicationDisplayName("Embedded UI Designer");
    app.setOrganizationName("EmbeddedDev");
    app.setOrganizationDomain("embeddeddev.org");
    app.setApplicationVersion("1.0.0");
    app.setWindowIcon(QIcon(":/packaging/embedded-ui-designer.png"));

    // Register C++ MVVM types with QML engine
    qmlRegisterType<DocumentModel>("EmbeddedUI", 1, 0, "DocumentModel");
    qmlRegisterUncreatableType<ScreenModel>("EmbeddedUI", 1, 0, "ScreenModel", "ScreenModel is instantiated by DocumentModel");
    qmlRegisterUncreatableType<WidgetModel>("EmbeddedUI", 1, 0, "WidgetModel", "WidgetModel is instantiated by DocumentModel");
    qmlRegisterType<DeviceManager>("EmbeddedUI", 1, 0, "DeviceManager");
    qmlRegisterType<FlashController>("EmbeddedUI", 1, 0, "FlashController");

    QCommandLineParser parser;
    parser.setApplicationDescription("Embedded UI Designer - Visual UI Designer and Code Generator for Microcontrollers");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption exportOption(
        QStringList() << "e" << "export",
        "Export target framework ('ugfx' or 'qul')",
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

    QCommandLineOption uiOption(
        QStringList() << "ui",
        "Select UI frontend ('qml' or 'widgets', default: 'qml')",
        "frontend",
        "qml"
    );
    parser.addOption(uiOption);

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

        if (target != "ugfx" && target != "qul") {
            std::cerr << "Error: Unknown export target '" << target.toStdString() 
                      << "'. Supported targets: 'ugfx', 'qul'." << std::endl;
            return 1;
        }

        // Load project into MVVM DocumentModel DOM
        DocumentModel docModel;
        if (!docModel.loadFromFile(projectPath)) {
            std::cerr << "Error: Failed to load project from: " << projectPath.toStdString() << std::endl;
            return 1;
        }

        std::cout << "Loaded project: " << docModel.projectName().toStdString() 
                  << " (" << docModel.screenWidth() << "x" << docModel.screenHeight() << ")" << std::endl;

        std::unique_ptr<IExporter> exporter;
        if (target == "ugfx") {
            std::cout << "Generating µGFX C project in: " << outDir.toStdString() << std::endl;
            exporter = std::make_unique<UgfxExporter>();
        } else if (target == "qul") {
            std::cout << "Generating Qt for MCUs (QUL) project in: " << outDir.toStdString() << std::endl;
            exporter = std::make_unique<QtMcuExporter>();
        }

        if (!exporter->exportProject(&docModel, outDir)) {
            std::cerr << "Export Error: " << exporter->lastError().toStdString() << std::endl;
            return 1;
        }

        std::cout << "Successfully exported " << target.toStdString() << " project to: " 
                  << QDir(outDir).absolutePath().toStdString() << std::endl;
        return 0;
    }

    // -------------------------------------------------------------------------
    // 2. Interactive GUI Mode (MVVM QML Frontend with Widgets Fallback)
    // -------------------------------------------------------------------------
    QString chosenUi = parser.value(uiOption).toLower().trimmed();

    if (chosenUi == "widgets") {
        MainWindow window;
        window.show();
        return app.exec();
    }

    // Default: Launch modern QML MVVM Interface
    DocumentModel docModel;
    docModel.loadSampleProject(); // Initialize with sample embedded dashboard

    DeviceManager deviceManager;
    FlashController flashController(&deviceManager);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("documentModel", &docModel);
    engine.rootContext()->setContextProperty("deviceManager", &deviceManager);
    engine.rootContext()->setContextProperty("flashController", &flashController);

    const QUrl url(QStringLiteral("qrc:/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        // Fallback to QtWidgets MainWindow if QML failed to load
        MainWindow window;
        window.show();
        return app.exec();
    }

    return app.exec();
}
