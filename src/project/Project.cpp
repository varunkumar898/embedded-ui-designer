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
#include "PathComponent.h"
#include "CustomComponentInstance.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSet>
#include <QDebug>

Project::Project(CanvasScene* scene, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
{
    m_displayConfig = DisplayConfig();

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

void Project::newProject(const QString& name, int width, int height) {
    m_name = name;
    m_filePath.clear();
    m_targetFramework = "qt-for-mcus";
    
    DisplayConfig cfg;
    cfg.width = width;
    cfg.height = height;
    cfg.colorDepth = 16;
    cfg.type = "LCD";
    cfg.dpi = 96;

    m_displayConfig = cfg;
    if (m_scene) {
        m_scene->clearComponents();
        m_scene->setDisplayConfig(cfg);
        m_scene->setScreenBackgroundColor(Qt::white);
    }

    m_dirty = false;
    emit projectLoaded();
}

QJsonObject Project::toJson() const {
    QJsonObject root;
    root["version"] = "1.0";
    root["name"] = m_name;
    root["target"] = m_targetFramework;
    root["display"] = m_displayConfig.toJson();

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

    // ── Ruler guides ──────────────────────────────────────────────────────
    if (m_scene) {
        QJsonArray hg, vg;
        for (qreal y : m_scene->hGuides()) hg.append(y);
        for (qreal x : m_scene->vGuides()) vg.append(x);
        mainPage["hGuides"] = hg;
        mainPage["vGuides"] = vg;
    }

    pages.append(mainPage);
    root["pages"] = pages;

    QJsonArray assets;
    root["assets"] = assets;

    // Custom component library
    QJsonArray libArr;
    for (const ComponentDefinition& def : m_componentLibrary)
        libArr.append(def.toJson());
    root["componentLibrary"] = libArr;

    return root;
}

bool Project::fromJson(const QJsonObject& root) {
    m_name = root.value("name").toString("MyEmbeddedApp");
    m_targetFramework = root.value("target").toString("qt-for-mcus");

    if (root.contains("display") && root.value("display").isObject()) {
        m_displayConfig = DisplayConfig::fromJson(root.value("display").toObject());
    }

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

                // Restore ruler guides
                QList<qreal> hg, vg;
                for (const QJsonValue& v : mainPage.value("hGuides").toArray()) hg.append(v.toDouble());
                for (const QJsonValue& v : mainPage.value("vGuides").toArray()) vg.append(v.toDouble());
                m_scene->setGuides(hg, vg);

                if (mainPage.contains("components") && mainPage.value("components").isArray()) {
                    QJsonArray compsArray = mainPage.value("components").toArray();
                    for (int i = 0; i < compsArray.size(); ++i) {
                        QJsonObject compObj = compsArray[i].toObject();
                        QString type = compObj.value("type").toString();
                        QString id = compObj.value("id").toString();

                        UIComponent* comp = createComponentInstance(type, id);
                        if (comp) {
                            comp->fromJson(compObj);
                            m_scene->addUIComponent(comp);
                        }
                    }
                }
            }
        }
    }

    m_dirty = false;
    emit projectLoaded();

    // Restore component library
    m_componentLibrary.clear();
    for (const QJsonValue& v : root.value("componentLibrary").toArray())
        m_componentLibrary.append(ComponentDefinition::fromJson(v.toObject()));

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
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) return -1;

    const QJsonArray pages = document.object().value("pages").toArray();
    if (pages.isEmpty()) return 0;
    const QJsonArray components = pages.first().toObject().value("components").toArray();
    QSet<QString> usedIds;
    for (UIComponent* component : m_scene->uiComponents()) usedIds.insert(component->componentId());

    int imported = 0;
    for (const QJsonValue& value : components) {
        const QJsonObject data = value.toObject();
        const QString type = data.value("type").toString();
        const QString originalId = data.value("id").toString(type.toLower());
        QString uniqueId = originalId;
        for (int suffix = 2; usedIds.contains(uniqueId); ++suffix) {
            uniqueId = originalId + "_" + QString::number(suffix);
        }
        UIComponent* component = createComponentInstance(type, uniqueId);
        if (!component) continue;
        component->fromJson(data);
        component->setComponentId(uniqueId);
        component->setCompPos(component->compX() + offset.x(), component->compY() + offset.y());
        m_scene->addUIComponent(component);
        usedIds.insert(uniqueId);
        ++imported;
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
    } else if (type == "Path") {
        return new PathComponent(id);
    } else if (type == "CustomInstance") {
        return new CustomComponentInstance(id);
    }
    return nullptr;
}

// ── ComponentDefinition library ──────────────────────────────────────────────────────

void Project::addComponentDefinition(const ComponentDefinition& def) {
    // Replace if same ID already exists
    for (ComponentDefinition& existing : m_componentLibrary) {
        if (existing.definitionId() == def.definitionId()) {
            existing = def;
            setDirty(true);
            return;
        }
    }
    m_componentLibrary.append(def);
    setDirty(true);
}

void Project::removeComponentDefinition(const QString& id) {
    m_componentLibrary.removeIf([&id](const ComponentDefinition& d) {
        return d.definitionId() == id;
    });
    setDirty(true);
}

const ComponentDefinition* Project::findDefinition(const QString& id) const {
    for (const ComponentDefinition& d : m_componentLibrary) {
        if (d.definitionId() == id) return &d;
    }
    return nullptr;
}

void Project::updateVariantsForDefinition(const QString& id, const QList<ComponentVariant>& variants) {
    for (ComponentDefinition& d : m_componentLibrary) {
        if (d.definitionId() == id) {
            d.setVariants(variants);
            setDirty(true);
            return;
        }
    }
}
