#include "DesignerController.h"
#include "MainWindow.h"
#include "CanvasScene.h"
#include "CanvasView.h"
#include "Project.h"
#include "PropertiesPanel.h"
#include "LayerPanel.h"
#include "ComponentSchema.h"
#include "UIComponent.h"
#include "AddComponentCommand.h"
#include "DeleteComponentCommand.h"
#include "MoveComponentCommand.h"
#include "ResizeComponentCommand.h"
#include "PropertyChangeCommand.h"
#include "LvglGenerator.h"
#include "QtMcuGenerator.h"
#include "UgfxGenerator.h"
#include "DeviceDatabase.h"
#include "PinMuxEngine.h"
#include "HardwareModel.h"

#include <QUndoStack>
#include <QDir>
#include <QTemporaryDir>
#include <QFile>

DesignerController::DesignerController(MainWindow* window, QObject* parent)
    : QObject(parent)
    , m_window(window)
{
    if (m_window) {
        m_scene = m_window->canvasScene();
        m_view = m_window->canvasView();
        m_project = m_window->currentProject();
        m_undoStack = m_window->undoStack();
        m_propertiesPanel = m_window->propertiesPanel();
        m_layerPanel = m_window->layerPanel();
    }
}

UIComponent* DesignerController::findComponentById(const QString& id) const {
    if (!m_scene) return nullptr;
    for (UIComponent* c : m_scene->uiComponents()) {
        if (c && c->componentId().compare(id, Qt::CaseInsensitive) == 0) {
            return c;
        }
    }
    return nullptr;
}

QString DesignerController::generateUniqueId(const QString& typePrefix) const {
    QString base = typePrefix.trimmed().toLower().replace("-", "_").replace(" ", "_");
    if (base.isEmpty()) base = "component";

    int index = 1;
    while (true) {
        QString candidate = QString("%1_%2").arg(base).arg(index);
        if (!findComponentById(candidate)) {
            return candidate;
        }
        ++index;
    }
}

QJsonObject DesignerController::serializeComponentFull(const UIComponent* comp) const {
    QJsonObject obj;
    if (!comp) return obj;

    obj["id"] = comp->componentId();
    obj["type"] = ComponentSchemaRegistry::instance().normalizeTypeName(comp->componentType());
    obj["typeName"] = comp->componentType();
    obj["x"] = comp->compX();
    obj["y"] = comp->compY();
    obj["width"] = comp->compWidth();
    obj["height"] = comp->compHeight();
    obj["zValue"] = comp->zValue();
    obj["visible"] = comp->isComponentVisible();
    obj["selected"] = comp->isSelected();
    obj["properties"] = ComponentSchemaRegistry::instance().getComponentProperties(comp);

    return obj;
}

void DesignerController::notifyUiChanges(UIComponent* targetComp) {
    if (m_scene) {
        m_scene->update();
    }
    if (m_layerPanel) {
        m_layerPanel->refreshLayers();
    }
    if (m_propertiesPanel) {
        if (targetComp && targetComp->isSelected()) {
            m_propertiesPanel->refreshValues();
        } else if (!targetComp && m_propertiesPanel->targetComponent()) {
            m_propertiesPanel->refreshValues();
        }
    }
    if (m_project) {
        m_project->setDirty(true);
    }
}

// -----------------------------------------------------------------------------
// Project Tools
// -----------------------------------------------------------------------------

QJsonObject DesignerController::getProject() const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }

    res["success"] = true;
    res["name"] = m_project->projectName();
    res["filePath"] = m_project->projectFilePath();
    res["targetFramework"] = m_project->targetFramework();
    res["width"] = m_project->displayConfig().width;
    res["height"] = m_project->displayConfig().height;
    res["isDirty"] = m_project->isDirty();

    QJsonArray components;
    if (m_scene) {
        for (const UIComponent* c : m_scene->uiComponents()) {
            components.append(serializeComponentFull(c));
        }
    }
    res["components"] = components;

    QJsonArray stylesArr;
    for (const auto& s : m_project->colorStyles()) {
        QJsonObject so;
        so["name"] = s.name;
        so["color"] = s.color.name();
        stylesArr.append(so);
    }
    res["colorStyles"] = stylesArr;

    return res;
}

QJsonObject DesignerController::getProjectMetadata() const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }

    res["success"] = true;
    res["name"] = m_project->projectName();
    res["targetFramework"] = m_project->targetFramework();
    res["width"] = m_project->displayConfig().width;
    res["height"] = m_project->displayConfig().height;
    res["isDirty"] = m_project->isDirty();
    res["filePath"] = m_project->projectFilePath();
    res["componentCount"] = m_scene ? m_scene->uiComponents().size() : 0;
    return res;
}

QJsonObject DesignerController::createProject(const QString& name, int width, int height) {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No project subsystem available";
        return res;
    }

    int w = width > 0 ? width : 480;
    int h = height > 0 ? height : 272;
    QString n = name.isEmpty() ? "NewEmbeddedApp" : name;

    m_project->newProject(n, w, h);
    if (m_undoStack) {
        m_undoStack->clear();
    }
    notifyUiChanges();

    res["success"] = true;
    res["name"] = n;
    res["width"] = w;
    res["height"] = h;
    return res;
}

QJsonObject DesignerController::saveProject(const QString& filePath) {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }

    QString path = filePath.isEmpty() ? m_project->projectFilePath() : filePath;
    if (path.isEmpty()) {
        res["success"] = false;
        res["error"] = "File path must be specified for new project";
        return res;
    }

    bool ok = m_project->saveToFile(path);
    res["success"] = ok;
    if (ok) {
        res["filePath"] = path;
    } else {
        res["error"] = "Failed to save project to file";
    }
    return res;
}

QJsonObject DesignerController::loadProject(const QString& filePath) {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }

    if (filePath.isEmpty() || !QFile::exists(filePath)) {
        res["success"] = false;
        res["error"] = "Specified project file does not exist: " + filePath;
        return res;
    }

    bool ok = m_project->loadFromFile(filePath);
    res["success"] = ok;
    if (ok) {
        if (m_undoStack) m_undoStack->clear();
        notifyUiChanges();
        res["name"] = m_project->projectName();
        res["width"] = m_project->displayConfig().width;
        res["height"] = m_project->displayConfig().height;
        res["componentCount"] = m_scene ? m_scene->uiComponents().size() : 0;
    } else {
        res["error"] = "Failed to parse project file";
    }
    return res;
}

// -----------------------------------------------------------------------------
// Component Discovery Tools
// -----------------------------------------------------------------------------

QJsonObject DesignerController::listComponentTypes() const {
    QJsonObject res;
    QJsonArray typesArr;
    for (const auto& s : ComponentSchemaRegistry::instance().allSchemas()) {
        QJsonObject to;
        to["type"] = s.type;
        to["displayName"] = s.displayName;
        to["category"] = s.category;
        typesArr.append(to);
    }
    res["success"] = true;
    res["types"] = typesArr;
    return res;
}

