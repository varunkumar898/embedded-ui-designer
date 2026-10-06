#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QString>
#include <QList>

class MainWindow;
class CanvasScene;
class CanvasView;
class Project;
class QUndoStack;
class PropertiesPanel;
class LayerPanel;
class UIComponent;

class DesignerController : public QObject {
    Q_OBJECT

public:
    explicit DesignerController(MainWindow* window, QObject* parent = nullptr);

    // Project Tools
    QJsonObject getProject() const;
    QJsonObject getProjectMetadata() const;
    QJsonObject createProject(const QString& name, int width, int height);
    QJsonObject saveProject(const QString& filePath = QString());
    QJsonObject loadProject(const QString& filePath);

    // Component Discovery Tools
    QJsonObject listComponentTypes() const;
    QJsonObject getComponentSchema(const QString& type = QString()) const;
    QJsonObject getComponent(const QString& id) const;
    QJsonObject getComponentTree() const;

    // Component Creation & Deletion
    QJsonObject createComponent(const QString& type, double x, double y, double width = -1, double height = -1, const QString& requestedId = QString(), const QJsonObject& properties = QJsonObject());
    QJsonObject duplicateComponent(const QString& id, double dx = 20, double dy = 20);
    QJsonObject deleteComponent(const QString& id);

    // Component Editing
    QJsonObject updateComponent(const QString& id, const QJsonObject& params);
    QJsonObject setComponentProperty(const QString& id, const QString& propName, const QJsonValue& val);
    QJsonObject setComponentProperties(const QString& id, const QJsonObject& properties);
    QJsonObject moveComponent(const QString& id, double x, double y, bool relative = false);
    QJsonObject resizeComponent(const QString& id, double width, double height);

    // Selection Tools
    QJsonObject getSelectedComponents() const;
    QJsonObject selectComponent(const QString& id, bool addToSelection = false);
    QJsonObject clearSelection();

    // Canvas Tools
    QJsonObject getCanvas() const;
    QJsonObject getCanvasSize() const;
    QJsonObject clearCanvas();

    // History Tools
    QJsonObject undo();
    QJsonObject redo();

    // Export Tools
    QJsonObject exportProject(const QString& target, const QString& outputDir = QString());
    QJsonObject exportComponent(const QString& id, const QString& target = "qml");

    // Unified dispatch helper for JSON-RPC method invocation
    QJsonObject dispatch(const QString& method, const QJsonObject& params);

    // Hardware & Pinmux Tools (Phases 22, 23, 24)
    QJsonObject getTarget() const;
    QJsonObject setTarget(const QString& targetType, const QString& targetId);
    QJsonObject listDevices(const QString& vendor = QString(), const QString& family = QString(), const QString& search = QString()) const;
    QJsonObject listBoards(const QString& vendor = QString(), const QString& mcu = QString(), const QString& search = QString()) const;
    QJsonObject getDeviceInfo(const QString& deviceId) const;
    QJsonObject getBoardInfo(const QString& boardId) const;

    QJsonObject getPinout(const QString& deviceId = QString()) const;
    QJsonObject listPins() const;
    QJsonObject getPin(const QString& pinName) const;
    QJsonObject configurePin(const QString& pinName, const QJsonObject& config, bool force = false);
    QJsonObject configurePins(const QJsonArray& assignments, bool force = false);

    QJsonObject listPeripherals() const;
    QJsonObject getPeripheral(const QString& name) const;
    QJsonObject configurePeripheral(const QString& name, const QJsonObject& config, bool force = false);

    QJsonObject getAvailablePins(const QString& peripheral, const QString& signal) const;
    QJsonObject getFreePins() const;

    QJsonObject validateHardwareConfiguration() const;
    QJsonObject getHardwareConfiguration() const;

    QJsonObject createCustomHardware(const QJsonObject& definition);
    QJsonObject saveHardwareDefinition(const QString& id, const QString& filePath);
    QJsonObject loadHardwareDefinition(const QString& filePath);

    // Lookup helper
    UIComponent* findComponentById(const QString& id) const;
    QString generateUniqueId(const QString& typePrefix) const;

private:
    void notifyUiChanges(UIComponent* targetComp = nullptr);
    QJsonObject serializeComponentFull(const UIComponent* comp) const;

    MainWindow* m_window = nullptr;
    CanvasScene* m_scene = nullptr;
    CanvasView* m_view = nullptr;
    Project* m_project = nullptr;
    QUndoStack* m_undoStack = nullptr;
    PropertiesPanel* m_propertiesPanel = nullptr;
    LayerPanel* m_layerPanel = nullptr;
};
