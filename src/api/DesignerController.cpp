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

    QJsonObject err;
    err["success"] = false;
    err["error"] = "Unknown method: " + method;
    return err;
}