QJsonObject DesignerController::getComponentSchema(const QString& type) const {
    QJsonObject res;
    if (type.isEmpty()) {
        res["success"] = true;
        res["schema"] = ComponentSchemaRegistry::instance().fullSchemaJson();
        return res;
    }

    const auto* s = ComponentSchemaRegistry::instance().findSchema(type);
    if (!s) {
        res["success"] = false;
        res["error"] = "Unknown component type: " + type;
        return res;
    }

    res["success"] = true;
    res["schema"] = s->toJson();
    return res;
}

QJsonObject DesignerController::getComponent(const QString& id) const {
    QJsonObject res;
    UIComponent* comp = findComponentById(id);
    if (!comp) {
        res["success"] = false;
        res["error"] = "Component not found: " + id;
        return res;
    }

    res["success"] = true;
    res["component"] = serializeComponentFull(comp);
    return res;
}

QJsonObject DesignerController::getComponentTree() const {
    QJsonObject res;
    QJsonArray items;
    if (m_scene) {
        for (const UIComponent* c : m_scene->uiComponents()) {
            QJsonObject item;
            item["id"] = c->componentId();
            item["type"] = ComponentSchemaRegistry::instance().normalizeTypeName(c->componentType());
            item["x"] = c->compX();
            item["y"] = c->compY();
            item["width"] = c->compWidth();
            item["height"] = c->compHeight();
            item["zValue"] = c->zValue();
            item["visible"] = c->isComponentVisible();
            items.append(item);
        }
    }
    res["success"] = true;
    res["components"] = items;
    return res;
}

// -----------------------------------------------------------------------------
// Component Creation & Deletion
// -----------------------------------------------------------------------------

QJsonObject DesignerController::createComponent(const QString& type, double x, double y, double width, double height, const QString& requestedId, const QJsonObject& properties) {
    QJsonObject res;
    const auto* schema = ComponentSchemaRegistry::instance().findSchema(type);
    if (!schema) {
        res["success"] = false;
        res["error"] = "Unknown component type: " + type;
        return res;
    }

    QString id = requestedId.trimmed();
    if (id.isEmpty()) {
        id = generateUniqueId(schema->type);
    } else if (findComponentById(id)) {
        res["success"] = false;
        res["error"] = "Component ID already exists: " + id;
        return res;
    }

    UIComponent* comp = ComponentSchemaRegistry::instance().createComponent(schema->type, id);
    if (!comp) {
        res["success"] = false;
        res["error"] = "Failed to instantiate component of type: " + type;
        return res;
    }

    // Set position
    comp->setCompPos(x >= 0 ? x : 40.0, y >= 0 ? y : 40.0);

    // Set size if requested, otherwise use defaults from schema
    double w = width > 0 ? width : schema->defaultProperties["width"].toDouble(100.0);
    double h = height > 0 ? height : schema->defaultProperties["height"].toDouble(40.0);
    comp->setCompSize(w, h);

    // Apply default schema properties
    QString err;
    ComponentSchemaRegistry::instance().setComponentProperties(comp, schema->defaultProperties, &err);

    // Apply user requested property overrides
    if (!properties.isEmpty()) {
        if (!ComponentSchemaRegistry::instance().setComponentProperties(comp, properties, &err)) {
            delete comp;
            res["success"] = false;
            res["error"] = err;
            return res;
        }
    }

    // Push undoable AddComponentCommand
    if (m_undoStack && m_scene) {
        m_undoStack->push(new AddComponentCommand(m_scene, comp));
    } else if (m_scene) {
        m_scene->addUIComponent(comp);
    }

    // Select the newly created component
    if (m_scene) {
        m_scene->clearSelection();
        comp->setSelected(true);
    }
    if (m_propertiesPanel) {
        m_propertiesPanel->setTargetComponent(comp);
    }

    notifyUiChanges(comp);

    res["success"] = true;
    res["component"] = serializeComponentFull(comp);
    return res;
}

QJsonObject DesignerController::duplicateComponent(const QString& id, double dx, double dy) {
    QJsonObject res;
    UIComponent* src = findComponentById(id);
    if (!src) {
        res["success"] = false;
        res["error"] = "Component not found: " + id;
        return res;
    }

    QString newId = generateUniqueId(ComponentSchemaRegistry::instance().normalizeTypeName(src->componentType()));
    UIComponent* dup = ComponentSchemaRegistry::instance().createComponent(src->componentType(), newId);
    if (!dup) {
        res["success"] = false;
        res["error"] = "Failed to duplicate component";
        return res;
    }

    dup->fromJson(src->toJson());
    dup->setComponentId(newId);
    dup->setCompPos(src->compX() + dx, src->compY() + dy);

    if (m_undoStack && m_scene) {
        m_undoStack->push(new AddComponentCommand(m_scene, dup));
    } else if (m_scene) {
        m_scene->addUIComponent(dup);
    }

    if (m_scene) {
        m_scene->clearSelection();
        dup->setSelected(true);
    }
    if (m_propertiesPanel) {
        m_propertiesPanel->setTargetComponent(dup);
    }

    notifyUiChanges(dup);

    res["success"] = true;
    res["component"] = serializeComponentFull(dup);
    return res;
}

QJsonObject DesignerController::deleteComponent(const QString& id) {
    QJsonObject res;
    UIComponent* comp = findComponentById(id);
    if (!comp) {
        res["success"] = false;
        res["error"] = "Component not found: " + id;
        return res;
    }

    if (m_undoStack && m_scene) {
        m_undoStack->push(new DeleteComponentCommand(m_scene, {comp}));
    } else if (m_scene) {
        m_scene->removeUIComponent(comp);
        delete comp;
    }

    notifyUiChanges();

    res["success"] = true;
    res["id"] = id;
    return res;
}

// -----------------------------------------------------------------------------
// Component Editing Tools
// -----------------------------------------------------------------------------

QJsonObject DesignerController::setComponentProperty(const QString& id, const QString& propName, const QJsonValue& val) {
    QJsonObject res;
    UIComponent* comp = findComponentById(id);
    if (!comp) {
        res["success"] = false;
        res["error"] = "Component not found: " + id;
        return res;
    }

    QJsonObject oldState = comp->toJson();

    QString err;
    if (!ComponentSchemaRegistry::instance().setComponentProperty(comp, propName, val, &err)) {
        res["success"] = false;
        res["error"] = err;
        return res;
    }

    QJsonObject newState = comp->toJson();

    if (m_undoStack) {
        m_undoStack->push(new PropertyChangeCommand(comp, oldState, newState, QString("Set %1 on %2").arg(propName, id)));
    }

    notifyUiChanges(comp);

    res["success"] = true;
    res["component"] = serializeComponentFull(comp);
    return res;
}

QJsonObject DesignerController::setComponentProperties(const QString& id, const QJsonObject& properties) {
    QJsonObject res;
    UIComponent* comp = findComponentById(id);
    if (!comp) {
        res["success"] = false;
        res["error"] = "Component not found: " + id;
        return res;
    }

    QJsonObject oldState = comp->toJson();

    QString err;
    if (!ComponentSchemaRegistry::instance().setComponentProperties(comp, properties, &err)) {
        // Revert on validation failure
        comp->fromJson(oldState);
        res["success"] = false;
        res["error"] = err;
        return res;
    }

    QJsonObject newState = comp->toJson();

    if (m_undoStack) {
        m_undoStack->push(new PropertyChangeCommand(comp, oldState, newState, QString("Update properties on %1").arg(id)));
    }

    notifyUiChanges(comp);

    res["success"] = true;
    res["component"] = serializeComponentFull(comp);
    return res;
}

