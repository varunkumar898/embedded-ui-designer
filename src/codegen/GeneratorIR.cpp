#include "GeneratorIR.h"
#include "ValueVisualizationComponent.h"
#include "ProgressBarComponent.h"
#include "CircularProgressComponent.h"
#include "GaugeComponent.h"
#include "SpeedometerComponent.h"
#include "BatteryComponent.h"
#include "PressureComponent.h"
#include "RpmComponent.h"
#include "TemperatureComponent.h"
#include "TabViewComponent.h"
#include "NavigationBarComponent.h"
#include "ListComponent.h"
#include "TableComponent.h"

namespace CodeGen {

IRProject GeneratorIR::buildFromProject(const Project* project) {
    IRProject ir;
    if (!project) return ir;

    ir.name = project->projectName();
    ir.framework = project->targetFramework();
    ir.targetFamily = project->hardwareConfig().family.isEmpty() ? "STM32" : project->hardwareConfig().family;
    ir.targetBoard = project->hardwareConfig().boardId;
    ir.displayWidth = project->displayConfig().width;
    ir.displayHeight = project->displayConfig().height;
    ir.isRound = project->displayConfig().round;
    ir.colorDepth = project->displayConfig().colorDepth;

    // 1. Data Sources
    for (const auto& ds : project->dataSources()) {
        IRDataSource irds;
        irds.id = ds.id();
        irds.name = ds.name();
        irds.type = ds.type();
        irds.direction = ds.direction();
        irds.dataType = ds.dataType();
        irds.hardwareRef = ds.hardwareRef();
        irds.initialValue = ds.value();
        ir.dataSources.append(irds);
    }

    // 2. Data Bindings
    for (const auto& db : project->dataBindings()) {
        IRBinding irdb;
        irdb.componentId = db.componentId();
        irdb.propertyName = db.propertyName();
        irdb.sourceId = db.sourceId();
        irdb.direction = db.direction();
        irdb.transformExpression = db.transformExpression();
        ir.bindings.append(irdb);
    }

    // 3. Screens & Widgets
    for (Screen* scr : project->screens()) {
        if (!scr) continue;
        IRScreen irScr;
        irScr.id = scr->id();
        irScr.name = scr->name();
        irScr.width = scr->width() > 0 ? scr->width() : ir.displayWidth;
        irScr.height = scr->height() > 0 ? scr->height() : ir.displayHeight;
        irScr.backgroundColor = scr->backgroundColor();

        for (UIComponent* comp : scr->components()) {
            if (!comp) continue;
            IRWidget w;
            w.id = comp->componentId();
            w.type = comp->componentType();
            w.x = comp->compX();
            w.y = comp->compY();
            w.width = comp->compWidth();
            w.height = comp->compHeight();
            w.zOrder = static_cast<int>(comp->zValue());
            w.parentScreenId = scr->id();
            w.properties = comp->toJson();
            w.stateStyles = comp->stateStyles();
            w.bindings = comp->bindings();
            w.interactions = comp->interactions();

            irScr.widgets.append(w);
        }

        ir.screens.append(irScr);
    }

    return ir;
}

} // namespace CodeGen
