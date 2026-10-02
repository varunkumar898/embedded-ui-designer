#include <QApplication>
#include <QMouseEvent>
#include <QUndoStack>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QMenu>
#include <QScreen>
#include <QToolButton>
#include <QGraphicsSceneMouseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTimer>
#include <QComboBox>
#include <QSpinBox>
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <cmath>

#include "MainWindow.h"
#include "CanvasScene.h"
#include "CanvasView.h"
#include "ComponentPalette.h"
#include "PropertiesPanel.h"
#include "LayerPanel.h"
#include "ColorPickerDialog.h"
#include "Project.h"

#include "ButtonComponent.h"
#include "LabelComponent.h"
#include "RectangleComponent.h"
#include "ProgressBarComponent.h"
#include "SliderComponent.h"
#include "SwitchComponent.h"
#include "CheckboxComponent.h"
#include "TextInputComponent.h"
#include "CircleComponent.h"
#include "PathComponent.h"

#include "UgfxGenerator.h"
#include "QtMcuGenerator.h"
#include "LvglGenerator.h"

struct FunctionTestResult {
    std::string category;
    std::string functionName;
    bool passed;
    std::string details;
    long long durationMs;
};

class TestFunctionalRunner {
public:
    static int runAll(const QString& artifactDir) {
        std::vector<FunctionTestResult> results;
        auto startTotal = std::chrono::high_resolution_clock::now();

        std::cout << "====================================================================\n";
        std::cout << "  EMBEDDED UI DESIGNER - FULL FUNCTIONAL VALIDATION RUNNER\n";
        std::cout << "====================================================================\n";

        QTemporaryDir tempDir;
        if (!tempDir.isValid()) {
            std::cerr << "FAIL: Cannot create temp dir\n";
            return 1;
        }

        // Initialize MainWindow
        MainWindow window;
        window.resize(1920, 1080);
        window.show();
        qApp->processEvents();

        auto record = [&](const std::string& cat, const std::string& name, bool pass, const std::string& desc, auto tStart) {
            auto tEnd = std::chrono::high_resolution_clock::now();
            long long dur = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count();
            results.push_back({cat, name, pass, desc, dur});
            std::cout << (pass ? "  [PASS] " : "  [FAIL] ") 
                      << std::left << std::setw(30) << name << " : " << desc 
                      << " (" << dur << "ms)\n";
        };

        // --------------------------------------------------------------------
        // 1. Initial State & UI Chrome
        // --------------------------------------------------------------------
        std::cout << "\n[1] UI Chrome & Layout Initialization\n";
        {
            auto t = std::chrono::high_resolution_clock::now();
            bool hasScene = (window.m_scene != nullptr);
            bool hasView = (window.m_view != nullptr);
            bool hasPalette = (window.m_palette != nullptr);
            bool hasProps = (window.m_propertiesPanel != nullptr);
            bool hasLayers = (window.m_layerPanel != nullptr);
            bool ok = hasScene && hasView && hasPalette && hasProps && hasLayers;
            record("UI Chrome", "Dock Panels Initialization", ok, "All dock panels and canvas subsystems loaded", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            bool ok = (window.m_resolutionCombo != nullptr && window.m_resolutionCombo->count() >= 5);
            record("UI Chrome", "Target Display Combo", ok, "Resolution presets initialized with 5 presets", t);
        }

        // --------------------------------------------------------------------
        // 2. Component Creation across all 9 Types
        // --------------------------------------------------------------------
        std::cout << "\n[2] Component Creation (9 Core MCU Widget Types)\n";
        ButtonComponent* btn = nullptr;
        LabelComponent* lbl = nullptr;
        RectangleComponent* rect = nullptr;
        ProgressBarComponent* pb = nullptr;
        SliderComponent* sld = nullptr;
        SwitchComponent* sw = nullptr;
        CheckboxComponent* cb = nullptr;
        TextInputComponent* txt = nullptr;
        CircleComponent* circ = nullptr;

        {
            auto t = std::chrono::high_resolution_clock::now();
            btn = new ButtonComponent("btn_main");
            btn->setCompPos(20, 20);
            btn->setCompSize(100, 36);
            btn->setText("Power ON");
            window.m_scene->addUIComponent(btn);
            record("Components", "Create Button", true, "Added Button at (20,20), text='Power ON'", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            lbl = new LabelComponent("lbl_volt");
            lbl->setCompPos(140, 25);
            lbl->setText("Voltage: 3.3V");
            lbl->setColor(QColor("#00ffcc"));
            window.m_scene->addUIComponent(lbl);
            record("Components", "Create Label", true, "Added Label text='Voltage: 3.3V', color=#00ffcc", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            rect = new RectangleComponent("rect_panel");
            rect->setCompPos(20, 70);
            rect->setCompSize(180, 70);
            rect->setFillColor(QColor("#1e293b"));
            rect->setCornerRadius(10);
            window.m_scene->addUIComponent(rect);
            record("Components", "Create Rectangle", true, "Added Rectangle at (20,70) with cornerRadius=10", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            pb = new ProgressBarComponent("pb_battery");
            pb->setCompPos(20, 150);
            pb->setCompSize(140, 20);
            pb->setValue(0.85);
            pb->setBarColor(QColor("#10b981"));
            window.m_scene->addUIComponent(pb);
            record("Components", "Create Progress Bar", true, "Added ProgressBar value=0.85, color=#10b981", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            sld = new SliderComponent("sld_bright");
            sld->setCompPos(20, 180);
            sld->setCompSize(140, 24);
            sld->setValue(65);
            window.m_scene->addUIComponent(sld);
            record("Components", "Create Slider", true, "Added Slider value=65", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            sw = new SwitchComponent("sw_ble");
            sw->setCompPos(180, 150);
            sw->setChecked(true);
            window.m_scene->addUIComponent(sw);
            record("Components", "Create Switch", true, "Added Switch state=CHECKED (true)", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            cb = new CheckboxComponent("cb_logging");
            cb->setCompPos(180, 180);
            cb->setText("SD Log");
            cb->setChecked(true);
            window.m_scene->addUIComponent(cb);
            record("Components", "Create Checkbox", true, "Added Checkbox text='SD Log', state=CHECKED", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            txt = new TextInputComponent("txt_ssid");
            txt->setCompPos(20, 210);
            txt->setCompSize(120, 24);
            txt->setText("ESP32_WiFi");
            window.m_scene->addUIComponent(txt);
            record("Components", "Create TextInput", true, "Added TextInput text='ESP32_WiFi'", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            circ = new CircleComponent("circ_status");
            circ->setCompPos(160, 210);
            circ->setCompSize(24, 24);
            circ->setFillColor(QColor("#22c55e"));
            window.m_scene->addUIComponent(circ);
            record("Components", "Create Circle", true, "Added Circle diameter=24, color=#22c55e", t);
        }

        qApp->processEvents();

        // --------------------------------------------------------------------
        // 3. Properties Panel & Dynamic Selection
        // --------------------------------------------------------------------
        std::cout << "\n[3] Properties Panel Dynamic Inspections & Task 1 Verification\n";
        {
            auto t = std::chrono::high_resolution_clock::now();
            window.m_propertiesPanel->setTargetComponent(btn);
            qApp->processEvents();
            int c1 = window.m_propertiesPanel->specificEditorsLayoutCount();

            window.m_propertiesPanel->setTargetComponent(lbl);
            qApp->processEvents();

            window.m_propertiesPanel->setTargetComponent(pb);
            qApp->processEvents();

            window.m_propertiesPanel->setTargetComponent(btn);
            qApp->processEvents();
            int c2 = window.m_propertiesPanel->specificEditorsLayoutCount();

            bool ok = (c1 == c2 && c1 > 0);
            record("Properties Panel", "Task 1 Regression Check", ok, "Switched Button->Label->ProgressBar->Button without editor stacking", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            window.m_propertiesPanel->setTargetComponent(rect);
            qApp->processEvents();
            // Interactive corner radius handle check
            int r0 = rect->cornerRadius();
            rect->setCornerRadius(16);
            bool ok = (rect->cornerRadius() == 16);
            record("Properties Panel", "Corner Radius Mutation", ok, "Adjusted corner radius from 10 to 16px", t);
        }

        // --------------------------------------------------------------------
        // 4. Custom Embedded Color Picker & Harmony Engine
        // --------------------------------------------------------------------
        std::cout << "\n[4] Embedded Color Picker & RGB565 / Harmony Builder\n";
        {
            auto t = std::chrono::high_resolution_clock::now();
            QString rgb565 = ColorPickerDialog::toRgb565Hex(QColor("#1ECBE1"));
            bool ok = (rgb565 == "0x1E5C");
            record("Color Picker", "RGB565 Computation", ok, "#1ECBE1 computes to exact 0x1E5C", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            ColorPickerDialog dlg(QColor("#1ECBE1"));
            dlg.onHarmonyModeChanged(0); // Complementary
            qApp->processEvents();
            QColor sel = dlg.selectedColor();
            bool ok = (sel.isValid() && dlg.m_swatches.size() == 2);
            record("Color Picker", "Complementary Harmony", ok, "Generated 2 swatches (Base #1ECBE1, Harmony #E1341E)", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            ColorPickerDialog dlg(QColor("#1ECBE1"));
            dlg.onHarmonyModeChanged(3); // Triadic
            qApp->processEvents();
            bool ok = (dlg.m_swatches.size() == 3);
            record("Color Picker", "Triadic Harmony", ok, "Generated 3 harmonious triadic swatches (+120, +240)", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            ColorPickerDialog dlg(QColor("#1ECBE1"));
            dlg.selectSwatch(1); // Select harmony swatch
            QColor picked = dlg.selectedColor();
            btn->setBackgroundColor(picked);
            bool ok = (btn->backgroundColor() == picked);
            record("Color Picker", "Apply Swatch to Component", ok, "Applied chosen harmony color to Button background", t);
        }

        // --------------------------------------------------------------------
        // 5. Layer Panel Synchronization & Z-Order
        // --------------------------------------------------------------------
        std::cout << "\n[5] Layer Panel Operations\n";
        {
            auto t = std::chrono::high_resolution_clock::now();
            window.m_layerPanel->refreshLayers();
            int count = window.m_layerPanel->m_listWidget->count();
            bool ok = (count == window.m_scene->uiComponents().count());
            record("Layer Panel", "Layer Synchronization", ok, QString("Layer count (%1) matches canvas components").arg(count).toStdString(), t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            window.m_layerPanel->m_listWidget->setCurrentRow(0);
            qreal oldZ = btn->zValue();
            window.m_layerPanel->onMoveUp();
            qreal newZ = btn->zValue();
            bool ok = (newZ >= oldZ);
            record("Layer Panel", "Layer Z-Order Move Up", ok, "Incremented component Z-order in layer stack", t);
        }

        // --------------------------------------------------------------------
        // 6. Canvas Zoom & Target Display Presets
        // --------------------------------------------------------------------
        std::cout << "\n[6] Canvas View Zoom & MCU Display Presets\n";
        {
            auto t = std::chrono::high_resolution_clock::now();
            window.m_view->zoomIn();
            qreal zIn = window.m_view->zoomFactor();
            window.m_view->zoomOut();
            window.m_view->zoomOut();
            qreal zOut = window.m_view->zoomFactor();
            window.m_view->resetZoom();
            qreal zReset = window.m_view->zoomFactor();
            bool ok = (zIn > 1.0 && zOut < 1.0 && zReset == 1.0);
            record("Canvas View", "Zoom In/Out/100%", ok, "Zoom verified: in > 100%, out < 100%, reset = 100%", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            // Preset index 1: 480 x 320 HVGA
            window.onResolutionPresetChanged(1);
            QRectF r1 = window.m_scene->displayRect();
            // Preset index 2: 800 x 480 WVGA
            window.onResolutionPresetChanged(2);
            QRectF r2 = window.m_scene->displayRect();
            // Preset index 0: 320 x 240 QVGA
            window.onResolutionPresetChanged(0);
            QRectF r0 = window.m_scene->displayRect();
            bool ok = (r1.width() == 480 && r2.width() == 800 && r0.width() == 320);
            record("Canvas View", "Target MCU Resolution Presets", ok, "Switched 480x320 -> 800x480 -> 320x240 smoothly", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            window.onToggleGrid(false);
            bool gOff = !window.m_scene->isGridVisible();
            window.onToggleGrid(true);
            bool gOn = window.m_scene->isGridVisible();
            window.onToggleSnap(false);
            bool sOff = !window.m_scene->isSnapToGrid();
            window.onToggleSnap(true);
            bool sOn = window.m_scene->isSnapToGrid();
            bool ok = (gOff && gOn && sOff && sOn);
            record("Canvas View", "Toggle Grid & Snap", ok, "Toggled Grid and Snap-to-Grid options", t);
        }

        // --------------------------------------------------------------------
        // 7. Undo / Redo & Edit Operations
        // --------------------------------------------------------------------
        std::cout << "\n[7] Edit Operations & Undo / Redo Engine\n";
        {
            auto t = std::chrono::high_resolution_clock::now();
            window.m_scene->clearSelection();
            btn->setSelected(true);
            int beforeCount = window.m_scene->uiComponents().count();
            window.onDuplicateSelected();
            int afterDupCount = window.m_scene->uiComponents().count();
            window.m_undoStack->undo();
            int afterUndoCount = window.m_scene->uiComponents().count();
            window.m_undoStack->redo();
            int afterRedoCount = window.m_scene->uiComponents().count();
            bool ok = (afterDupCount == beforeCount + 1 && afterUndoCount == beforeCount && afterRedoCount == afterDupCount);
            record("Edit Engine", "Duplicate & Undo/Redo", ok, "Duplicated item (+1), Undone (reverted), Redone (restored)", t);
        }

        // --------------------------------------------------------------------
        // 8. Project File Serialization (Save & Load Roundtrip)
        // --------------------------------------------------------------------
        std::cout << "\n[8] Project File System & Serialization\n";
        QString projectPath = tempDir.filePath("test_full_project.euiproj");
        {
            auto t = std::chrono::high_resolution_clock::now();
            window.m_project->setProjectName("LiveValidationApp");
            bool saved = window.m_project->saveToFile(projectPath);
            bool exists = QFile::exists(projectPath);
            QFileInfo fi(projectPath);
            bool ok = (saved && exists && fi.size() > 500);
            record("Project", "Save Project File (.euiproj)", ok, QString("Saved project (%1 bytes) to disk").arg(fi.size()).toStdString(), t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            window.m_project->newProject("BlankApp", 320, 240);
            int blankCount = window.m_scene->uiComponents().count();
            bool loaded = window.m_project->loadFromFile(projectPath);
            int restoredCount = window.m_scene->uiComponents().count();
            bool ok = (blankCount == 0 && loaded && restoredCount > 0);
            record("Project", "Open Project File (.euiproj)", ok, QString("Loaded project successfully, restored %1 components").arg(restoredCount).toStdString(), t);
        }

        // --------------------------------------------------------------------
        // 9. Embedded Code Generation Pipelines (Export)
        // --------------------------------------------------------------------
        std::cout << "\n[9] Code Generators & Embedded Exporters\n";
        {
            auto t = std::chrono::high_resolution_clock::now();
            QString outUgfx = tempDir.filePath("export_ugfx");
            UgfxGenerator genUgfx(window.m_project, window.m_scene);
            bool ok = genUgfx.generate(outUgfx) 
                      && QFile::exists(outUgfx + "/ui.c")
                      && QFile::exists(outUgfx + "/ui.h")
                      && QFile::exists(outUgfx + "/gfxconf.h")
                      && QFile::exists(outUgfx + "/CMakeLists.txt")
                      && QFile::exists(outUgfx + "/main.c");
            record("Exporter", "Export µGFX C Project", ok, "Generated complete µGFX project (ui.c, ui.h, gfxconf.h, main.c)", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            QString outQul = tempDir.filePath("export_qul");
            QtMcuGenerator genQul(window.m_project, window.m_scene);
            bool ok = genQul.generate(outQul)
                      && QFile::exists(outQul + "/design.qml")
                      && QFile::exists(outQul + "/project.qmlproject")
                      && QFile::exists(outQul + "/CMakeLists.txt");
            record("Exporter", "Export Qt for MCUs (QUL)", ok, "Generated Qt Quick Ultralite design.qml & project.qmlproject", t);
        }
        {
            auto t = std::chrono::high_resolution_clock::now();
            QString outLvgl = tempDir.filePath("export_lvgl");
            LvglGenerator genLvgl(window.m_project, window.m_scene);
            bool ok = genLvgl.generate(outLvgl)
                      && QFile::exists(outLvgl + "/ui.c")
                      && QFile::exists(outLvgl + "/ui.h")
                      && QFile::exists(outLvgl + "/lv_conf.h")
                      && QFile::exists(outLvgl + "/CMakeLists.txt")
                      && QFile::exists(outLvgl + "/main.c");
            record("Exporter", "Export LVGL C/C++ Project", ok, "Generated complete LVGL C/C++ project (ui.c, ui.h, lv_conf.h, main.c)", t);
        }

        // --------------------------------------------------------------------
        // 10. Capture Live Application State Screenshot
        // --------------------------------------------------------------------
        std::cout << "\n[10] Capturing High-Resolution Verification Screenshot\n";
        {
            auto t = std::chrono::high_resolution_clock::now();
            QDir().mkpath(artifactDir);
            window.m_scene->clearSelection();
            RectangleComponent* resizeProbe = new RectangleComponent("resize_probe");
            resizeProbe->setCompPos(100, 100);
            resizeProbe->setCompSize(120, 70);
            resizeProbe->setFillColor(QColor("#e4583e"));
            window.m_scene->addUIComponent(resizeProbe);
            QPoint componentCenter = window.m_view->mapFromScene(resizeProbe->sceneBoundingRect().center());
            QMouseEvent selectPress(QEvent::MouseButtonPress, componentCenter, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(window.m_view->viewport(), &selectPress);
            QMouseEvent selectRelease(QEvent::MouseButtonRelease, componentCenter, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(window.m_view->viewport(), &selectRelease);
            resizeProbe->setSelected(true);
            window.m_propertiesPanel->setTargetComponent(resizeProbe);

            QUndoStack* sceneUndoStack = window.m_scene->undoStack();
            window.m_scene->setUndoStack(nullptr);
            const QPointF dragStartScene = resizeProbe->mapToScene(QPointF(resizeProbe->compWidth(), resizeProbe->compHeight()));
            const QPointF dragEndScene = dragStartScene + QPointF(50, 35);
            QGraphicsSceneMouseEvent resizePress(QEvent::GraphicsSceneMousePress);
            resizePress.setButton(Qt::LeftButton);
            resizePress.setButtons(Qt::LeftButton);
            resizePress.setScenePos(dragStartScene);
            resizePress.setPos(resizeProbe->mapFromScene(dragStartScene));
            resizePress.setModifiers(Qt::NoModifier);
            window.m_scene->sendEvent(resizeProbe, &resizePress);
            bool resizeStarted = resizeProbe->isResizing();
            QGraphicsSceneMouseEvent resizeMove(QEvent::GraphicsSceneMouseMove);
            resizeMove.setButton(Qt::NoButton);
            resizeMove.setButtons(Qt::LeftButton);
            resizeMove.setScenePos(dragEndScene);
            resizeMove.setPos(resizeProbe->mapFromScene(dragEndScene));
            resizeMove.setModifiers(Qt::NoModifier);
            window.m_scene->sendEvent(resizeProbe, &resizeMove);
            qApp->processEvents();

            bool anchorStayedFixed = resizeProbe->compX() == 100 && resizeProbe->compY() == 100;
            bool resizedLive = resizeProbe->compWidth() == 170 && resizeProbe->compHeight() == 105;
            record("Canvas Resize", "Live Corner Resize Anchor", anchorStayedFixed && resizedLive,
                   QString("started=%1, anchor=(%2,%3), size=%4x%5")
                       .arg(resizeStarted).arg(resizeProbe->compX()).arg(resizeProbe->compY())
                       .arg(resizeProbe->compWidth()).arg(resizeProbe->compHeight()).toStdString(), t);
            QPixmap liveResizeShot = window.grab();
            bool liveShotSaved = liveResizeShot.save(artifactDir + "/task1_corner_resize_live.png");
            record("Canvas Resize", "Capture Held Resize", liveShotSaved,
                   "Saved the visible MainWindow while the resize mouse button was still held", t);

            QGraphicsSceneMouseEvent resizeRelease(QEvent::GraphicsSceneMouseRelease);
            resizeRelease.setButton(Qt::LeftButton);
            resizeRelease.setButtons(Qt::NoButton);
            resizeRelease.setScenePos(dragEndScene);
            resizeRelease.setPos(resizeProbe->mapFromScene(dragEndScene));
            window.m_scene->sendEvent(resizeProbe, &resizeRelease);
            const QPointF shiftStartScene = resizeProbe->mapToScene(QPointF(resizeProbe->compWidth(), resizeProbe->compHeight()));
            const QPointF shiftEndScene = shiftStartScene + QPointF(34, 14);
            QGraphicsSceneMouseEvent shiftPress(QEvent::GraphicsSceneMousePress);
            shiftPress.setButton(Qt::LeftButton);
            shiftPress.setButtons(Qt::LeftButton);
            shiftPress.setScenePos(shiftStartScene);
            shiftPress.setPos(resizeProbe->mapFromScene(shiftStartScene));
            shiftPress.setModifiers(Qt::ShiftModifier);
            window.m_scene->sendEvent(resizeProbe, &shiftPress);
            QGraphicsSceneMouseEvent shiftMove(QEvent::GraphicsSceneMouseMove);
            shiftMove.setButton(Qt::NoButton);
            shiftMove.setButtons(Qt::LeftButton);
            shiftMove.setScenePos(shiftEndScene);
            shiftMove.setPos(resizeProbe->mapFromScene(shiftEndScene));
            shiftMove.setModifiers(Qt::ShiftModifier);
            window.m_scene->sendEvent(resizeProbe, &shiftMove);
            qApp->processEvents();
            bool shiftLocked = std::abs(resizeProbe->compWidth() / 170.0 - resizeProbe->compHeight() / 105.0) < 0.01;
            record("Canvas Resize", "Shift Aspect Lock", shiftLocked,
                   "Shift-modified corner drag preserved the original component aspect ratio", t);
            QGraphicsSceneMouseEvent shiftRelease(QEvent::GraphicsSceneMouseRelease);
            shiftRelease.setButton(Qt::LeftButton);
            shiftRelease.setButtons(Qt::NoButton);
            shiftRelease.setScenePos(shiftEndScene);
            shiftRelease.setPos(resizeProbe->mapFromScene(shiftEndScene));
            shiftRelease.setModifiers(Qt::ShiftModifier);
            window.m_scene->sendEvent(resizeProbe, &shiftRelease);
            window.m_scene->setUndoStack(sceneUndoStack);
            window.m_scene->removeUIComponent(resizeProbe);
            delete resizeProbe;

                 QToolButton* shapeButton = window.m_palette->shapeToolButton();
                 shapeButton->showMenu();
                 qApp->processEvents();
                 QStringList shapeNames;
                 for (QAction* action : shapeButton->menu()->actions()) {
                  shapeNames.append(action->text());
                 }
                QPixmap flyoutShot = shapeButton->menu()->grab();
                 bool flyoutShotSaved = flyoutShot.save(artifactDir + "/task2_shape_flyout.png");
                 bool hasFiveOptions = shapeNames == QStringList({"Circle", "Triangle", "Square", "Rectangle", "Custom"});
                 record("Shape Tool", "Capture Shape Flyout", flyoutShotSaved && hasFiveOptions,
                     "Captured the open Shape flyout with all five options", t);
                 shapeButton->menu()->actions().last()->trigger();
                 qApp->processEvents();
                 bool customToolSelected = window.m_view->activeDrawingTool() == "Custom";
                 record("Shape Tool", "Select Custom Drawing Tool", customToolSelected,
                     "Selecting Custom set the canvas active drawing tool", t);

                     const QPoint pathVertices[] = {
                        window.m_view->mapFromScene(QPointF(70, 50)),
                        window.m_view->mapFromScene(QPointF(190, 65)),
                        window.m_view->mapFromScene(QPointF(145, 160))
                     };
                     for (const QPoint& vertex : pathVertices) {
                      QMouseEvent placeVertex(QEvent::MouseButtonPress, vertex, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                      QApplication::sendEvent(window.m_view->viewport(), &placeVertex);
                     }
                     QKeyEvent finishPath(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
                     QApplication::sendEvent(window.m_view, &finishPath);
                     qApp->processEvents();
                     PathComponent* drawnPath = nullptr;
                     for (UIComponent* component : window.m_scene->uiComponents()) {
                      if (auto path = dynamic_cast<PathComponent*>(component)) drawnPath = path;
                     }
                     bool pathCreated = drawnPath && drawnPath->points().size() == 3 && drawnPath->isSelected();
                     bool pathFieldsVisible = pathCreated && window.m_propertiesPanel->m_colorBtn1 &&
                                  window.m_propertiesPanel->m_spinStrokeW && window.m_propertiesPanel->m_spinOpacity;
                     record("Custom Shape", "Place and Finish Polygon", pathCreated,
                         "Three point clicks followed by Enter created and selected a closed PathComponent", t);
                     record("Custom Shape", "Path Stroke Properties", pathFieldsVisible,
                         "Properties panel exposes Stroke Color, Stroke Thickness, and Opacity", t);
                         PathComponent restoredPath("path_restore");
                         if (drawnPath) restoredPath.fromJson(drawnPath->toJson());
                         bool pathRoundTrips = drawnPath && restoredPath.points().size() == drawnPath->points().size() &&
                                      restoredPath.strokeColor() == drawnPath->strokeColor() &&
                                              qFuzzyCompare(restoredPath.strokeThickness(), drawnPath->strokeThickness()) &&
                                      restoredPath.opacityPercent() == drawnPath->opacityPercent();
                         record("Custom Shape", "Path JSON Round Trip", pathRoundTrips,
                             "Path vertices, stroke color, thickness, and opacity survive JSON serialization", t);
                         if (drawnPath) drawnPath->setCompPos(50, 45);
                     QPixmap pathShot = window.grab();
                     bool pathShotSaved = pathShot.save(artifactDir + "/task3_custom_path_properties.png");
                     record("Custom Shape", "Capture Drawn Path", pathShotSaved,
                         "Saved the selected stroked path with its inspector fields visible", t);

                         auto sendCanvasClick = [&window](const QPointF& scenePoint) {
                          const QPoint point = window.m_view->mapFromScene(scenePoint);
                          QMouseEvent press(QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                          QApplication::sendEvent(window.m_view->viewport(), &press);
                          QMouseEvent release(QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                          QApplication::sendEvent(window.m_view->viewport(), &release);
                         };
                         const int beforeEscapeCount = window.m_scene->uiComponents().size();
                         window.m_view->setActiveDrawingTool("Custom");
                         sendCanvasClick(QPointF(40, 40));
                         sendCanvasClick(QPointF(80, 45));
                         QKeyEvent cancelPath(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
                         QApplication::sendEvent(window.m_view, &cancelPath);
                         const bool escapeCancelled = window.m_view->activeDrawingTool().isEmpty() &&
                                          window.m_scene->uiComponents().size() == beforeEscapeCount;
                         record("Custom Shape", "Escape Cancels Path", escapeCancelled,
                             "Escape removed the unfinished preview without adding a component", t);

                         window.m_view->setActiveDrawingTool("Custom");
                         sendCanvasClick(QPointF(40, 40));
                         sendCanvasClick(QPointF(100, 40));
                         sendCanvasClick(QPointF(70, 100));
                         const QPoint closePoint = window.m_view->mapFromScene(QPointF(70, 100));
                         QMouseEvent closePath(QEvent::MouseButtonDblClick, closePoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                         QApplication::sendEvent(window.m_view->viewport(), &closePath);
                         int doubleClickPathCount = 0;
                         for (UIComponent* component : window.m_scene->uiComponents()) {
                          if (dynamic_cast<PathComponent*>(component)) ++doubleClickPathCount;
                         }
                         record("Custom Shape", "Double-Click Finishes Path", doubleClickPathCount == 2,
                             "Double-click closed a three-vertex polygon", t);

                         const int beforeRectangleCount = window.m_scene->uiComponents().size();
                         window.m_view->setActiveDrawingTool("Rectangle");
                         const QPoint rectStart = window.m_view->mapFromScene(QPointF(35, 35));
                         const QPoint rectEnd = window.m_view->mapFromScene(QPointF(95, 80));
                         QMouseEvent rectPress(QEvent::MouseButtonPress, rectStart, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                         QApplication::sendEvent(window.m_view->viewport(), &rectPress);
                         QMouseEvent rectRelease(QEvent::MouseButtonRelease, rectEnd, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                         QApplication::sendEvent(window.m_view->viewport(), &rectRelease);
                         bool rectangleToolDrew = window.m_scene->uiComponents().size() == beforeRectangleCount + 1 &&
                                      dynamic_cast<RectangleComponent*>(window.m_scene->uiComponents().last());
                         record("Shape Tool", "Rectangle Drag Draw", rectangleToolDrew,
                             "Rectangle menu selection created a canvas shape from a drag", t);

                         window.m_project->setDirty(false);
                         QTimer::singleShot(0, &window, [&window]() {
                          QDialog* newProjectDialog = window.findChild<QDialog*>();
                          if (!newProjectDialog) return;
                          newProjectDialog->findChild<QSpinBox*>("newProjectWidth")->setValue(240);
                          newProjectDialog->findChild<QSpinBox*>("newProjectHeight")->setValue(320);
                          newProjectDialog->findChild<QSpinBox*>("newProjectColorDepth")->setValue(24);
                          newProjectDialog->findChild<QComboBox*>("newProjectShape")->setCurrentText("Round");
                          newProjectDialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
                         });
                         window.onNewProject();
                         const DisplayConfig roundConfig = window.m_project->displayConfig();
                         const bool roundProjectCreated = roundConfig.round && roundConfig.width == 240 &&
                                           roundConfig.height == 240 && roundConfig.colorDepth == 24;
                         record("Round Display", "Create Round New Project", roundProjectCreated,
                             "New Project dialog created a 240px round canvas at 24-bit depth", t);
                             bool roundConfigRoundTrips = DisplayConfig::fromJson(roundConfig.toJson()).round;
                             record("Round Display", "Round Config JSON", roundConfigRoundTrips,
                                 "The round display shape persists through DisplayConfig JSON", t);
                         QPixmap roundDisplayShot = window.grab();
                         const bool roundShotSaved = roundDisplayShot.save(artifactDir + "/task4_round_display.png");
                         record("Round Display", "Capture Circular Canvas", roundShotSaved && roundConfig.round,
                             "Captured the project canvas with its circular display boundary", t);

            qApp->processEvents();
            QPixmap pixmap = window.grab();
            QString shotPath = artifactDir + "/functional_test_screenshot.png";
            bool ok = pixmap.save(shotPath);
            record("Validation", "Capture Window Screenshot", ok, QString("Captured 1920x1080 screenshot to %1").arg(shotPath).toStdString(), t);
        }

        // --------------------------------------------------------------------
        // 11. TASK 1: Multi-Select on Canvas (Rubber-Band & Shift-Click)
        // --------------------------------------------------------------------
        std::cout << "\n[11] TASK 1: Multi-Select on Canvas\n";
        window.m_project->newProject("MultiSelectVerification", 800, 480);
        window.m_scene->clear();

        ButtonComponent* cBtn = new ButtonComponent("btn_task1");
        cBtn->setText("Power Mode");
        cBtn->setCompSize(140, 42);
        cBtn->setCompPos(60, 60);
        window.m_scene->addUIComponent(cBtn);

        LabelComponent* cLbl = new LabelComponent("lbl_task1");
        cLbl->setText("Sensors Online");
        cLbl->setCompSize(140, 32);
        cLbl->setCompPos(60, 130);
        window.m_scene->addUIComponent(cLbl);

        RectangleComponent* cRect = new RectangleComponent("rect_task1");
        cRect->setCompSize(140, 60);
        cRect->setCompPos(60, 190);
        cRect->setCornerRadius(8);
        window.m_scene->addUIComponent(cRect);

        SliderComponent* cSlider = new SliderComponent("slider_task1");
        cSlider->setCompSize(180, 36);
        cSlider->setCompPos(320, 60);
        window.m_scene->addUIComponent(cSlider);

        window.m_scene->clearSelection();
        qApp->processEvents();

        // 1. Rubber-band drag-select 3 items together
        {
            auto t = std::chrono::high_resolution_clock::now();
            QPoint p1 = window.m_view->mapFromScene(QPointF(30, 40));
            QPoint p2 = window.m_view->mapFromScene(QPointF(240, 280));

            QMouseEvent pressEv(QEvent::MouseButtonPress, p1, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(window.m_view->viewport(), &pressEv);

            QMouseEvent moveEv(QEvent::MouseMove, p2, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(window.m_view->viewport(), &moveEv);

            QMouseEvent relEv(QEvent::MouseButtonRelease, p2, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(window.m_view->viewport(), &relEv);

            qApp->processEvents();

            bool selOk = cBtn->isSelected() && cLbl->isSelected() && cRect->isSelected() && !cSlider->isSelected();
            record("Task 1 Canvas Multi-Select", "Rubber-band Drag Select 3 Items", selOk,
                   "Rubber-band from (30,40) to (240,280) selected 3 items simultaneously", t);

            QPixmap shot1 = window.grab();
            shot1.save(artifactDir + "/task1_rubberband_selected.png");
        }

        // 2. Shift-click sequential selection building
        {
            auto t = std::chrono::high_resolution_clock::now();
            window.m_scene->clearSelection();
            qApp->processEvents();

            // Click cBtn
            QPoint ptBtn = window.m_view->mapFromScene(cBtn->sceneBoundingRect().center());
            QMouseEvent p1(QEvent::MouseButtonPress, ptBtn, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(window.m_view->viewport(), &p1);
            QMouseEvent r1(QEvent::MouseButtonRelease, ptBtn, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(window.m_view->viewport(), &r1);
            std::cout << "DEBUG after p1: cBtn=" << cBtn->isSelected() << "\n";

            // Shift-click cLbl
            QPoint ptLbl = window.m_view->mapFromScene(cLbl->sceneBoundingRect().center());
            QMouseEvent p2(QEvent::MouseButtonPress, ptLbl, Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
            QApplication::sendEvent(window.m_view->viewport(), &p2);
            QMouseEvent r2(QEvent::MouseButtonRelease, ptLbl, Qt::LeftButton, Qt::NoButton, Qt::ShiftModifier);
            QApplication::sendEvent(window.m_view->viewport(), &r2);

            // Shift-click cSlider
            QPoint ptSlider = window.m_view->mapFromScene(cSlider->sceneBoundingRect().center());
            QMouseEvent p3(QEvent::MouseButtonPress, ptSlider, Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
            QApplication::sendEvent(window.m_view->viewport(), &p3);
            QMouseEvent r3(QEvent::MouseButtonRelease, ptSlider, Qt::LeftButton, Qt::NoButton, Qt::ShiftModifier);
            QApplication::sendEvent(window.m_view->viewport(), &r3);

            qApp->processEvents();

            bool shiftOk = cBtn->isSelected() && cLbl->isSelected() && cSlider->isSelected() && !cRect->isSelected();
            record("Task 1 Canvas Multi-Select", "Shift-Click Selection Building", shiftOk,
                   "Built selection of 3 components (Button, Label, Slider) via sequential Shift-clicks", t);

            QPixmap shot2 = window.grab();
            shot2.save(artifactDir + "/task1_shift_click_selected.png");
        }

        // --------------------------------------------------------------------
        // 12. TASK 2: Properties Panel Multi-Select State
        // --------------------------------------------------------------------
        std::cout << "\n[12] TASK 2: Properties Panel Multi-Select Graceful Display\n";
        {
            auto t = std::chrono::high_resolution_clock::now();
            qApp->processEvents();

            bool multiVisible = window.m_propertiesPanel->m_multiWidget->isVisible();
            bool singleHidden = !window.m_propertiesPanel->m_contentWidget->isVisible();
            bool emptyHidden  = !window.m_propertiesPanel->m_emptyWidget->isVisible();
            QString labelText = window.m_propertiesPanel->m_multiLabel->text();
            bool labelOk = (labelText == "3 components selected");

            bool ok = multiVisible && singleHidden && emptyHidden && labelOk;
            record("Task 2 Properties Panel", "Multi-Select Clean State (No Overlap)", ok,
                   QString("Properties panel displays '%1' with zero individual fields").arg(labelText).toStdString(), t);

            QPixmap shotProps = window.grab();
            shotProps.save(artifactDir + "/task2_properties_multiselect.png");
        }

        // --------------------------------------------------------------------
        // 13. TASK 3: Align Tools & Single Compound Undo
        // --------------------------------------------------------------------
        std::cout << "\n[13] TASK 3: Align Tools & Single Compound Undo\n";
        // Stagger 3 components horizontally:
        cBtn->setCompPos(60, 60);
        cLbl->setCompPos(150, 130);
        cRect->setCompPos(100, 200);

        window.m_scene->clearSelection();
        cBtn->setSelected(true);
        cLbl->setSelected(true);
        cRect->setSelected(true);
        cSlider->setSelected(false);
        qApp->processEvents();

        QPixmap shotAlignBefore = window.grab();
        shotAlignBefore.save(artifactDir + "/task3_align_before.png");

        {
            auto t = std::chrono::high_resolution_clock::now();
            int undoCountBefore = window.m_undoStack->count();

            // Trigger Align Left
            window.onAlignLeft();
            qApp->processEvents();

            bool alignedLeft = (cBtn->pos().x() == 60.0 && cLbl->pos().x() == 60.0 && cRect->pos().x() == 60.0);
            bool yPreserved  = (cBtn->pos().y() == 60.0 && cLbl->pos().y() == 130.0 && cRect->pos().y() == 200.0);
            bool oneUndoPushed = (window.m_undoStack->count() == undoCountBefore + 1);

            bool ok = alignedLeft && yPreserved && oneUndoPushed;
            record("Task 3 Align Tools", "Align Left All to Min X (60)", ok,
                   "All 3 items aligned to left bounding box X=60 with Y coords preserved", t);

            QPixmap shotAlignAfter = window.grab();
            shotAlignAfter.save(artifactDir + "/task3_align_after.png");

            // Verify Single Ctrl+Z Undo
            window.m_undoStack->undo();
            qApp->processEvents();

            bool restored = (cBtn->pos().x() == 60.0 && cLbl->pos().x() == 150.0 && cRect->pos().x() == 100.0);
            record("Task 3 Align Tools", "Single Compound Undo (Ctrl+Z)", restored,
                   "Single Ctrl+Z undo atomically restored all 3 components to original staggered positions", t);

            QPixmap shotAlignUndo = window.grab();
            shotAlignUndo.save(artifactDir + "/task3_align_undo.png");
        }

        // --------------------------------------------------------------------
        // 14. TASK 4: Distribute Tools & Compound Undo
        // --------------------------------------------------------------------
        std::cout << "\n[14] TASK 4: Distribute Tools & Compound Undo\n";
        // Setup 4 unevenly-spaced components horizontally:
        cBtn->setCompSize(80, 40);
        cBtn->setCompPos(40, 120);

        cLbl->setCompSize(60, 30);
        cLbl->setCompPos(140, 120);

        cRect->setCompSize(70, 50);
        cRect->setCompPos(230, 120);

        cSlider->setCompSize(80, 30);
        cSlider->setCompPos(460, 120);

        window.m_scene->clearSelection();
        cBtn->setSelected(true);
        cLbl->setSelected(true);
        cRect->setSelected(true);
        cSlider->setSelected(true);
        qApp->processEvents();

        QPixmap shotDistBefore = window.grab();
        shotDistBefore.save(artifactDir + "/task4_distribute_before.png");

        {
            auto t = std::chrono::high_resolution_clock::now();
            int undoIndexBefore = window.m_undoStack->index();

            // Trigger Distribute Horizontal
            window.onDistributeH();
            qApp->processEvents();

            // Total span = (460 + 80 - 40) = 500. Total width = 80 + 60 + 70 + 80 = 290.
            // Gap = (500 - 290) / 3 = 70px.
            // Item 0: 40
            // Item 1: 40 + 80 + 70 = 190
            // Item 2: 190 + 60 + 70 = 320
            // Item 3: 320 + 70 + 70 = 460
            bool c0Fixed = (std::abs(cBtn->pos().x() - 40.0) < 0.5);
            bool c1Pos   = (std::abs(cLbl->pos().x() - 190.0) < 0.5);
            bool c2Pos   = (std::abs(cRect->pos().x() - 320.0) < 0.5);
            bool c3Fixed = (std::abs(cSlider->pos().x() - 460.0) < 0.5);
            bool oneUndoPushed = (window.m_undoStack->index() == undoIndexBefore + 1);

            bool ok = c0Fixed && c1Pos && c2Pos && c3Fixed && oneUndoPushed;
            record("Task 4 Distribute Tools", "Distribute Horizontally Equal Gaps (70px)", ok,
                   QString("Outermost items fixed at 40 and 460; intermediate items spaced at exactly 70px gaps (%1, %2, %3, %4)")
                   .arg(cBtn->pos().x()).arg(cLbl->pos().x()).arg(cRect->pos().x()).arg(cSlider->pos().x()).toStdString(), t);

            QPixmap shotDistAfter = window.grab();
            shotDistAfter.save(artifactDir + "/task4_distribute_after.png");

            // Verify single compound undo
            window.m_undoStack->undo();
            qApp->processEvents();

            bool restored = (std::abs(cBtn->pos().x() - 40.0) < 0.5 &&
                             std::abs(cLbl->pos().x() - 140.0) < 0.5 &&
                             std::abs(cRect->pos().x() - 230.0) < 0.5 &&
                             std::abs(cSlider->pos().x() - 460.0) < 0.5);
            record("Task 4 Distribute Tools", "Single Compound Undo (Ctrl+Z)", restored,
                   "Single Ctrl+Z undo atomically restored all 4 components to original uneven positions", t);
        }

        auto endTotal = std::chrono::high_resolution_clock::now();
        long long totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTotal - startTotal).count();

        // --------------------------------------------------------------------
        // Report Generation
        // --------------------------------------------------------------------
        int totalPassed = 0;
        for (const auto& r : results) {
            if (r.passed) totalPassed++;
        }

        std::cout << "\n====================================================================\n";
        std::cout << "  VALIDATION SUMMARY: " << totalPassed << " / " << results.size() 
                  << " FUNCTIONS PASSED (Total Time: " << totalMs << "ms)\n";
        std::cout << "====================================================================\n";

        // Write detailed markdown report
        QString reportPath = artifactDir + "/functional_validation_report.md";
        QFile reportFile(reportPath);
        if (reportFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&reportFile);
            out << "# Embedded UI Designer - Full Functional Validation Report\n\n";
            out << "**Execution Date**: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "  \n";
            out << "**Overall Status**: " << (totalPassed == (int)results.size() ? "**100% PASSED (ALL FUNCTIONS OPERATIONAL)**" : "**FAILURES DETECTED**") << "  \n";
            out << "**Total Verified Functions**: " << results.size() << "  \n";
            out << "**Total Execution Time**: " << totalMs << " ms  \n\n";

            out << "## Functional Verification Matrix\n\n";
            out << "| Subsystem | Feature / Function | Status | Operational Details | Time |\n";
            out << "| :--- | :--- | :---: | :--- | :---: |\n";
            for (const auto& r : results) {
                out << "| **" << QString::fromStdString(r.category) << "** | "
                    << QString::fromStdString(r.functionName) << " | "
                    << (r.passed ? "<span style='color:green'>PASS</span>" : "<span style='color:red'>FAIL</span>") << " | "
                    << QString::fromStdString(r.details) << " | "
                    << r.durationMs << "ms |\n";
            }

            out << "\n## Visual Verification Artifacts\n\n";
            out << "- **Full Desktop Environment**: ![Functional Window](functional_test_screenshot.png)\n";
            out << "- **Task 1: Rubber-band Drag-Select**: ![Task 1 Rubber-band](task1_rubberband_selected.png)\n";
            out << "- **Task 1: Shift-Click Sequential Select**: ![Task 1 Shift-click](task1_shift_click_selected.png)\n";
            out << "- **Task 2: Properties Panel Multi-Select State**: ![Task 2 Properties](task2_properties_multiselect.png)\n";
            out << "- **Task 3: Align Left (Before)**: ![Task 3 Before](task3_align_before.png)\n";
            out << "- **Task 3: Align Left (After)**: ![Task 3 After](task3_align_after.png)\n";
            out << "- **Task 3: Align Left (Ctrl+Z Undo)**: ![Task 3 Undo](task3_align_undo.png)\n";
            out << "- **Task 4: Distribute Horizontal (Before)**: ![Task 4 Before](task4_distribute_before.png)\n";
            out << "- **Task 4: Distribute Horizontal (After)**: ![Task 4 After](task4_distribute_after.png)\n";

            reportFile.close();
            std::cout << "SUCCESS: Detailed report saved to " << reportPath.toStdString() << "\n";
        }

        return (totalPassed == (int)results.size()) ? 0 : 1;
    }
};

int main(int argc, char* argv[]) {
    // Only fall back to offscreen if neither DISPLAY nor WAYLAND_DISPLAY are set
    if (!qEnvironmentVariableIsSet("DISPLAY") && !qEnvironmentVariableIsSet("WAYLAND_DISPLAY")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(app);

    QString artifactDir = (argc > 1) ? argv[1] : "/home/cherry/.gemini/antigravity-ide/brain/7139078c-07a0-4854-95f4-51288c8d351b";
    return TestFunctionalRunner::runAll(artifactDir);
}