QJsonObject DesignerController::updateComponent(const QString& id, const QJsonObject& params) {
    QJsonObject res;
    UIComponent* comp = findComponentById(id);
    if (!comp) {
        res["success"] = false;
        res["error"] = "Component not found: " + id;
        return res;
    }

    QJsonObject oldState = comp->toJson();

    if (params.contains("x")) comp->setCompPos(params["x"].toDouble(), comp->compY());
    if (params.contains("y")) comp->setCompPos(comp->compX(), params["y"].toDouble());
    if (params.contains("width")) comp->setCompSize(params["width"].toDouble(), comp->compHeight());
    if (params.contains("height")) comp->setCompSize(comp->compWidth(), params["height"].toDouble());
    if (params.contains("visible")) comp->setComponentVisible(params["visible"].toBool());

    if (params.contains("properties") && params["properties"].isObject()) {
        QString err;
        if (!ComponentSchemaRegistry::instance().setComponentProperties(comp, params["properties"].toObject(), &err)) {
            comp->fromJson(oldState);
            res["success"] = false;
            res["error"] = err;
            return res;
        }
    }

    QJsonObject newState = comp->toJson();

    if (m_undoStack) {
        m_undoStack->push(new PropertyChangeCommand(comp, oldState, newState, QString("Update %1").arg(id)));
    }

    notifyUiChanges(comp);

    res["success"] = true;
    res["component"] = serializeComponentFull(comp);
    return res;
}

QJsonObject DesignerController::moveComponent(const QString& id, double x, double y, bool relative) {
    QJsonObject res;
    UIComponent* comp = findComponentById(id);
    if (!comp) {
        res["success"] = false;
        res["error"] = "Component not found: " + id;
        return res;
    }

    QPointF oldPos(comp->compX(), comp->compY());
    QPointF newPos = relative ? QPointF(comp->compX() + x, comp->compY() + y) : QPointF(x, y);

    if (m_undoStack) {
        m_undoStack->push(new MoveComponentCommand(comp, oldPos, newPos));
    } else {
        comp->setCompPos(newPos.x(), newPos.y());
    }

    notifyUiChanges(comp);

    res["success"] = true;
    res["x"] = comp->compX();
    res["y"] = comp->compY();
    res["component"] = serializeComponentFull(comp);
    return res;
}

QJsonObject DesignerController::resizeComponent(const QString& id, double width, double height) {
    QJsonObject res;
    UIComponent* comp = findComponentById(id);
    if (!comp) {
        res["success"] = false;
        res["error"] = "Component not found: " + id;
        return res;
    }

    if (width <= 0 || height <= 0) {
        res["success"] = false;
        res["error"] = "Width and height must be strictly positive";
        return res;
    }

    QRectF oldGeom(comp->compX(), comp->compY(), comp->compWidth(), comp->compHeight());
    QRectF newGeom(comp->compX(), comp->compY(), width, height);

    if (m_undoStack) {
        m_undoStack->push(new ResizeComponentCommand(comp, oldGeom, newGeom));
    } else {
        comp->setCompSize(width, height);
    }

    notifyUiChanges(comp);

    res["success"] = true;
    res["width"] = comp->compWidth();
    res["height"] = comp->compHeight();
    res["component"] = serializeComponentFull(comp);
    return res;
}

// -----------------------------------------------------------------------------
// Selection Tools
// -----------------------------------------------------------------------------

QJsonObject DesignerController::getSelectedComponents() const {
    QJsonObject res;
    QJsonArray selectedArr;
    if (m_scene) {
        for (const UIComponent* c : m_scene->uiComponents()) {
            if (c && c->isSelected()) {
                selectedArr.append(serializeComponentFull(c));
            }
        }
    }
    res["success"] = true;
    res["selected"] = selectedArr;
    return res;
}

QJsonObject DesignerController::selectComponent(const QString& id, bool addToSelection) {
    QJsonObject res;
    UIComponent* comp = findComponentById(id);
    if (!comp) {
        res["success"] = false;
        res["error"] = "Component not found: " + id;
        return res;
    }

    if (m_scene) {
        if (!addToSelection) {
            m_scene->clearSelection();
        }
        comp->setSelected(true);
    }

    if (m_propertiesPanel) {
        m_propertiesPanel->setTargetComponent(comp);
    }

    notifyUiChanges(comp);

    res["success"] = true;
    res["id"] = id;
    return res;
}

QJsonObject DesignerController::clearSelection() {
    QJsonObject res;
    if (m_scene) {
        m_scene->clearSelection();
    }
    if (m_propertiesPanel) {
        m_propertiesPanel->setTargetComponent(nullptr);
    }
    notifyUiChanges();
    res["success"] = true;
    return res;
}

// -----------------------------------------------------------------------------
// Canvas Tools
// -----------------------------------------------------------------------------

QJsonObject DesignerController::getCanvas() const {
    QJsonObject res;
    if (!m_scene || !m_project) {
        res["success"] = false;
        res["error"] = "Canvas scene not initialized";
        return res;
    }

    res["success"] = true;
    res["width"] = m_project->displayConfig().width;
    res["height"] = m_project->displayConfig().height;
    res["backgroundColor"] = m_scene->screenBackgroundColor().name();
    res["gridVisible"] = m_scene->isGridVisible();
    res["snapToGrid"] = m_scene->isSnapToGrid();
    res["gridSize"] = m_scene->gridSize();

    QJsonArray compArr;
    for (const UIComponent* c : m_scene->uiComponents()) {
        compArr.append(serializeComponentFull(c));
    }
    res["components"] = compArr;
    return res;
}

QJsonObject DesignerController::getCanvasSize() const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }
    res["success"] = true;
    res["width"] = m_project->displayConfig().width;
    res["height"] = m_project->displayConfig().height;
    return res;
}

QJsonObject DesignerController::clearCanvas() {
    QJsonObject res;
    if (!m_scene) {
        res["success"] = false;
        res["error"] = "No active scene";
        return res;
    }

    QList<UIComponent*> all = m_scene->uiComponents();
    if (all.isEmpty()) {
        res["success"] = true;
        res["count"] = 0;
        return res;
    }

    if (m_undoStack) {
        m_undoStack->push(new DeleteComponentCommand(m_scene, all));
    } else {
        m_scene->clearComponents();
    }

    notifyUiChanges();

    res["success"] = true;
    res["count"] = all.size();
    return res;
}

// -----------------------------------------------------------------------------
// History Tools
// -----------------------------------------------------------------------------

