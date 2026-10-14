#include "IRAdapter.h"
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
#include "GaugeComponent.h"
#include "SpeedometerComponent.h"
#include "BatteryComponent.h"
#include "PressureComponent.h"
#include "RpmComponent.h"
#include "TemperatureComponent.h"
#include "CircularProgressComponent.h"
#include "TabViewComponent.h"
#include "NavigationBarComponent.h"
#include "ListComponent.h"
#include "TableComponent.h"
#include "GroupComponent.h"
#include "CustomComponentInstance.h"
#include <QDebug>
#include <QFileInfo>
#include <QSet>

namespace IR {

// ============================================================================
// Type normalization helper
// ============================================================================

static QString normalizeComponentType(const QString& rawType) {
    QString t = rawType.trimmed().toLower();
    if (t == "button") return "Button";
    if (t == "label" || t == "text" || t == "p" || t == "span" || t == "h1" || t == "h2" || t == "h3") return "Label";
    if (t == "rectangle" || t == "rect" || t == "container" || t == "div" || t == "section") return "Rectangle";
    if (t == "progressbar" || t == "linearprogress" || t == "progress") return "ProgressBar";
    if (t == "circularprogress") return "CircularProgress";
    if (t == "gauge") return "Gauge";
    if (t == "speedometer") return "Speedometer";
    if (t == "battery") return "Battery";
    if (t == "pressure") return "Pressure";
    if (t == "rpm") return "RPM";
    if (t == "temperature") return "Temperature";
    if (t == "tabview") return "TabView";
    if (t == "navigationbar" || t == "navbar") return "NavigationBar";
    if (t == "list") return "List";
    if (t == "table") return "Table";
    if (t == "image" || t == "img") return "Image";
    if (t == "slider") return "Slider";
    if (t == "switch") return "Switch";
    if (t == "checkbox") return "Checkbox";
    if (t == "textinput" || t == "input" || t == "textarea") return "TextInput";
    if (t == "circle") return "Circle";
    if (t == "path") return "Path";
    if (t == "group") return "Group";
    if (t == "custominstance") return "CustomInstance";
    return rawType; // fallback to original
}

// ============================================================================
// ProjectToIRAdapter
// ============================================================================

IRProject ProjectToIRAdapter::convert(const Project& project) {
    IRProject ir;
    ir.version = IRVersion();
    ir.id = IRNodeId::generate();
    ir.name = project.projectName();
    ir.displayName = project.displayName();
    ir.targetFramework = project.targetFramework();

    // Hardware & Target Profile
    Hardware::HardwareConfig hw = project.hardwareConfig();
    ir.targetBoard = hw.deviceId;
    ir.targetFamily = hw.deviceId;
    ir.targetProfile.mcu = hw.deviceId;
    ir.targetProfile.framework = project.targetFramework();
    ir.targetProfile.maxFlash = hw.flashBytes;
    ir.targetProfile.maxRam = hw.ramBytes;

    // Display Config
    DisplayConfig disp = project.displayConfig();
    ir.displayWidth = disp.width;
    ir.displayHeight = disp.height;
    ir.colorDepth = disp.colorDepth;
    ir.colorFormat = disp.colorFormat;
    ir.isRound = disp.round;

    // Screens
    for (const Screen* screen : project.screens()) {
        if (screen) {
            ir.screens.append(convertScreen(*screen));
        }
    }
    if (project.activeScreen()) {
        ir.activeScreenId = IRNodeId::fromString(project.activeScreen()->id());
    } else if (!ir.screens.isEmpty()) {
        ir.activeScreenId = ir.screens.first().id;
    }

    // Data Sources
    for (const DataSource& ds : project.dataSources()) {
        ir.dataSources.append(convertDataSource(ds));
    }

    // Data Bindings
    for (const DataBinding& b : project.dataBindings()) {
        ir.bindings.append(convertDataBinding(b));
    }

    // Image Assets extraction
    QSet<QString> seenImagePaths;
    for (const Screen* screen : project.screens()) {
        if (!screen) continue;
        for (const UIComponent* comp : screen->components()) {
            if (const auto* img = dynamic_cast<const ImageComponent*>(comp)) {
                QString path = img->imagePath();
                if (!path.isEmpty() && !seenImagePaths.contains(path)) {
                    seenImagePaths.insert(path);
                    IRAsset asset;
                    asset.id = IRNodeId::generate();
                    asset.name = QFileInfo(path).fileName();
                    asset.type = IRAssetType::Image;
                    asset.filePath = path;
                    asset.originalPath = path;
                    asset.colorFormat = img->format();
                    QImage qimg = img->loadedImage();
                    if (!qimg.isNull()) {
                        asset.width = qimg.width();
                        asset.height = qimg.height();
                        asset.fileSize = QFileInfo(path).size();
                    }
                    ir.assets.append(asset);
                }
            }
        }
    }

    return ir;
}

IRScreen ProjectToIRAdapter::convertScreen(const Screen& screen) {
    IRScreen s;
    s.id = IRNodeId::fromString(screen.id());
    s.name = screen.name();
    s.width = screen.width();
    s.height = screen.height();
    s.backgroundColor.type = IRColor::Type::Solid;
    s.backgroundColor.solidColor = screen.backgroundColor();

    s.hGuides = screen.hGuides();
    s.vGuides = screen.vGuides();

    for (const UIComponent* comp : screen.components()) {
        if (comp) {
            s.widgets.append(convertComponent(*comp));
        }
    }

    // Screen-level data sources & bindings
    for (const DataSource& ds : screen.dataSources()) {
        s.provenance.originalProperties["hasScreenDataSources"] = true;
    }

    return s;
}

IRWidget ProjectToIRAdapter::convertComponent(const UIComponent& comp) {
    IRWidget w;
    w.id = IRNodeId::fromString(comp.componentId());
    w.componentType = comp.componentType();
    w.name = comp.componentId();

    // Layout
    w.layout.type = IRLayoutType::Absolute;
    w.layout.left = comp.compX();
    w.layout.hasLeft = true;
    w.layout.top = comp.compY();
    w.layout.hasTop = true;

    // Properties: capture full component JSON for complete fidelity
    w.properties = comp.toJson();
    w.customProperties = comp.toJson();

    // Specific component labels/text
    if (const auto* btn = dynamic_cast<const ButtonComponent*>(&comp)) {
        w.label = btn->text();
        w.textStyle.color.type = IRColor::Type::Solid;
        w.textStyle.color.solidColor = btn->textColor();
    } else if (const auto* lbl = dynamic_cast<const LabelComponent*>(&comp)) {
        w.label = lbl->text();
        w.textStyle.fontSize = lbl->pixelSize();
        w.textStyle.fontFamily = lbl->fontFamily();
        w.textStyle.color.type = IRColor::Type::Solid;
        w.textStyle.color.solidColor = lbl->color();
    } else if (const auto* txt = dynamic_cast<const TextInputComponent*>(&comp)) {
        w.label = txt->placeholder();
    } else if (const auto* chk = dynamic_cast<const CheckboxComponent*>(&comp)) {
        w.label = chk->text();
    }

    if (const auto* grp = dynamic_cast<const GroupComponent*>(&comp)) {
        for (const UIComponent* child : grp->children()) {
            if (child) {
                w.children.append(convertComponent(*child));
            }
        }
    } else if (const auto* img = dynamic_cast<const ImageComponent*>(&comp)) {
        w.properties["imagePath"] = img->imagePath();
        w.properties["format"] = img->format();
        w.properties["opacityPercent"] = img->opacityPercent();
        w.properties["scalingMode"] = img->scalingMode();
    }

    // Style & Effects
    w.effects = convertStyle(comp);

    // Component-level bindings
    for (const DataBinding& b : comp.bindings()) {
        w.bindings.append(convertDataBinding(b));
    }

    return w;
}

IREffects ProjectToIRAdapter::convertStyle(const UIComponent& comp) {
    IREffects effects;
    effects.opacity = comp.effectiveOpacity();
    effects.backgroundColor.type = IRColor::Type::Solid;
    effects.backgroundColor.solidColor = comp.effectiveBackgroundColor(Qt::transparent);

    effects.border.width = comp.effectiveBorderWidth(0);
    effects.border.color.type = IRColor::Type::Solid;
    effects.border.color.solidColor = comp.effectiveBorderColor(Qt::transparent);

    if (comp.hasCornerRadius()) {
        effects.borderRadius = comp.cornerRadius();
    }

    return effects;
}

IRDataSource ProjectToIRAdapter::convertDataSource(const DataSource& ds) {
    IRDataSource irDs;
    irDs.id = IRNodeId::fromString(ds.id());
    irDs.name = ds.name();
    irDs.type = IRDataSourceType::Hardware;
    irDs.hardwareRef = ds.hardwareRef();
    if (ds.metadata().contains("pollingIntervalMs")) {
        irDs.updateInterval = ds.metadata()["pollingIntervalMs"].toInt();
    }
    irDs.initialValue = ds.value();
    return irDs;
}

IRBinding ProjectToIRAdapter::convertDataBinding(const DataBinding& b) {
    IRBinding irB;
    irB.id = IRNodeId::generate();
    irB.sourceId = b.sourceId();
    irB.componentId = IRNodeId::fromString(b.componentId());
    irB.propertyPath = b.propertyName();
    irB.transformExpression = b.transformExpression();

    switch (b.direction()) {
        case BindingDirection::Read:      irB.direction = IRBindingDirection::Read; break;
        case BindingDirection::Write:     irB.direction = IRBindingDirection::Write; break;
        case BindingDirection::ReadWrite: irB.direction = IRBindingDirection::ReadWrite; break;
    }

    return irB;
}

// ============================================================================
// IRToProjectAdapter
// ============================================================================

std::unique_ptr<Project> IRToProjectAdapter::convert(const IRProject& ir, CanvasScene* scene) {
    auto project = std::make_unique<Project>(scene);
    populateProject(ir, project.get());
    return project;
}

void IRToProjectAdapter::populateProject(const IRProject& ir, Project* project) {
    if (!project) return;

    project->setProjectName(!ir.name.isEmpty() ? ir.name : ir.displayName);
    project->setDisplayName(!ir.displayName.isEmpty() ? ir.displayName : ir.name);
    project->setTargetFramework(!ir.targetFramework.isEmpty() ? ir.targetFramework : ir.targetProfile.framework);

    // Display
    DisplayConfig disp;
    disp.width = ir.displayWidth;
    disp.height = ir.displayHeight;
    disp.colorDepth = ir.colorDepth;
    disp.colorFormat = ir.colorFormat;
    disp.round = ir.isRound;
    project->setDisplayConfig(disp);

    // Hardware
    Hardware::HardwareConfig hw;
    hw.deviceId = !ir.targetBoard.isEmpty() ? ir.targetBoard : (!ir.targetProfile.mcu.isEmpty() ? ir.targetProfile.mcu : ir.targetFamily);
    hw.flashBytes = ir.targetProfile.maxFlash;
    hw.ramBytes = ir.targetProfile.maxRam;
    project->setHardwareConfig(hw);

    // Data Sources
    QList<DataSource> sources;
    for (const IRDataSource& ds : ir.dataSources) {
        sources.append(convertDataSource(ds));
    }
    project->setDataSources(sources);

    // Data Bindings
    QList<DataBinding> bindings;
    for (const IRBinding& b : ir.bindings) {
        bindings.append(convertDataBinding(b));
    }
    project->setDataBindings(bindings);

    // Screens: replace initial default screens with IR screens
    QList<QString> oldScreenIds;
    for (Screen* s : project->screens()) {
        oldScreenIds.append(s->id());
    }

    for (const IRScreen& s : ir.screens) {
        Screen* screen = convertScreen(s, project);
        project->addScreen(screen);
    }

    if (!ir.screens.isEmpty()) {
        for (const QString& oldId : oldScreenIds) {
            project->removeScreen(oldId);
        }
        QString activeId = ir.activeScreenId.isValid() ? ir.activeScreenId.toString() : ir.screens.first().id.toString();
        project->setActiveScreenById(activeId);
    }
}

Screen* IRToProjectAdapter::convertScreen(const IRScreen& irScreen, QObject* parent) {
    Screen* s = new Screen(irScreen.id.toString(), irScreen.name, parent);
    s->setWidth(irScreen.width);
    s->setHeight(irScreen.height);
    if (irScreen.backgroundColor.isValid()) {
        s->setBackgroundColor(irScreen.backgroundColor.solidColor);
    }
    s->setGuides(irScreen.hGuides, irScreen.vGuides);

    for (const IRWidget& w : irScreen.widgets) {
        UIComponent* comp = convertComponent(w);
        if (comp) {
            comp->setScreenId(s->id());
            s->addComponent(comp);
        }
    }

    return s;
}

UIComponent* IRToProjectAdapter::convertComponent(const IRWidget& irWidget) {
    QString normalizedType = normalizeComponentType(irWidget.componentType);
    QString id = irWidget.id.toString();

    UIComponent* comp = nullptr;
    if (normalizedType == "Group" || !irWidget.children.isEmpty()) {
        auto* grp = new GroupComponent(id);
        for (const auto& childWidget : irWidget.children) {
            UIComponent* childComp = convertComponent(childWidget);
            if (childComp) {
                grp->addChild(childComp);
            }
        }
        comp = grp;
    } else {
        comp = Project::createComponentInstance(normalizedType, id);
    }

    if (!comp) {
        // Fallback to Rectangle or Label
        if (!irWidget.label.isEmpty()) {
            comp = new LabelComponent(id);
        } else {
            comp = new RectangleComponent(id);
        }
    }

    // Restore full state if saved in properties or customProperties
    QJsonObject props = !irWidget.properties.isEmpty() ? irWidget.properties : irWidget.customProperties;
    if (!props.isEmpty()) {
        comp->fromJson(props);
    }

    if (auto* img = dynamic_cast<ImageComponent*>(comp)) {
        if (props.contains("imagePath")) {
            img->setImagePath(props["imagePath"].toString());
        }
        if (props.contains("format")) {
            img->setFormat(props["format"].toString());
        }
        if (props.contains("opacityPercent")) {
            img->setOpacityPercent(props["opacityPercent"].toInt());
        }
        if (props.contains("scalingMode")) {
            img->setScalingMode(props["scalingMode"].toString());
        }
    }

    // Ensure layout positions are respected if specified
    if (irWidget.layout.hasLeft || irWidget.layout.hasTop) {
        qreal x = irWidget.layout.hasLeft ? irWidget.layout.left : comp->compX();
        qreal y = irWidget.layout.hasTop ? irWidget.layout.top : comp->compY();
        comp->setCompPos(x, y);
    }

    // Sizing from style or properties if provided
    if (props.contains("width") && props.contains("height")) {
        comp->setCompSize(props["width"].toDouble(), props["height"].toDouble());
    }

    // Apply label
    if (!irWidget.label.isEmpty()) {
        if (auto* btn = dynamic_cast<ButtonComponent*>(comp)) {
            btn->setText(irWidget.label);
        } else if (auto* lbl = dynamic_cast<LabelComponent*>(comp)) {
            lbl->setText(irWidget.label);
        } else if (auto* txt = dynamic_cast<TextInputComponent*>(comp)) {
            txt->setPlaceholder(irWidget.label);
        } else if (auto* chk = dynamic_cast<CheckboxComponent*>(comp)) {
            chk->setText(irWidget.label);
        }
    }

    // Apply colors if explicitly set in effects
    if (irWidget.effects.backgroundColor.isValid()) {
        comp->applyColorStyle("background", irWidget.effects.backgroundColor.solidColor);
    }

    // Bindings
    QList<DataBinding> compBindings;
    for (const IRBinding& b : irWidget.bindings) {
        compBindings.append(convertDataBinding(b));
    }
    if (!compBindings.isEmpty()) {
        comp->setBindings(compBindings);
    }

    return comp;
}

DataSource IRToProjectAdapter::convertDataSource(const IRDataSource& irDs) {
    DataSource ds(irDs.id.toString(), irDs.name, DataSourceType::Variable);
    ds.setHardwareRef(irDs.hardwareRef);
    if (irDs.updateInterval > 0) {
        QJsonObject meta = ds.metadata();
        meta["pollingIntervalMs"] = irDs.updateInterval;
        ds.setMetadata(meta);
    }
    if (irDs.initialValue.isValid()) {
        ds.setValue(irDs.initialValue);
    }
    return ds;
}

DataBinding IRToProjectAdapter::convertDataBinding(const IRBinding& irB) {
    BindingDirection dir = BindingDirection::Read;
    switch (irB.direction) {
        case IRBindingDirection::Read:      dir = BindingDirection::Read; break;
        case IRBindingDirection::Write:     dir = BindingDirection::Write; break;
        case IRBindingDirection::ReadWrite: dir = BindingDirection::ReadWrite; break;
    }

    DataBinding b(irB.componentId.toString(), irB.propertyPath, irB.sourceId, dir);
    if (!irB.transformExpression.isEmpty()) {
        b.setTransformExpression(irB.transformExpression);
    }
    return b;
}

} // namespace IR
