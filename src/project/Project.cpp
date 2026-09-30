#include "Project.h"
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
#include "ColorStyle.h"
#include "CustomComponentDefinition.h"
#include "CustomComponentInstance.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QSet>
#include <QPointF>

Project::Project(CanvasScene* scene, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
{
    m_displayConfig = DisplayConfig();
    initDefaultStyles();

    if (m_scene) {
        connect(m_scene, &CanvasScene::componentChanged, this, [this]() { setDirty(true); });
        connect(m_scene, &CanvasScene::componentAdded, this, [this]() { setDirty(true); });
        connect(m_scene, &CanvasScene::componentRemoved, this, [this]() { setDirty(true); });
    }
}

void Project::setProjectName(const QString& name) {
    if (m_name != name) {
        m_name = name;
        setDirty(true);
    }
}

void Project::setTargetFramework(const QString& target) {
    if (m_targetFramework != target) {
        m_targetFramework = target;
        setDirty(true);
    }
}

void Project::setDisplayConfig(const DisplayConfig& config) {
    m_displayConfig = config;
    if (m_scene) {
        m_scene->setDisplayConfig(config);
    }
    setDirty(true);
}

void Project::setDirty(bool dirty) {
    if (m_dirty != dirty) {
        m_dirty = dirty;
        emit projectModified();
    }
}

QString Project::appDataDirectory() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return path;
}

bool Project::autoSave() {
    QString autoSavePath = QDir(appDataDirectory()).filePath("recovery.euiproj");
    return saveToFile(autoSavePath);
}

bool Project::loadSampleProject() {
    if (loadFromFile(":/examples/sample_dashboard.euiproj")) {
        return true;
    }
    return loadFromFile(":/examples/simple.euiproj");
}

void Project::newProject(const QString& name, const DisplayConfig& config) {
    m_name = name;
    m_filePath.clear();
    m_targetFramework = "qt-for-mcus";
    m_displayConfig = config;

    if (m_scene) {
        m_scene->clearComponents();
        m_scene->setDisplayConfig(config);
        m_scene->setScreenBackgroundColor(Qt::white);
    }

    initDefaultStyles();
    m_customComponentDefinitions.clear();
    emit colorStylesChanged();
    emit customComponentsChanged();

    m_dirty = false;
    emit projectLoaded();
}

void Project::newProject(const QString& name, int width, int height) {
    DisplayConfig cfg;
    cfg.width = width;
    cfg.height = height;
    cfg.colorDepth = 16;
    cfg.type = "LCD";
    cfg.dpi = 96;
    newProject(name, cfg);
}

QJsonObject Project::toJson() const {
    QJsonObject root;
    root["version"] = "1.0";
    root["name"] = m_name;
    root["target"] = m_targetFramework;
    root["display"] = m_displayConfig.toJson();

    // Top-level colorStyles
    QJsonArray stylesArr;
    for (const auto& s : m_colorStyles) {
        stylesArr.append(s.toJson());
    }
    root["colorStyles"] = stylesArr;

    // Top-level customComponents
    QJsonArray customArr;
    for (const auto& def : m_customComponentDefinitions) {
        customArr.append(def.toJson());
    }
    root["customComponents"] = customArr;

    QJsonArray pages;
    QJsonObject mainPage;
    mainPage["id"] = "MainScreen";
    mainPage["width"] = m_displayConfig.width;
    mainPage["height"] = m_displayConfig.height;
    mainPage["backgroundColor"] = m_scene ? m_scene->screenBackgroundColor().name() : "#ffffff";

    QJsonArray compsArray;
    if (m_scene) {
        for (UIComponent* comp : m_scene->uiComponents()) {
            compsArray.append(comp->toJson());
        }
    }
    mainPage["components"] = compsArray;
    pages.append(mainPage);
    root["pages"] = pages;

    QJsonArray assets;
    root["assets"] = assets;

    return root;
}