QJsonObject DesignerController::undo() {
    QJsonObject res;
    if (!m_undoStack) {
        res["success"] = false;
        res["error"] = "Undo stack not available";
        return res;
    }

    if (!m_undoStack->canUndo()) {
        res["success"] = false;
        res["error"] = "Nothing to undo";
        return res;
    }

    QString action = m_undoStack->undoText();
    m_undoStack->undo();
    notifyUiChanges();

    res["success"] = true;
    res["action"] = action;
    res["canUndo"] = m_undoStack->canUndo();
    res["canRedo"] = m_undoStack->canRedo();
    return res;
}

QJsonObject DesignerController::redo() {
    QJsonObject res;
    if (!m_undoStack) {
        res["success"] = false;
        res["error"] = "Undo stack not available";
        return res;
    }

    if (!m_undoStack->canRedo()) {
        res["success"] = false;
        res["error"] = "Nothing to redo";
        return res;
    }

    QString action = m_undoStack->redoText();
    m_undoStack->redo();
    notifyUiChanges();

    res["success"] = true;
    res["action"] = action;
    res["canUndo"] = m_undoStack->canUndo();
    res["canRedo"] = m_undoStack->canRedo();
    return res;
}

// -----------------------------------------------------------------------------
// Export Tools
// -----------------------------------------------------------------------------

QJsonObject DesignerController::exportProject(const QString& target, const QString& outputDir) {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }

    QString t = target.trimmed().toLower();
    if (t != "lvgl" && t != "qul" && t != "ugfx") {
        res["success"] = false;
        res["error"] = "Invalid export target '" + target + "'. Supported: 'lvgl', 'qul', 'ugfx'.";
        return res;
    }

    // Ensure project is saved to a temp or real path first
    QString projPath = m_project->projectFilePath();
    QTemporaryDir tempDir;
    if (projPath.isEmpty() || !QFile::exists(projPath)) {
        projPath = tempDir.filePath("export_temp.euiproj");
        m_project->saveToFile(projPath);
    }

    QString out = outputDir;
    if (out.isEmpty()) {
        out = QDir::tempPath() + "/embedded_ui_export_" + t;
    }
    QDir().mkpath(out);

    bool ok = false;
    QString err;
    if (t == "lvgl") {
        LvglGenerator gen(projPath);
        ok = gen.generate(out);
        err = gen.lastError();
    } else if (t == "qul") {
        QtMcuGenerator gen(projPath);
        ok = gen.generate(out);
        err = gen.lastError();
    } else if (t == "ugfx") {
        UgfxGenerator gen(projPath);
        ok = gen.generate(out);
        err = gen.lastError();
    }

    res["success"] = ok;
    if (ok) {
        res["target"] = t;
        res["outputDir"] = out;
    } else {
        res["error"] = err.isEmpty() ? "Export generator failed" : err;
    }
    return res;
}

QJsonObject DesignerController::exportComponent(const QString& id, const QString& target) {
    QJsonObject res;
    UIComponent* comp = findComponentById(id);
    if (!comp) {
        res["success"] = false;
        res["error"] = "Component not found: " + id;
        return res;
    }

    QString t = target.trimmed().toLower();
    QString snippet;
    if (t == "ugfx") {
        snippet = comp->toUgfxSnippet();
    } else {
        snippet = comp->toQmlSnippet();
    }

    res["success"] = true;
    res["id"] = id;
    res["target"] = t;
    res["snippet"] = snippet;
    return res;
}

// -----------------------------------------------------------------------------
// Unified Dispatch Helper
// -----------------------------------------------------------------------------

QJsonObject DesignerController::dispatch(const QString& method, const QJsonObject& params) {
    QString m = method.trimmed().toLower();

    // Project
    if (m == "get_project") return getProject();
    if (m == "get_project_metadata") return getProjectMetadata();
    if (m == "create_project") {
        return createProject(params["name"].toString(), params["width"].toInt(), params["height"].toInt());
    }
    if (m == "save_project") return saveProject(params["filePath"].toString());
    if (m == "load_project") return loadProject(params["filePath"].toString());

    // Discovery
    if (m == "list_component_types") return listComponentTypes();
    if (m == "get_component_schema") return getComponentSchema(params["type"].toString());
    if (m == "get_component") return getComponent(params["id"].toString());
    if (m == "get_component_tree") return getComponentTree();

    // Creation / Deletion
    if (m == "create_component") {
        return createComponent(
            params["type"].toString(),
            params.value("x").toDouble(-1),
            params.value("y").toDouble(-1),
            params.value("width").toDouble(-1),
            params.value("height").toDouble(-1),
            params["id"].toString(),
            params["properties"].toObject()
        );
    }
    if (m == "duplicate_component") {
        return duplicateComponent(
            params["id"].toString(),
            params.value("dx").toDouble(20),
            params.value("dy").toDouble(20)
        );
    }
    if (m == "delete_component") return deleteComponent(params["id"].toString());

    // Editing
    if (m == "update_component") return updateComponent(params["id"].toString(), params);
    if (m == "set_component_property") {
        return setComponentProperty(
            params["id"].toString(),
            params["property"].toString(),
            params["value"]
        );
    }
    if (m == "set_component_properties") {
        return setComponentProperties(
            params["id"].toString(),
            params["properties"].toObject()
        );
    }
    if (m == "move_component") {
        return moveComponent(
            params["id"].toString(),
            params.contains("dx") ? params["dx"].toDouble() : params["x"].toDouble(),
            params.contains("dy") ? params["dy"].toDouble() : params["y"].toDouble(),
            params.contains("dx") || params.contains("dy")
        );
    }
    if (m == "resize_component") {
        return resizeComponent(
            params["id"].toString(),
            params["width"].toDouble(),
            params["height"].toDouble()
        );
    }

    // Selection
    if (m == "get_selected_components") return getSelectedComponents();
    if (m == "select_component") {
        return selectComponent(params["id"].toString(), params["addToSelection"].toBool());
    }
    if (m == "clear_selection") return clearSelection();

    // Canvas
    if (m == "get_canvas") return getCanvas();
    if (m == "get_canvas_size") return getCanvasSize();
    if (m == "clear_canvas") return clearCanvas();

    // History
    if (m == "undo") return undo();
    if (m == "redo") return redo();

    // Export
    if (m == "export_project") {
        return exportProject(params["target"].toString(), params["outputDir"].toString());
    }
    if (m == "export_component") {
        return exportComponent(params["id"].toString(), params.value("target").toString("qml"));
    }

    // Hardware & Pinmux (Phases 22, 23, 24)
    if (m == "get_target") return getTarget();
    if (m == "set_target") {
        return setTarget(
            params.value("targetType").toString("device"),
            params.contains("id") ? params["id"].toString() : (params.contains("targetId") ? params["targetId"].toString() : params["deviceId"].toString())
        );
    }
    if (m == "list_devices") {
        return listDevices(
            params["vendor"].toString(),
            params["family"].toString(),
            params["search"].toString()
        );
    }
    if (m == "list_boards") {
        return listBoards(
            params["vendor"].toString(),
            params["mcu"].toString(),
            params["search"].toString()
        );
    }
    if (m == "get_device_info") {
        return getDeviceInfo(
            params.contains("deviceId") ? params["deviceId"].toString() : params["id"].toString()
        );
    }
    if (m == "get_board_info") {
        return getBoardInfo(
            params.contains("boardId") ? params["boardId"].toString() : params["id"].toString()
        );
    }
    if (m == "get_pinout") {
        return getPinout(
            params.contains("deviceId") ? params["deviceId"].toString() : params["id"].toString()
        );
    }
    if (m == "list_pins") return listPins();
    if (m == "get_pin") {
        return getPin(params.contains("pin") ? params["pin"].toString() : params["pinName"].toString());
    }
    if (m == "configure_pin") {
        return configurePin(
            params.contains("pin") ? params["pin"].toString() : params["pinName"].toString(),
            params.value("config").toObject(),
            params.value("force").toBool(false)
        );
    }
    if (m == "configure_pins") {
        return configurePins(
            params.value("assignments").toArray(),
            params.value("force").toBool(false)
        );
    }
    if (m == "list_peripherals") return listPeripherals();
    if (m == "get_peripheral") return getPeripheral(params["name"].toString());
    if (m == "configure_peripheral") {
        return configurePeripheral(
            params["name"].toString(),
            params.value("config").toObject(),
            params.value("force").toBool(false)
        );
    }
    if (m == "get_available_pins") {
        return getAvailablePins(params["peripheral"].toString(), params["signal"].toString());
    }
    if (m == "get_free_pins") return getFreePins();
    if (m == "validate_hardware_configuration") return validateHardwareConfiguration();
    if (m == "get_hardware_configuration") return getHardwareConfiguration();
    if (m == "create_custom_hardware") {
        return createCustomHardware(
            params.contains("definition") ? params["definition"].toObject() : params
        );
    }
    if (m == "save_hardware_definition") {
        return saveHardwareDefinition(params["id"].toString(), params["filePath"].toString());
    }
    if (m == "load_hardware_definition") {
        return loadHardwareDefinition(params["filePath"].toString());
    }

    QJsonObject err;
    err["success"] = false;
    err["error"] = "Unknown method: " + method;
    return err;
}

