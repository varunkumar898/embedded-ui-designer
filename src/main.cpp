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

        // Validate that the project file exists and is readable
        CanvasScene scene;
        Project project(&scene);
        if (!project.loadFromFile(projectPath)) {
            std::cerr << "Error: Failed to load project from: " << projectPath.toStdString() << std::endl;
            return 1;
        }

        std::cout << "Loaded project: " << project.projectName().toStdString() 
                  << " (" << project.displayConfig().width << "x" << project.displayConfig().height << ")" << std::endl;

        if (target == "ugfx") {
            std::cout << "Generating µGFX C project in: " << outDir.toStdString() << std::endl;
            UgfxGenerator generator(projectPath);
            if (!generator.generate(outDir)) {
                std::cerr << "Export Error: " << generator.lastError().toStdString() << std::endl;
                return 1;
            }
        } else if (target == "qul") {
            std::cout << "Generating Qt for MCUs (QUL) project in: " << outDir.toStdString() << std::endl;
            QtMcuGenerator generator(projectPath);
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
    // 2. Interactive GUI Mode (QtWidgets MainWindow)
    // -------------------------------------------------------------------------
    MainWindow window;
    window.show();
    return app.exec();
}