bool Project::fromJson(const QJsonObject& root) {
    m_name = root.value("name").toString("MyEmbeddedApp");
    m_targetFramework = root.value("target").toString("qt-for-mcus");

    if (root.contains("display") && root.value("display").isObject()) {
        m_displayConfig = DisplayConfig::fromJson(root.value("display").toObject());
    }

    // Load top-level colorStyles
    m_colorStyles.clear();
    if (root.contains("colorStyles") && root.value("colorStyles").isArray()) {
        QJsonArray stylesArr = root.value("colorStyles").toArray();
        for (const auto& v : stylesArr) {
            m_colorStyles.append(ColorStyle::fromJson(v.toObject()));
        }
    } else {
        initDefaultStyles();
    }
    emit colorStylesChanged();

    // Load top-level customComponents
    m_customComponentDefinitions.clear();
    if (root.contains("customComponents") && root.value("customComponents").isArray()) {
        QJsonArray customArr = root.value("customComponents").toArray();
        for (const auto& v : customArr) {
            m_customComponentDefinitions.append(CustomComponentDefinition::fromJson(v.toObject()));
        }
    }
    emit customComponentsChanged();

    if (m_scene) {
        m_scene->clearComponents();
        m_scene->setDisplayConfig(m_displayConfig);

        if (root.contains("pages") && root.value("pages").isArray()) {
            QJsonArray pages = root.value("pages").toArray();
            if (!pages.isEmpty()) {
                QJsonObject mainPage = pages.first().toObject();
                if (mainPage.contains("backgroundColor")) {
                    m_scene->setScreenBackgroundColor(QColor(mainPage.value("backgroundColor").toString()));
                }

                if (mainPage.contains("components") && mainPage.value("components").isArray()) {
                    QJsonArray compsArray = mainPage.value("components").toArray();
                    for (int i = 0; i < compsArray.size(); ++i) {
                        QJsonObject compObj = compsArray[i].toObject();
                        QString type = compObj.value("type").toString();
                        QString id = compObj.value("id").toString();

                        UIComponent* comp = createComponentInstance(type, id);
                        if (comp) {
                            if (auto cci = dynamic_cast<CustomComponentInstance*>(comp)) {
                                cci->setProject(this);
                            }
                            comp->fromJson(compObj);
                            m_scene->addUIComponent(comp);
                        }
                    }
                }
            }
        }

        // Resolve all style refs on canvas components
        for (auto comp : m_scene->uiComponents()) {
            const auto& refs = comp->colorStyleRefs();
            for (auto it = refs.begin(); it != refs.end(); ++it) {
                QString styleName = it.value();
                if (hasColorStyle(styleName)) {
                    comp->applyColorStyle(styleName, resolveColor(styleName));
                }
            }
        }
    }

    m_dirty = false;
    emit projectLoaded();
    return true;
}

bool Project::saveToFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    m_filePath = filePath;
    m_dirty = false;
    emit projectSaved(filePath);
    return true;
}

bool Project::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Fallback: check if the file is embedded in QRC resources
        QString qrcPath = filePath.startsWith(":/") ? filePath : (":/" + filePath);
        if (QFile::exists(qrcPath)) {
            file.setFileName(qrcPath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qWarning() << "Failed to open file for reading:" << filePath;
                return false;
            }
        } else {
            qWarning() << "Failed to open file for reading:" << filePath;
            return false;
        }
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON document:" << filePath;
        return false;
    }

    bool success = fromJson(doc.object());
    if (success) {
        m_filePath = filePath;
    }
    return success;
}

int Project::importFromFile(const QString& filePath, QPointF offset) {
    if (!m_scene) return -1;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Import: Failed to open file:" << filePath;
        return -1;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Import: Invalid JSON in" << filePath;
        return -1;
    }
    QJsonObject root = doc.object();

    // Collect existing IDs in the current scene to detect collisions
    QSet<QString> existingIds;
    for (auto comp : m_scene->uiComponents()) {
        existingIds.insert(comp->componentId());
    }

    // Helper lambda: deduplicate an ID by appending _2, _3 ...
    auto uniqueId = [&](const QString& baseId) -> QString {
        if (!existingIds.contains(baseId)) {
            existingIds.insert(baseId);
            return baseId;
        }
        int suffix = 2;
        while (true) {
            QString candidate = baseId + "_" + QString::number(suffix);
            if (!existingIds.contains(candidate)) {
                existingIds.insert(candidate);
                return candidate;
            }
            ++suffix;
        }
    };

    // Import color styles that don't already exist by name
    if (root.contains("colorStyles") && root.value("colorStyles").isArray()) {
        for (const auto& v : root.value("colorStyles").toArray()) {
            ColorStyle cs = ColorStyle::fromJson(v.toObject());
            if (!hasColorStyle(cs.name)) {
                addColorStyle(cs);
            }
        }
    }

    // Import custom component definitions that don't already exist by id
    if (root.contains("customComponents") && root.value("customComponents").isArray()) {
        for (const auto& v : root.value("customComponents").toArray()) {
            CustomComponentDefinition def = CustomComponentDefinition::fromJson(v.toObject());
            if (findCustomComponentDefinition(def.id).id.isEmpty()) {
                addCustomComponentDefinition(def);
            }
        }
    }

    // Import components from the first (and only) page
    int imported = 0;
    if (root.contains("pages") && root.value("pages").isArray()) {
        QJsonArray pages = root.value("pages").toArray();
        if (!pages.isEmpty()) {
            QJsonObject mainPage = pages.first().toObject();
            if (mainPage.contains("components") && mainPage.value("components").isArray()) {
                QJsonArray compsArray = mainPage.value("components").toArray();
                for (int i = 0; i < compsArray.size(); ++i) {
                    QJsonObject compObj = compsArray[i].toObject();
                    QString type = compObj.value("type").toString();
                    QString originalId = compObj.value("id").toString();

                    // Deduplicate ID
                    QString newId = uniqueId(originalId.isEmpty() ? (type.toLower() + "_imp") : originalId);
                    compObj["id"] = newId;

                    // Offset position so imported items don't stack exactly on existing ones
                    qreal x = compObj.value("x").toDouble() + offset.x();
                    qreal y = compObj.value("y").toDouble() + offset.y();
                    compObj["x"] = x;
                    compObj["y"] = y;

                    UIComponent* comp = createComponentInstance(type, newId);
                    if (comp) {
                        if (auto cci = dynamic_cast<CustomComponentInstance*>(comp)) {
                            cci->setProject(this);
                        }
                        comp->fromJson(compObj);
                        // Ensure position matches offset-adjusted values
                        comp->setCompPos(x, y);
                        m_scene->addUIComponent(comp);
                        ++imported;
                    }
                }
            }
        }
    }

    // Resolve any imported style refs
    for (auto comp : m_scene->uiComponents()) {
        const auto& refs = comp->colorStyleRefs();
        for (auto it = refs.begin(); it != refs.end(); ++it) {
            QString styleName = it.value();
            if (hasColorStyle(styleName)) {
                comp->applyColorStyle(styleName, resolveColor(styleName));
            }
        }
    }

    if (imported > 0) {
        m_dirty = true;
        emit projectModified();
    }
    return imported;
}

