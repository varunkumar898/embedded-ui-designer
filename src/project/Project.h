#pragma once

#include <QString>
#include <QJsonObject>
#include <QList>
#include <QStandardPaths>
#include "DisplayConfig.h"
#include "CanvasScene.h"

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

    // AppData storage & Autosave
    static QString appDataDirectory();
    bool autoSave();
    bool loadSampleProject();

    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& root);

    // Component Factory
    static UIComponent* createComponentInstance(const QString& type, const QString& id);

    // Named Color Styles
    QList<struct ColorStyle> colorStyles() const { return m_colorStyles; }
    void addColorStyle(const struct ColorStyle& style);
    void updateColorStyle(const QString& name, const QColor& newColor);
    void removeColorStyle(const QString& name);
    QColor resolveColor(const QString& styleName, const QColor& defaultColor = QColor()) const;
    bool hasColorStyle(const QString& name) const;
    void setColorStyles(const QList<struct ColorStyle>& styles);

    // Custom Component Definitions
    QList<class CustomComponentDefinition> customComponentDefinitions() const { return m_customComponentDefinitions; }
    void addCustomComponentDefinition(const class CustomComponentDefinition& def);
    class CustomComponentDefinition findCustomComponentDefinition(const QString& id) const;
    void removeCustomComponentDefinition(const QString& id);

signals:
    void projectModified();
    void projectLoaded();
    void projectSaved(const QString& filePath);
    void colorStylesChanged();
    void customComponentsChanged();

private:
    CanvasScene* m_scene = nullptr;
    QString m_name = "MyEmbeddedApp";
    QString m_filePath;
    QString m_targetFramework = "ugfx"; // Default to µGFX (royalty-free)
    DisplayConfig m_displayConfig;
    bool m_dirty = false;

    QList<struct ColorStyle> m_colorStyles;
    QList<class CustomComponentDefinition> m_customComponentDefinitions;
    void initDefaultStyles();
};
