#pragma once

#include <QString>
#include <QJsonObject>
#include <QList>
#include <QStandardPaths>
#include "DisplayConfig.h"
#include "CanvasScene.h"
#include "ComponentDefinition.h"
#include "ColorStyle.h"
#include "HardwareModel.h"

class Project : public QObject {
    Q_OBJECT

public:
    explicit Project(CanvasScene* scene, QObject* parent = nullptr);

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

private:
    CanvasScene* m_scene = nullptr;
    QString m_name = "MyEmbeddedApp";
    QString m_filePath;
    QString m_targetFramework = "ugfx"; // Default to µGFX (royalty-free)
    DisplayConfig m_displayConfig;
    Hardware::HardwareConfig m_hardwareConfig;
    bool m_dirty = false;
    QList<ComponentDefinition> m_componentLibrary;

    QList<ColorStyle> m_colorStyles;
    void initDefaultStyles();
};