UIComponent* Project::createComponentInstance(const QString& type, const QString& id) {
    if (type == "Button") {
        return new ButtonComponent(id);
    } else if (type == "Text" || type == "Label") {
        return new LabelComponent(id);
    } else if (type == "Rectangle") {
        return new RectangleComponent(id);
    } else if (type == "ProgressBar") {
        return new ProgressBarComponent(id);
    } else if (type == "Image") {
        return new ImageComponent(id);
    } else if (type == "Slider") {
        return new SliderComponent(id);
    } else if (type == "Switch") {
        return new SwitchComponent(id);
    } else if (type == "Checkbox") {
        return new CheckboxComponent(id);
    } else if (type == "TextInput") {
        return new TextInputComponent(id);
    } else if (type == "Circle") {
        return new CircleComponent(id);
    } else if (type == "CustomComponent") {
        return new CustomComponentInstance(id);
    }
    return nullptr;
}

void Project::initDefaultStyles() {
    m_colorStyles = {
        { "Primary", QColor("#2196F3") },
        { "Background", QColor("#1E2026") },
        { "Accent", QColor("#FF5722") },
        { "Text", QColor("#FFFFFF") }
    };
}

void Project::addColorStyle(const ColorStyle& style) {
    for (int i = 0; i < m_colorStyles.size(); ++i) {
        if (m_colorStyles[i].name == style.name) {
            updateColorStyle(style.name, style.color);
            return;
        }
    }
    m_colorStyles.append(style);
    setDirty(true);
    emit colorStylesChanged();
}

void Project::updateColorStyle(const QString& name, const QColor& newColor) {
    bool found = false;
    for (auto& s : m_colorStyles) {
        if (s.name == name) {
            s.color = newColor;
            found = true;
            break;
        }
    }
    if (!found) {
        m_colorStyles.append({name, newColor});
    }
    setDirty(true);
    emit colorStylesChanged();

    // Immediately update every component on the canvas that references this style
    if (m_scene) {
        for (auto comp : m_scene->uiComponents()) {
            comp->applyColorStyle(name, newColor);
        }
    }
}

void Project::removeColorStyle(const QString& name) {
    for (int i = 0; i < m_colorStyles.size(); ++i) {
        if (m_colorStyles[i].name == name) {
            m_colorStyles.removeAt(i);
            setDirty(true);
            emit colorStylesChanged();
            break;
        }
    }
}

QColor Project::resolveColor(const QString& styleName, const QColor& defaultColor) const {
    for (const auto& s : m_colorStyles) {
        if (s.name == styleName) {
            return s.color;
        }
    }
    return defaultColor;
}

bool Project::hasColorStyle(const QString& name) const {
    for (const auto& s : m_colorStyles) {
        if (s.name == name) return true;
    }
    return false;
}

void Project::setColorStyles(const QList<ColorStyle>& styles) {
    m_colorStyles = styles;
    setDirty(true);
    emit colorStylesChanged();
}

void Project::addCustomComponentDefinition(const CustomComponentDefinition& def) {
    for (int i = 0; i < m_customComponentDefinitions.size(); ++i) {
        if (m_customComponentDefinitions[i].id == def.id) {
            m_customComponentDefinitions[i] = def;
            setDirty(true);
            emit customComponentsChanged();
            return;
        }
    }
    m_customComponentDefinitions.append(def);
    setDirty(true);
    emit customComponentsChanged();
}

CustomComponentDefinition Project::findCustomComponentDefinition(const QString& id) const {
    for (const auto& def : m_customComponentDefinitions) {
        if (def.id == id) {
            return def;
        }
    }
    return CustomComponentDefinition();
}

void Project::removeCustomComponentDefinition(const QString& id) {
    for (int i = 0; i < m_customComponentDefinitions.size(); ++i) {
        if (m_customComponentDefinitions[i].id == id) {
            m_customComponentDefinitions.removeAt(i);
            setDirty(true);
            emit customComponentsChanged();
            break;
        }
    }
}