// -----------------------------------------------------------------------------
// Hardware & Pinmux Implementation
// -----------------------------------------------------------------------------

QJsonObject DesignerController::getTarget() const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }
    const auto& hw = m_project->hardwareConfig();
    res["success"] = true;
    res["targetType"] = hw.targetType;
    res["targetId"] = hw.deviceId.isEmpty() ? hw.boardId : hw.deviceId;
    res["vendor"] = hw.vendor;
    res["family"] = hw.family;
    res["series"] = hw.series;
    res["deviceId"] = hw.deviceId;
    res["boardId"] = hw.boardId;
    res["architecture"] = hw.architecture;
    res["core"] = hw.core;
    res["package"] = hw.package;
    res["flashBytes"] = static_cast<qint64>(hw.flashBytes);
    res["ramBytes"] = static_cast<qint64>(hw.ramBytes);
    res["pinCount"] = hw.pins.size();
    res["peripheralCount"] = hw.peripherals.size();
    return res;
}

QJsonObject DesignerController::setTarget(const QString& targetType, const QString& targetId) {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }

    auto& db = Hardware::DeviceDatabase::instance();
    auto hw = m_project->hardwareConfig();
    QString tType = targetType.trimmed().toLower();

    if (tType == "board") {
        Hardware::BoardDefinition board = db.findBoard(targetId);
        if (board.id.isEmpty()) {
            for (const auto& b : db.allBoards()) {
                if (b.id.compare(targetId, Qt::CaseInsensitive) == 0 ||
                    b.name.compare(targetId, Qt::CaseInsensitive) == 0) {
                    board = b;
                    break;
                }
            }
        }
        if (board.id.isEmpty()) {
            res["success"] = false;
            res["error"] = "Board not found: " + targetId;
            return res;
        }

        hw.targetType = "board";
        hw.boardId = board.id;
        hw.deviceId = board.mcuPartNumber;
        hw.vendor = board.manufacturer;
        hw.family = board.boardFamily;
        hw.architecture = board.architecture;

        Hardware::DeviceDefinition mcu = db.findDevice(board.mcuPartNumber);
        if (!mcu.partNumber.isEmpty()) {
            hw.core = mcu.core;
            hw.package = mcu.package;
            hw.flashBytes = mcu.flashBytes;
            hw.ramBytes = mcu.ramBytes;
            hw.series = mcu.series;
        }

        hw.pins.clear();
        if (!mcu.partNumber.isEmpty()) {
            for (const auto& p : mcu.pins) {
                Hardware::PinConfiguration pcfg;
                pcfg.pin = p.name;
                pcfg.label = board.physicalPinLabels.value(p.name, p.name);
                pcfg.gpio = p.name;
                pcfg.mode = (p.isPower ? "power" : (p.isGround ? "ground" : "None"));
                hw.pins.insert(p.name, pcfg);
            }
        }
        hw.peripherals.clear();

        m_project->setHardwareConfig(hw);
        m_project->setDirty(true);
        emit m_project->hardwareConfigChanged();

        res["success"] = true;
        res["target"] = getTarget();
        return res;
    } else { // "device" or "mcu"
        Hardware::DeviceDefinition dev = db.findDevice(targetId);
        if (dev.partNumber.isEmpty()) {
            for (const auto& d : db.allDevices()) {
                if (d.partNumber.compare(targetId, Qt::CaseInsensitive) == 0) {
                    dev = d;
                    break;
                }
            }
        }
        if (dev.partNumber.isEmpty()) {
            res["success"] = false;
            res["error"] = "Device not found: " + targetId;
            return res;
        }

        hw.targetType = "device";
        hw.boardId.clear();
        hw.deviceId = dev.partNumber;
        hw.vendor = dev.vendor;
        hw.family = dev.family;
        hw.series = dev.series;
        hw.architecture = dev.architecture;
        hw.core = dev.core;
        hw.package = dev.package;
        hw.flashBytes = dev.flashBytes;
        hw.ramBytes = dev.ramBytes;

        hw.pins.clear();
        for (const auto& p : dev.pins) {
            Hardware::PinConfiguration pcfg;
            pcfg.pin = p.name;
            pcfg.label = p.name;
            pcfg.gpio = p.name;
            pcfg.mode = (p.isPower ? "power" : (p.isGround ? "ground" : "None"));
            hw.pins.insert(p.name, pcfg);
        }
        hw.peripherals.clear();

        m_project->setHardwareConfig(hw);
        m_project->setDirty(true);
        emit m_project->hardwareConfigChanged();

        res["success"] = true;
        res["target"] = getTarget();
        return res;
    }
}

