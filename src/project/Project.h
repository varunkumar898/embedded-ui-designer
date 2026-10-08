#pragma once

#include <QString>
#include <QJsonObject>
#include <QList>
#include <QStandardPaths>
#include <QPointer>
#include "DisplayConfig.h"
#include "CanvasScene.h"
#include "ComponentDefinition.h"

#include "ColorStyle.h"
#include "HardwareModel.h"
#include "Screen.h"
#include "DataSource.h"
#include "DataBinding.h"

class Project : public QObject {
    Q_OBJECT

public:
    explicit Project(CanvasScene* scene, QObject* parent = nullptr);
    ~Project() override;

    QString projectName() const { return m_name; }
    void setProjectName(const QString& name);

    QString projectFilePath() const { return m_filePath; }
    void setProjectFilePath(const QString& path) { m_filePath = path; }

    QString targetFramework() const { return m_targetFramework; }
    void setTargetFramework(const QString& target);

    DisplayConfig displayConfig() const { return m_displayConfig; }
    void setDisplayConfig(const DisplayConfig& config);

    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty);

    void newProject(const QString& name = "MyEmbeddedApp", int width = 320, int height = 240);
    bool saveToFile(const QString& filePath);
    bool loadFromFile(const QString& filePath);
    int importFromFile(const QString& filePath, QPointF offset = QPointF(20, 20));

    // AppData storage & Autosave
    static QString appDataDirectory();
    bool autoSave();
    bool loadSampleProject();

    // ── Multi-Screen Model (Phase 1A) ────────────────────────────────────
    const QList<Screen*>& screens() const { return m_screens; }
    Screen* activeScreen() const { return m_activeScreen; }
    void setActiveScreen(Screen* screen);
    void setActiveScreenById(const QString& screenId);
    Screen* addScreen(const QString& name = QString(), int width = 0, int height = 0);
    void addScreen(Screen* screen);
    bool removeScreen(const QString& screenId);
    Screen* takeScreen(const QString& screenId);
    bool renameScreen(const QString& screenId, const QString& newName);
    Screen* duplicateScreen(const QString& screenId);
    bool moveScreen(int fromIndex, int toIndex);
    Screen* findScreen(const QString& screenId) const;
    int screenCount() const { return m_screens.size(); }
    int activeScreenIndex() const;

    // ── Project-level Data Sources (Phase 1B) ─────────────────────────────
    const QList<DataSource>& dataSources() const { return m_dataSources; }
    void addDataSource(const DataSource& source);
    void removeDataSource(const QString& sourceId);
    const DataSource* findDataSource(const QString& sourceId) const;
    void setDataSources(const QList<DataSource>& sources);

    // ── Project-level Data Bindings (Phase 1B) ────────────────────────────
    const QList<DataBinding>& dataBindings() const { return m_dataBindings; }
    void addDataBinding(const DataBinding& binding);
    void removeDataBinding(const QString& componentId, const QString& propertyName);
    void setDataBindings(const QList<DataBinding>& bindings);

    // Custom component library
    const QList<ComponentDefinition>& componentLibrary() const { return m_componentLibrary; }
    void addComponentDefinition(const ComponentDefinition& def);
    void removeComponentDefinition(const QString& id);
    const ComponentDefinition* findDefinition(const QString& id) const;
    void updateVariantsForDefinition(const QString& id, const QList<ComponentVariant>& variants);

    // ── Named Color Styles ────────────────────────────────────────────────
    QList<ColorStyle> colorStyles() const { return m_colorStyles; }
    void addColorStyle(const ColorStyle& style);
    /// Change a style's color and immediately re-apply it to every canvas
    /// component that references it (live propagation, no reload).
    void updateColorStyle(const QString& name, const QColor& newColor);
    void removeColorStyle(const QString& name);
    QColor resolveColor(const QString& styleName, const QColor& defaultColor = QColor()) const;
    bool hasColorStyle(const QString& name) const;
    void setColorStyles(const QList<ColorStyle>& styles);

    // ── Universal Hardware Configuration ──────────────────────────────────
    Hardware::HardwareConfig hardwareConfig() const { return m_hardwareConfig; }
    void setHardwareConfig(const Hardware::HardwareConfig& config);

    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& root);

    // Component Factory
    static UIComponent* createComponentInstance(const QString& type, const QString& id);

signals:
    void projectModified();
    void projectLoaded();
    void projectSaved(const QString& filePath);
    void colorStylesChanged();
    void hardwareConfigChanged();

    // Screen signals
    void screenAdded(Screen* screen);
    void screenRemoved(const QString& screenId);
    void screenRenamed(const QString& screenId, const QString& newName);
    void activeScreenChanged(Screen* screen);
    void screenListChanged();

    // Data Source & Binding signals
    void dataSourcesChanged();
    void dataBindingsChanged();

private:
    void clearScreens();
    void syncSceneToActiveScreen();
    void connectSceneSignals();

    QPointer<CanvasScene> m_scene;
    QString m_name = "MyEmbeddedApp";

    QString m_filePath;
    QString m_targetFramework = "ugfx"; // Default to µGFX (royalty-free)
    DisplayConfig m_displayConfig;
    Hardware::HardwareConfig m_hardwareConfig;
    bool m_dirty = false;
    QList<ComponentDefinition> m_componentLibrary;

    QList<Screen*> m_screens;
    Screen* m_activeScreen = nullptr;

    QList<DataSource> m_dataSources;
    QList<DataBinding> m_dataBindings;

    QList<ColorStyle> m_colorStyles;
    void initDefaultStyles();
};

