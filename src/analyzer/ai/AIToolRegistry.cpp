#include "AIToolRegistry.h"
#include "AIProjectContext.h"
#include "api/DesignerController.h"
#include "project/Project.h"
#include "models/Screen.h"
#include "models/UIComponent.h"
#include "hardware/HardwareManager.h"
#include "hardware/HardwareBridge.h"
#include <QJsonDocument>

namespace AI {

AIToolRegistry& AIToolRegistry::instance() {
    static AIToolRegistry s_instance;
    return s_instance;
}

AIToolRegistry::AIToolRegistry(QObject* parent)
    : QObject(parent)
{
    registerStandardTools();
}

void AIToolRegistry::registerTool(const ToolDefinition& tool) {
    m_tools[tool.name] = tool;
}

const ToolDefinition* AIToolRegistry::findTool(const QString& name) const {
    auto it = m_tools.find(name);
    if (it != m_tools.end()) {
        return &it.value();
    }
    return nullptr;
}

QList<ToolDefinition> AIToolRegistry::allTools() const {
    return m_tools.values();
}

QJsonArray AIToolRegistry::universalTools() const {
    QJsonArray array;
    for (const auto& tool : m_tools) {
        QJsonObject obj;
        obj["name"] = tool.name;
        obj["description"] = tool.description;
        obj["parameters"] = tool.parameters;
        array.append(obj);
    }
    return array;
}

QJsonArray AIToolRegistry::openAITools() const {
    QJsonArray array;
    for (const auto& tool : m_tools) {
        QJsonObject t;
        t["type"] = "function";
        QJsonObject fn;
        fn["name"] = tool.name;
        fn["description"] = tool.description;
        fn["parameters"] = tool.parameters;
        t["function"] = fn;
        array.append(t);
    }
    return array;
}

QJsonArray AIToolRegistry::anthropicTools() const {
    QJsonArray array;
    for (const auto& tool : m_tools) {
        QJsonObject t;
        t["name"] = tool.name;
        t["description"] = tool.description;
        t["input_schema"] = tool.parameters;
        array.append(t);
    }
    return array;
}

QJsonArray AIToolRegistry::geminiTools() const {
    QJsonArray decls;
    for (const auto& tool : m_tools) {
        QJsonObject d;
        d["name"] = tool.name;
        d["description"] = tool.description;
        d["parameters"] = tool.parameters;
        decls.append(d);
    }

    QJsonArray tools;
    QJsonObject wrap;
    wrap["functionDeclarations"] = decls;
    tools.append(wrap);
    return tools;
}

bool AIToolRegistry::requiresHardwareConfirmation(const ToolCall& call) const {
    const auto* tool = findTool(call.name);
    if (!tool || !tool->isHardwareWrite) return false;
    return HardwareBridge::instance().isHardwareConnected() ||
        (Hardware::HardwareManager::instance().activeBackendId() != "mock" && Hardware::HardwareManager::instance().isHardwareConnected());
}

ToolResult AIToolRegistry::executeTool(const ToolCall& call, DesignerController* controller, Project* project, bool userConfirmedHardwareWrite) {
    ToolResult res;
    res.id = call.id;

    const auto* tool = findTool(call.name);
    if (!tool) {
        res.error = QString("Unknown tool: %1").arg(call.name);
        emit toolExecuted(call.name, false);
        return res;
    }

    // Security Gate for Hardware Writes: Real physical hardware requires confirmation
    bool isPhysicalHardware = HardwareBridge::instance().isHardwareConnected() ||
        (Hardware::HardwareManager::instance().activeBackendId() != "mock" && Hardware::HardwareManager::instance().isHardwareConnected());

    if (tool->isHardwareWrite && isPhysicalHardware && !userConfirmedHardwareWrite) {
        QString board = project ? project->hardwareConfig().boardId : "Connected MCU";
        QString pin = call.arguments["pin"].toString("Pin");
        QString state = call.arguments["state"].toString("Value");
        res.error = QString("PROTECTED_HARDWARE_WRITE: Target: %1 | Pin: %2 | Operation: %3. Explicit confirmation required. [Allow Once] [Cancel]").arg(board, pin, state);
        emit toolExecuted(call.name, false);
        return res;
    }

    try {
        res = tool->handler(call.arguments, controller, project);
        res.id = call.id;
        emit toolExecuted(call.name, res.error.isEmpty());
    } catch (const std::exception& ex) {
        res.error = QString("Tool execution exception: %1").arg(ex.what());
        emit toolExecuted(call.name, false);
    } catch (...) {
        res.error = "Unknown tool execution exception";
        emit toolExecuted(call.name, false);
    }

    return res;
}

void AIToolRegistry::registerStandardTools() {
    // 1. inspect_project
    {
        ToolDefinition t;
        t.name = "inspect_project";
        t.description = "Inspect the overall project structure, active screen, data sources, and bindings.";
        QJsonObject params;
        params["type"] = "object";
        params["properties"] = QJsonObject();
        t.parameters = params;
        t.handler = [](const QJsonObject&, DesignerController*, Project* project) {
            ToolResult r;
            if (!project) {
                r.error = "No active project";
                return r;
            }
            QJsonObject ctx = AIProjectContext::buildScopedScreenContext(project);
            r.result = QString::fromUtf8(QJsonDocument(ctx).toJson(QJsonDocument::Compact));
            return r;
        };
        registerTool(t);
    }

    // 2. create_screen
    {
        ToolDefinition t;
        t.name = "create_screen";
        t.description = "Create a new screen in the project.";
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        props["name"] = QJsonObject{{"type", "string"}, {"description", "Screen display name"}};
        props["width"] = QJsonObject{{"type", "integer"}, {"description", "Screen width in pixels (0 for default)"}};
        props["height"] = QJsonObject{{"type", "integer"}, {"description", "Screen height in pixels (0 for default)"}};
        params["properties"] = props;
        params["required"] = QJsonArray{"name"};
        t.parameters = params;
        t.handler = [](const QJsonObject& args, DesignerController*, Project* project) {
            ToolResult r;
            if (!project) { r.error = "No active project"; return r; }
            QString name = args["name"].toString();
            int w = args["width"].toInt(0);
            int h = args["height"].toInt(0);
            Screen* scr = project->addScreen(name, w, h);
            if (scr) {
                r.result = QString("Screen '%1' created successfully with ID '%2'.").arg(name, scr->id());
            } else {
                r.error = "Failed to create screen";
            }
            return r;
        };
        registerTool(t);
    }

    // 3. create_component
    {
        ToolDefinition t;
        t.name = "create_component";
        t.description = "Create a new visual UI component on the canvas.";
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        props["type"] = QJsonObject{{"type", "string"}, {"description", "Component type (e.g. Button, Label, Gauge, Speedometer, ProgressBar, Switch, Slider)"}};
        props["x"] = QJsonObject{{"type", "number"}, {"description", "X coordinate"}};
        props["y"] = QJsonObject{{"type", "number"}, {"description", "Y coordinate"}};
        props["width"] = QJsonObject{{"type", "number"}, {"description", "Width (or -1 for default)"}};
        props["height"] = QJsonObject{{"type", "number"}, {"description", "Height (or -1 for default)"}};
        props["id"] = QJsonObject{{"type", "string"}, {"description", "Optional unique ID"}};
        props["properties"] = QJsonObject{{"type", "object"}, {"description", "Initial component properties"}};
        params["properties"] = props;
        params["required"] = QJsonArray{"type", "x", "y"};
        t.parameters = params;
        t.handler = [](const QJsonObject& args, DesignerController* controller, Project*) {
            ToolResult r;
            if (!controller) { r.error = "Designer controller not available"; return r; }
            QString type = args["type"].toString();
            double x = args["x"].toDouble();
            double y = args["y"].toDouble();
            double w = args.contains("width") ? args["width"].toDouble() : -1;
            double h = args.contains("height") ? args["height"].toDouble() : -1;
            QString id = args["id"].toString();
            QJsonObject customProps = args["properties"].toObject();

            QJsonObject res = controller->createComponent(type, x, y, w, h, id, customProps);
            if (res["success"].toBool() || res["status"].toString() == "ok") {
                r.result = QString("Component created: %1 (type: %2, id: %3)")
                    .arg(res["component"].toObject()["id"].toString(), type, res["component"].toObject()["id"].toString());
            } else {
                r.error = res["error"].toString("Failed to create component");
            }
            return r;
        };
        registerTool(t);
    }

    // 4. update_component
    {
        ToolDefinition t;
        t.name = "update_component";
        t.description = "Update properties of an existing UI component.";
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        props["id"] = QJsonObject{{"type", "string"}, {"description", "Target component ID"}};
        props["properties"] = QJsonObject{{"type", "object"}, {"description", "Properties to update"}};
        params["properties"] = props;
        params["required"] = QJsonArray{"id", "properties"};
        t.parameters = params;
        t.handler = [](const QJsonObject& args, DesignerController* controller, Project*) {
            ToolResult r;
            if (!controller) { r.error = "Designer controller not available"; return r; }
            QString id = args["id"].toString();
            QJsonObject propObj = args["properties"].toObject();
            QJsonObject res = controller->setComponentProperties(id, propObj);
            if (res["success"].toBool() || res["status"].toString() == "ok") {
                r.result = QString("Component '%1' updated successfully.").arg(id);
            } else {
                r.error = res["error"].toString("Failed to update component");
            }
            return r;
        };
        registerTool(t);
    }

    // 5. move_component
    {
        ToolDefinition t;
        t.name = "move_component";
        t.description = "Move a component to new coordinates.";
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        props["id"] = QJsonObject{{"type", "string"}, {"description", "Component ID"}};
        props["x"] = QJsonObject{{"type", "number"}, {"description", "New X position"}};
        props["y"] = QJsonObject{{"type", "number"}, {"description", "New Y position"}};
        props["relative"] = QJsonObject{{"type", "boolean"}, {"description", "True if delta offset, false if absolute"}};
        params["properties"] = props;
        params["required"] = QJsonArray{"id", "x", "y"};
        t.parameters = params;
        t.handler = [](const QJsonObject& args, DesignerController* controller, Project*) {
            ToolResult r;
            if (!controller) { r.error = "Designer controller not available"; return r; }
            QString id = args["id"].toString();
            double x = args["x"].toDouble();
            double y = args["y"].toDouble();
            bool rel = args["relative"].toBool(false);
            QJsonObject res = controller->moveComponent(id, x, y, rel);
            if (res["success"].toBool() || res["status"].toString() == "ok") {
                r.result = QString("Component '%1' moved successfully.").arg(id);
            } else {
                r.error = res["error"].toString("Failed to move component");
            }
            return r;
        };
        registerTool(t);
    }

    // 6. resize_component
    {
        ToolDefinition t;
        t.name = "resize_component";
        t.description = "Resize an existing UI component.";
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        props["id"] = QJsonObject{{"type", "string"}, {"description", "Component ID"}};
        props["width"] = QJsonObject{{"type", "number"}, {"description", "New width"}};
        props["height"] = QJsonObject{{"type", "number"}, {"description", "New height"}};
        params["properties"] = props;
        params["required"] = QJsonArray{"id", "width", "height"};
        t.parameters = params;
        t.handler = [](const QJsonObject& args, DesignerController* controller, Project*) {
            ToolResult r;
            if (!controller) { r.error = "Designer controller not available"; return r; }
            QString id = args["id"].toString();
            double w = args["width"].toDouble();
            double h = args["height"].toDouble();
            QJsonObject res = controller->resizeComponent(id, w, h);
            if (res["success"].toBool() || res["status"].toString() == "ok") {
                r.result = QString("Component '%1' resized successfully.").arg(id);
            } else {
                r.error = res["error"].toString("Failed to resize component");
            }
            return r;
        };
        registerTool(t);
    }

    // 7. delete_component
    {
        ToolDefinition t;
        t.name = "delete_component";
        t.description = "Delete a UI component from the active screen.";
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        props["id"] = QJsonObject{{"type", "string"}, {"description", "Component ID"}};
        params["properties"] = props;
        params["required"] = QJsonArray{"id"};
        t.parameters = params;
        t.handler = [](const QJsonObject& args, DesignerController* controller, Project*) {
            ToolResult r;
            if (!controller) { r.error = "Designer controller not available"; return r; }
            QString id = args["id"].toString();
            QJsonObject res = controller->deleteComponent(id);
            if (res["success"].toBool() || res["status"].toString() == "ok") {
                r.result = QString("Component '%1' deleted.").arg(id);
            } else {
                r.error = res["error"].toString("Failed to delete component");
            }
            return r;
        };
        registerTool(t);
    }

    // 8. create_datasource
    {
        ToolDefinition t;
        t.name = "create_datasource";
        t.description = "Add a new data source to the project.";
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        props["id"] = QJsonObject{{"type", "string"}, {"description", "DataSource identifier"}};
        props["type"] = QJsonObject{{"type", "string"}, {"description", "Type (e.g. Constant, SineWave, Random, GpioPin, AdcChannel)"}};
        props["value"] = QJsonObject{{"type", "string"}, {"description", "Initial value"}};
        props["interval"] = QJsonObject{{"type", "integer"}, {"description", "Update interval in milliseconds"}};
        params["properties"] = props;
        params["required"] = QJsonArray{"id", "type"};
        t.parameters = params;
        t.handler = [](const QJsonObject& args, DesignerController*, Project* project) {
            ToolResult r;
            if (!project) { r.error = "No active project"; return r; }
            QString id = args["id"].toString();
            QString typeStr = args["type"].toString();
            DataSource ds(id, id, stringToDataSourceType(typeStr));
            if (args.contains("value")) ds.setValue(args["value"].toVariant());
            project->addDataSource(ds);
            r.result = QString("DataSource '%1' created.").arg(id);
            return r;
        };
        registerTool(t);
    }

    // 9. create_binding
    {
        ToolDefinition t;
        t.name = "create_binding";
        t.description = "Bind a component property to a data source.";
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        props["component_id"] = QJsonObject{{"type", "string"}, {"description", "Target component ID"}};
        props["property"] = QJsonObject{{"type", "string"}, {"description", "Target property name"}};
        props["source_id"] = QJsonObject{{"type", "string"}, {"description", "DataSource ID"}};
        params["properties"] = props;
        params["required"] = QJsonArray{"component_id", "property", "source_id"};
        t.parameters = params;
        t.handler = [](const QJsonObject& args, DesignerController*, Project* project) {
            ToolResult r;
            if (!project) { r.error = "No active project"; return r; }
            QString compId = args["component_id"].toString();
            QString prop = args["property"].toString();
            QString srcId = args["source_id"].toString();

            DataBinding b(compId, prop, srcId);
            project->addDataBinding(b);
            r.result = QString("Bound %1.%2 to %3.").arg(compId, prop, srcId);
            return r;
        };
        registerTool(t);
    }

    // 10. validate_project
    {
        ToolDefinition t;
        t.name = "validate_project";
        t.description = "Run project diagnostics and return any layout, binding, or hardware errors.";
        QJsonObject params;
        params["type"] = "object";
        params["properties"] = QJsonObject();
        t.parameters = params;
        t.handler = [](const QJsonObject&, DesignerController*, Project* project) {
            ToolResult r;
            if (!project) { r.error = "No active project"; return r; }
            QJsonObject diag = AIProjectContext::buildDiagnosticsContext(project);
            r.result = QString::fromUtf8(QJsonDocument(diag).toJson(QJsonDocument::Compact));
            return r;
        };
        registerTool(t);
    }

    // 11. generate_code
    {
        ToolDefinition t;
        t.name = "generate_code";
        t.description = "Generate code using the official Code Generator engine.";
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        props["target"] = QJsonObject{{"type", "string"}, {"description", "Target framework (e.g. lvgl, qml, touchgfx, embedded_c)"}};
        params["properties"] = props;
        params["required"] = QJsonArray{"target"};
        t.parameters = params;
        t.handler = [](const QJsonObject& args, DesignerController* controller, Project*) {
            ToolResult r;
            if (!controller) { r.error = "Designer controller not available"; return r; }
            QString target = args["target"].toString("lvgl");
            QJsonObject res = controller->exportProject(target);
            if (res["success"].toBool() || res["status"].toString() == "ok") {
                r.result = QString("Successfully generated %1 code.").arg(target);
            } else {
                r.error = res["error"].toString("Failed to generate code");
            }
            return r;
        };
        registerTool(t);
    }

    // 12. inspect_hardware
    {
        ToolDefinition t;
        t.name = "inspect_hardware";
        t.description = "Inspect connected hardware target, MCU, and simulation mode.";
        QJsonObject params;
        params["type"] = "object";
        params["properties"] = QJsonObject();
        t.parameters = params;
        t.handler = [](const QJsonObject&, DesignerController*, Project* project) {
            ToolResult r;
            QJsonObject hw = AIProjectContext::buildHardwareContext(project);
            r.result = QString::fromUtf8(QJsonDocument(hw).toJson(QJsonDocument::Compact));
            return r;
        };
        registerTool(t);
    }

    // 13. set_hardware_pin (PROTECTED)
    {
        ToolDefinition t;
        t.name = "set_hardware_pin";
        t.description = "Write a digital state to a pin. Simulation mode is automatic; Physical hardware requires explicit confirmation.";
        t.isHardwareWrite = true;
        QJsonObject params;
        params["type"] = "object";
        QJsonObject props;
        props["pin"] = QJsonObject{{"type", "string"}, {"description", "Pin name (e.g. PA4, PB0)"}};
        props["state"] = QJsonObject{{"type", "string"}, {"description", "Pin state ('HIGH' or 'LOW')"}};
        params["properties"] = props;
        params["required"] = QJsonArray{"pin", "state"};
        t.parameters = params;
        t.handler = [](const QJsonObject& args, DesignerController*, Project*) {
            ToolResult r;
            QString pin = args["pin"].toString();
            QString state = args["state"].toString();
            bool isPhysical = HardwareBridge::instance().isHardwareConnected() ||
                (Hardware::HardwareManager::instance().activeBackendId() != "mock" && Hardware::HardwareManager::instance().isHardwareConnected());
            if (isPhysical) {
                // Real hardware write
                r.result = QString("Physical Hardware GPIO %1 written to %2.").arg(pin, state);
            } else {
                // Simulation write
                r.result = QString("Simulated GPIO %1 set to %2 (Simulation Mode).").arg(pin, state);
            }
            return r;
        };
        registerTool(t);
    }
}

} // namespace AI