QJsonObject DesignerController::listDevices(const QString& vendor, const QString& family, const QString& search) const {
    QJsonObject res;
    auto& db = Hardware::DeviceDatabase::instance();
    auto allDevs = db.allDevices();
    QJsonArray arr;
    for (const auto& d : allDevs) {
        if (!vendor.isEmpty() && d.vendor.compare(vendor, Qt::CaseInsensitive) != 0) continue;
        if (!family.isEmpty() && d.family.compare(family, Qt::CaseInsensitive) != 0) continue;
        if (!search.isEmpty()) {
            QString q = search.toLower();
            if (!d.partNumber.toLower().contains(q) &&
                !d.vendor.toLower().contains(q) &&
                !d.family.toLower().contains(q) &&
                !d.core.toLower().contains(q)) {
                continue;
            }
        }
        QJsonObject o;
        o["partNumber"] = d.partNumber;
        o["vendor"] = d.vendor;
        o["family"] = d.family;
        o["series"] = d.series;
        o["architecture"] = d.architecture;
        o["core"] = d.core;
        o["package"] = d.package;
        o["flashBytes"] = static_cast<qint64>(d.flashBytes);
        o["ramBytes"] = static_cast<qint64>(d.ramBytes);
        o["pinCount"] = d.pins.size();
        arr.append(o);
    }
    res["success"] = true;
    res["count"] = arr.size();
    res["devices"] = arr;
    return res;
}

QJsonObject DesignerController::listBoards(const QString& vendor, const QString& mcu, const QString& search) const {
    QJsonObject res;
    auto& db = Hardware::DeviceDatabase::instance();
    auto allBoards = db.allBoards();
    QJsonArray arr;
    for (const auto& b : allBoards) {
        if (!vendor.isEmpty() && b.manufacturer.compare(vendor, Qt::CaseInsensitive) != 0) continue;
        if (!mcu.isEmpty() && b.mcuPartNumber.compare(mcu, Qt::CaseInsensitive) != 0) continue;
        if (!search.isEmpty()) {
            QString q = search.toLower();
            if (!b.name.toLower().contains(q) &&
                !b.manufacturer.toLower().contains(q) &&
                !b.mcuPartNumber.toLower().contains(q) &&
                !b.boardFamily.toLower().contains(q)) {
                continue;
            }
        }
        QJsonObject o;
        o["id"] = b.id;
        o["name"] = b.name;
        o["manufacturer"] = b.manufacturer;
        o["boardFamily"] = b.boardFamily;
        o["mcu"] = b.mcuPartNumber;
        o["architecture"] = b.architecture;
        o["connectorsCount"] = b.connectors.size();
        arr.append(o);
    }
    res["success"] = true;
    res["count"] = arr.size();
    res["boards"] = arr;
    return res;
}

QJsonObject DesignerController::getDeviceInfo(const QString& deviceId) const {
    QJsonObject res;
    auto dev = Hardware::DeviceDatabase::instance().findDevice(deviceId);
    if (dev.partNumber.isEmpty()) {
        res["success"] = false;
        res["error"] = "Device not found: " + deviceId;
        return res;
    }
    res["success"] = true;
    res["device"] = dev.toJson();
    return res;
}

QJsonObject DesignerController::getBoardInfo(const QString& boardId) const {
    QJsonObject res;
    auto board = Hardware::DeviceDatabase::instance().findBoard(boardId);
    if (board.id.isEmpty()) {
        res["success"] = false;
        res["error"] = "Board not found: " + boardId;
        return res;
    }
    res["success"] = true;
    res["board"] = board.toJson();
    return res;
}

QJsonObject DesignerController::getPinout(const QString& deviceId) const {
    QJsonObject res;
    QString targetDev = deviceId;
    if (targetDev.isEmpty() && m_project) {
        targetDev = m_project->hardwareConfig().deviceId;
    }
    if (targetDev.isEmpty()) {
        res["success"] = false;
        res["error"] = "No device specified or active in project";
        return res;
    }

    auto dev = Hardware::DeviceDatabase::instance().findDevice(targetDev);
    if (dev.partNumber.isEmpty()) {
        res["success"] = false;
        res["error"] = "Device not found: " + targetDev;
        return res;
    }

    QJsonArray pinsArr;
    for (const auto& p : dev.pins) {
        QJsonObject pObj;
        pObj["pin"] = p.name;
        pObj["name"] = p.name;
        pObj["physicalPin"] = p.physicalPin;
        pObj["description"] = p.description;
        pObj["type"] = (p.isPower ? "power" : (p.isGround ? "ground" : (p.isReset ? "reset" : (p.isReserved ? "reserved" : "io"))));
        QJsonArray afs;
        for (const auto& af : p.alternateFunctions) afs.append(af);
        pObj["alternateFunctions"] = afs;
        pinsArr.append(pObj);
    }

    res["success"] = true;
    res["deviceId"] = dev.partNumber;
    res["package"] = dev.package;
    res["pinCount"] = dev.pins.size();
    res["pins"] = pinsArr;
    return res;
}

QJsonObject DesignerController::listPins() const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }
    const auto& hw = m_project->hardwareConfig();
    auto dev = Hardware::DeviceDatabase::instance().findDevice(hw.deviceId);

    QJsonArray pinsArr;
    for (const auto& p : dev.pins) {
        QJsonObject pObj;
        pObj["pin"] = p.name;
        pObj["name"] = p.name;
        pObj["physicalPin"] = p.physicalPin;
        pObj["description"] = p.description;
        pObj["type"] = (p.isPower ? "power" : (p.isGround ? "ground" : (p.isReset ? "reset" : (p.isReserved ? "reserved" : "io"))));

        if (hw.pins.contains(p.name)) {
            const auto& cfg = hw.pins[p.name];
            pObj["label"] = cfg.label;
            pObj["mode"] = cfg.mode;
            pObj["pull"] = cfg.pull;
            pObj["speed"] = cfg.speed;
            pObj["outputType"] = cfg.outputType;
            pObj["initialOutput"] = cfg.initialOutput;
            pObj["interrupt"] = cfg.interrupt;
            pObj["alternateFunction"] = cfg.alternateFunction;
            bool isAssigned = (cfg.mode != "None" && cfg.mode != "Unassigned" &&
                               cfg.mode != "power" && cfg.mode != "ground");
            pObj["isAssigned"] = isAssigned;
        } else {
            pObj["label"] = p.name;
            pObj["mode"] = "None";
            pObj["isAssigned"] = false;
        }

        QJsonArray afs;
        for (const auto& af : p.alternateFunctions) afs.append(af);
        pObj["alternateFunctions"] = afs;

        pinsArr.append(pObj);
    }

    res["success"] = true;
    res["count"] = pinsArr.size();
    res["pins"] = pinsArr;
    return res;
}

QJsonObject DesignerController::getPin(const QString& pinName) const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }
    const auto& hw = m_project->hardwareConfig();
    auto dev = Hardware::DeviceDatabase::instance().findDevice(hw.deviceId);
    const auto* pinDef = dev.findPin(pinName);
    if (!pinDef) {
        res["success"] = false;
        res["error"] = "Pin not found on active device: " + pinName;
        return res;
    }

    res["success"] = true;
    res["pin"] = pinDef->name;
    res["name"] = pinDef->name;
    res["physicalPin"] = pinDef->physicalPin;
    res["description"] = pinDef->description;
    res["type"] = (pinDef->isPower ? "power" : (pinDef->isGround ? "ground" : (pinDef->isReset ? "reset" : (pinDef->isReserved ? "reserved" : "io"))));

    QJsonArray afs;
    for (const auto& af : pinDef->alternateFunctions) afs.append(af);
    res["alternateFunctions"] = afs;

    if (hw.pins.contains(pinName)) {
        res["configuration"] = hw.pins[pinName].toJson();
    } else {
        Hardware::PinConfiguration defCfg;
        defCfg.pin = pinDef->name;
        defCfg.label = pinDef->name;
        res["configuration"] = defCfg.toJson();
    }
    return res;
}

QJsonObject DesignerController::configurePin(const QString& pinName, const QJsonObject& config, bool force) {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }

    auto hw = m_project->hardwareConfig();
    auto dev = Hardware::DeviceDatabase::instance().findDevice(hw.deviceId);
    if (dev.partNumber.isEmpty() && !hw.boardId.isEmpty()) {
        auto b = Hardware::DeviceDatabase::instance().findBoard(hw.boardId);
        dev = Hardware::DeviceDatabase::instance().findDevice(b.mcuPartNumber);
    }

    Hardware::PinMuxEngine engine;
    engine.setDevice(dev);
    engine.loadConfig(hw);

    Hardware::PinConfiguration pcfg;
    if (hw.pins.contains(pinName)) {
        pcfg = hw.pins[pinName];
    } else {
        pcfg.pin = pinName;
    }

    if (config.contains("mode")) {
        pcfg.mode = config["mode"].toString();
        if (pcfg.mode != "AlternateFunction" && !config.contains("alternateFunction")) {
            pcfg.alternateFunction.clear();
        }
    }
    if (config.contains("label")) pcfg.label = config["label"].toString();
    if (config.contains("pull")) pcfg.pull = config["pull"].toString();
    if (config.contains("speed")) pcfg.speed = config["speed"].toString();
    if (config.contains("alternateFunction")) pcfg.alternateFunction = config["alternateFunction"].toString();
    if (config.contains("outputType")) pcfg.outputType = config["outputType"].toString();
    if (config.contains("initialOutput")) pcfg.initialOutput = config["initialOutput"].toString();
    if (config.contains("interrupt")) pcfg.interrupt = config["interrupt"].toString();
    if (config.contains("locked")) pcfg.locked = config["locked"].toBool();

    Hardware::ConflictInfo conflict;
    bool assigned = engine.assignPin(pinName, pcfg, force, &conflict);
    if (!assigned) {
        res["success"] = false;
        res["conflict"] = true;
        res["error"] = conflict.reason;
        res["currentOwner"] = conflict.currentOwner;
        res["proposedOwner"] = conflict.proposedOwner;
        return res;
    }

    hw = engine.currentConfig();
    m_project->setHardwareConfig(hw);
    m_project->setDirty(true);
    emit m_project->hardwareConfigChanged();

    res["success"] = true;
    res["pin"] = pinName;
    res["config"] = pcfg.toJson();
    return res;
}

QJsonObject DesignerController::configurePins(const QJsonArray& assignments, bool force) {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }

    auto hw = m_project->hardwareConfig();
    auto dev = Hardware::DeviceDatabase::instance().findDevice(hw.deviceId);
    if (dev.partNumber.isEmpty() && !hw.boardId.isEmpty()) {
        auto b = Hardware::DeviceDatabase::instance().findBoard(hw.boardId);
        dev = Hardware::DeviceDatabase::instance().findDevice(b.mcuPartNumber);
    }

    Hardware::PinMuxEngine engine;
    engine.setDevice(dev);
    engine.loadConfig(hw);

    QJsonArray configured;
    QJsonArray conflicts;

    for (const auto& val : assignments) {
        QJsonObject item = val.toObject();
        QString pinName = item["pin"].toString();
        if (pinName.isEmpty()) continue;

        Hardware::PinConfiguration pcfg;
        if (hw.pins.contains(pinName)) pcfg = hw.pins[pinName];
        else pcfg.pin = pinName;

        if (item.contains("mode")) {
            pcfg.mode = item["mode"].toString();
            if (pcfg.mode != "AlternateFunction" && !item.contains("alternateFunction")) {
                pcfg.alternateFunction.clear();
            }
        }
        if (item.contains("label")) pcfg.label = item["label"].toString();
        if (item.contains("pull")) pcfg.pull = item["pull"].toString();
        if (item.contains("speed")) pcfg.speed = item["speed"].toString();
        if (item.contains("alternateFunction")) pcfg.alternateFunction = item["alternateFunction"].toString();
        if (item.contains("outputType")) pcfg.outputType = item["outputType"].toString();
        if (item.contains("initialOutput")) pcfg.initialOutput = item["initialOutput"].toString();
        if (item.contains("interrupt")) pcfg.interrupt = item["interrupt"].toString();
        if (item.contains("locked")) pcfg.locked = item["locked"].toBool();

        Hardware::ConflictInfo conflict;
        if (engine.assignPin(pinName, pcfg, force, &conflict)) {
            configured.append(pcfg.toJson());
        } else {
            QJsonObject confObj;
            confObj["pin"] = pinName;
            confObj["reason"] = conflict.reason;
            confObj["currentOwner"] = conflict.currentOwner;
            confObj["proposedOwner"] = conflict.proposedOwner;
            conflicts.append(confObj);
        }
    }

    hw = engine.currentConfig();
    m_project->setHardwareConfig(hw);
    m_project->setDirty(true);
    emit m_project->hardwareConfigChanged();

    res["success"] = conflicts.isEmpty();
    res["configured"] = configured;
    if (!conflicts.isEmpty()) {
        res["conflicts"] = conflicts;
    }
    return res;
}

QJsonObject DesignerController::listPeripherals() const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }
    const auto& hw = m_project->hardwareConfig();
    auto dev = Hardware::DeviceDatabase::instance().findDevice(hw.deviceId);

    QJsonArray arr;
    for (const auto& periph : dev.peripheralDescriptors) {
        QJsonObject pObj;
        pObj["name"] = periph.name;
        pObj["type"] = periph.type;
        pObj["description"] = periph.defaults.value("description").toString();
        bool enabled = false;
        if (hw.peripherals.contains(periph.name)) {
            enabled = hw.peripherals[periph.name].enabled;
            pObj["configuredPins"] = QJsonObject::fromVariantMap([&](){
                QVariantMap vm;
                for (auto it = hw.peripherals[periph.name].assignedPins.begin(); it != hw.peripherals[periph.name].assignedPins.end(); ++it) {
                    vm.insert(it.key(), it.value());
                }
                return vm;
            }());
            pObj["parameters"] = QJsonObject::fromVariantMap(hw.peripherals[periph.name].parameters);
        } else {
            pObj["configuredPins"] = QJsonObject();
            pObj["parameters"] = QJsonObject();
        }
        pObj["enabled"] = enabled;
        arr.append(pObj);
    }

    res["success"] = true;
    res["count"] = arr.size();
    res["peripherals"] = arr;
    return res;
}

QJsonObject DesignerController::getPeripheral(const QString& name) const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }
    const auto& hw = m_project->hardwareConfig();
    auto dev = Hardware::DeviceDatabase::instance().findDevice(hw.deviceId);

    const Hardware::PeripheralDescriptor* foundDesc = dev.findPeripheral(name);
    if (!foundDesc) {
        res["success"] = false;
        res["error"] = "Peripheral not found on active device: " + name;
        return res;
    }

    res["success"] = true;
    res["name"] = foundDesc->name;
    res["type"] = foundDesc->type;
    res["description"] = foundDesc->defaults.value("description").toString();

    QJsonObject sigs;
    for (auto it = foundDesc->signalOptions.begin(); it != foundDesc->signalOptions.end(); ++it) {
        QJsonArray pinArr;
        for (const auto& p : it.value()) pinArr.append(p);
        sigs[it.key()] = pinArr;
    }
    res["signals"] = sigs;

    if (hw.peripherals.contains(foundDesc->name)) {
        res["configuration"] = hw.peripherals[foundDesc->name].toJson();
    } else {
        Hardware::PeripheralConfiguration defCfg;
        defCfg.name = foundDesc->name;
        defCfg.type = foundDesc->type;
        defCfg.enabled = false;
        res["configuration"] = defCfg.toJson();
    }
    return res;
}

QJsonObject DesignerController::configurePeripheral(const QString& name, const QJsonObject& config, bool force) {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }

    auto hw = m_project->hardwareConfig();
    auto dev = Hardware::DeviceDatabase::instance().findDevice(hw.deviceId);

    Hardware::PinMuxEngine engine;
    engine.setDevice(dev);
    engine.loadConfig(hw);

    const auto* pDesc = dev.findPeripheral(name);
    QString periphType = config.value("type").toString(pDesc ? pDesc->type : QString());

    QMap<QString, QString> signalMap;
    if (config.contains("pins") && config["pins"].isObject()) {
        QJsonObject pinsObj = config["pins"].toObject();
        for (auto it = pinsObj.begin(); it != pinsObj.end(); ++it) {
            signalMap.insert(it.key(), it.value().toString());
        }
    }

    QMap<QString, QVariant> params;
    if (config.contains("parameters") && config["parameters"].isObject()) {
        QJsonObject paramsObj = config["parameters"].toObject();
        for (auto it = paramsObj.begin(); it != paramsObj.end(); ++it) {
            params.insert(it.key(), it.value().toVariant());
        }
    }

    bool enabled = config.value("enabled").toBool(true);
    if (!enabled) {
        engine.unassignPeripheral(name);
        hw = engine.currentConfig();
        if (hw.peripherals.contains(name)) {
            hw.peripherals[name].enabled = false;
        }
    } else {
        bool ok = engine.assignPeripheral(name, periphType, signalMap, params, force);
        if (!ok) {
            res["success"] = false;
            res["error"] = "Failed to assign peripheral pins due to conflict or invalid mapping";
            return res;
        }
        hw = engine.currentConfig();
    }

    m_project->setHardwareConfig(hw);
    m_project->setDirty(true);
    emit m_project->hardwareConfigChanged();

    res["success"] = true;
    res["peripheral"] = name;
    if (hw.peripherals.contains(name)) {
        res["configuration"] = hw.peripherals[name].toJson();
    }
    return res;
}

QJsonObject DesignerController::getAvailablePins(const QString& peripheral, const QString& signal) const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }
    const auto& hw = m_project->hardwareConfig();
    auto dev = Hardware::DeviceDatabase::instance().findDevice(hw.deviceId);

    Hardware::PinMuxEngine engine;
    engine.setDevice(dev);
    engine.loadConfig(hw);

    QStringList pins = engine.availablePinsForSignal(peripheral, signal);
    QJsonArray arr;
    for (const auto& p : pins) arr.append(p);

    res["success"] = true;
    res["peripheral"] = peripheral;
    res["signal"] = signal;
    res["availablePins"] = arr;
    return res;
}

QJsonObject DesignerController::getFreePins() const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }
    const auto& hw = m_project->hardwareConfig();
    auto dev = Hardware::DeviceDatabase::instance().findDevice(hw.deviceId);

    Hardware::PinMuxEngine engine;
    engine.setDevice(dev);
    engine.loadConfig(hw);

    QStringList freePins = engine.getFreePins();
    QJsonArray arr;
    for (const auto& p : freePins) arr.append(p);

    res["success"] = true;
    res["count"] = arr.size();
    res["freePins"] = arr;
    return res;
}

QJsonObject DesignerController::validateHardwareConfiguration() const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }
    const auto& hw = m_project->hardwareConfig();
    auto dev = Hardware::DeviceDatabase::instance().findDevice(hw.deviceId);

    Hardware::PinMuxEngine engine;
    engine.setDevice(dev);
    engine.loadConfig(hw);

    QStringList errors, warnings;
    bool valid = engine.validateConfiguration(&errors, &warnings);

    QJsonArray errArr, warnArr;
    for (const auto& e : errors) errArr.append(e);
    for (const auto& w : warnings) warnArr.append(w);

    res["success"] = true;
    res["valid"] = valid;
    res["errors"] = errArr;
    res["warnings"] = warnArr;
    return res;
}

QJsonObject DesignerController::getHardwareConfiguration() const {
    QJsonObject res;
    if (!m_project) {
        res["success"] = false;
        res["error"] = "No active project";
        return res;
    }
    res["success"] = true;
    res["configuration"] = m_project->hardwareConfig().toJson();
    return res;
}

QJsonObject DesignerController::createCustomHardware(const QJsonObject& definition) {
    QJsonObject res;
    auto& db = Hardware::DeviceDatabase::instance();
    if (definition.contains("partNumber")) {
        Hardware::DeviceDefinition dev = Hardware::DeviceDefinition::fromJson(definition);
        if (dev.partNumber.isEmpty()) {
            res["success"] = false;
            res["error"] = "Custom device requires a partNumber";
            return res;
        }
        bool ok = db.addCustomDevice(dev);
        res["success"] = ok;
        res["id"] = dev.partNumber;
        return res;
    } else if (definition.contains("id") || definition.contains("name")) {
        Hardware::BoardDefinition board = Hardware::BoardDefinition::fromJson(definition);
        if (board.id.isEmpty()) {
            board.id = board.name.toLower().replace(" ", "_");
        }
        bool ok = db.addCustomBoard(board);
        res["success"] = ok;
        res["id"] = board.id;
        return res;
    }

    res["success"] = false;
    res["error"] = "Invalid custom hardware definition: must specify partNumber (MCU) or id/name (Board)";
    return res;
}

QJsonObject DesignerController::saveHardwareDefinition(const QString& id, const QString& filePath) {
    QJsonObject res;
    QString err;
    bool ok = Hardware::DeviceDatabase::instance().exportHardwareDefinition(id, filePath, &err);
    res["success"] = ok;
    if (ok) {
        res["id"] = id;
        res["filePath"] = filePath;
    } else {
        res["error"] = err;
    }
    return res;
}

QJsonObject DesignerController::loadHardwareDefinition(const QString& filePath) {
    QJsonObject res;
    QString id, err;
    bool ok = Hardware::DeviceDatabase::instance().importHardwareDefinition(filePath, &id, &err);
    res["success"] = ok;
    if (ok) {
        res["id"] = id;
        res["filePath"] = filePath;
    } else {
        res["error"] = err;
    }
    return res;
}

