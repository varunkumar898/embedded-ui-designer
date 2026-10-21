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
#include "GaugeComponent.h"
#include "SpeedometerComponent.h"
#include "BatteryComponent.h"
#include "PressureComponent.h"
#include "RpmComponent.h"
#include "TemperatureComponent.h"
#include "CircularProgressComponent.h"
#include "TabViewComponent.h"
#include "NavigationBarComponent.h"
#include "ListComponent.h"
#include "TableComponent.h"
#include "GroupComponent.h"
#include "codegen/FirmwareExportManager.h"
#include "hardware/DeviceDatabase.h"
#include "hardware/BoardRegistry.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSet>
#include <QDebug>

Project::Project(CanvasScene* scene, QObject* parent)
    : QObject(parent)
    , m_scene(scene)
    , m_name("MyEmbeddedApp")
    , m_targetFramework("qt-for-mcus")
    , m_dirty(false)
{
    m_displayConfig = DisplayConfig();
    initDefaultStyles();
    connectSceneSignals();

    Screen* mainScreen = new Screen("screen_main", "Main Screen", this);
    mainScreen->setWidth(m_displayConfig.width);
    mainScreen->setHeight(m_displayConfig.height);
    mainScreen->setBackgroundColor(Qt::white);
    connect(mainScreen, &Screen::screenChanged, this, [this]() { setDirty(true); });

    m_screens.append(mainScreen);
    m_activeScreen = mainScreen;

    if (m_scene) {
        for (UIComponent* comp : m_scene->uiComponents()) {
            if (comp && !mainScreen->hasComponent(comp)) {
                comp->setScreenId(mainScreen->id());
                mainScreen->addComponent(comp);
            }
        }
    }
}

Project::~Project() {
    blockSignals(true);
    clearScreens();
}


void Project::connectSceneSignals() {
    if (m_scene) {
        connect(m_scene, &CanvasScene::componentChanged, this, [this](UIComponent*) {
            setDirty(true);
        });
        connect(m_scene, &CanvasScene::componentAdded, this, [this](UIComponent* comp) {
            if (m_activeScreen && comp && !m_activeScreen->hasComponent(comp)) {
                comp->setScreenId(m_activeScreen->id());
                m_activeScreen->addComponent(comp);
            }
            setDirty(true);
        });
        connect(m_scene, &CanvasScene::componentRemoved, this, [this](UIComponent* comp) {
            if (m_activeScreen && comp && m_activeScreen->hasComponent(comp)) {
                m_activeScreen->removeComponent(comp);
            }
            setDirty(true);
        });
    }
}

void Project::clearScreens() {
    if (m_scene) {
        m_scene->detachAllComponents();
    }
    m_activeScreen = nullptr;
    qDeleteAll(m_screens);
    m_screens.clear();
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
    if (m_activeScreen) {
        m_activeScreen->setWidth(config.width);
        m_activeScreen->setHeight(config.height);
    }
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

    clearScreens();

    Screen* mainScreen = new Screen("screen_main", "Main Screen", this);
    mainScreen->setWidth(width);
    mainScreen->setHeight(height);
    mainScreen->setBackgroundColor(Qt::white);
    connect(mainScreen, &Screen::screenChanged, this, [this]() { setDirty(true); });

    m_screens.append(mainScreen);
    m_activeScreen = mainScreen;

    if (m_scene) {
        m_scene->clearComponents();
        m_scene->setDisplayConfig(cfg);
        m_scene->setScreenBackgroundColor(Qt::white);
        m_scene->clearGuides();
    }

    m_hardwareConfig.clear();
    emit hardwareConfigChanged();

    // Task 5: back to the defaults (full framebuffer, single buffer) for a new project.
    m_boardDisplay = BoardDisplayConfig();
    emit boardDisplayConfigChanged();

    m_dataSources.clear();
    emit dataSourcesChanged();

    m_dataBindings.clear();
    emit dataBindingsChanged();

    initDefaultStyles();
    emit colorStylesChanged();

    m_dirty = false;
    emit screenListChanged();
    emit activeScreenChanged(m_activeScreen);
    emit projectLoaded();
}

void Project::setHardwareConfig(const Hardware::HardwareConfig& config) {
    m_hardwareConfig = config;
    setDirty(true);
    emit hardwareConfigChanged();
    emit projectModified();
}

void Project::setBoardDisplayConfig(const BoardDisplayConfig& config) {
    if (m_boardDisplay == config) {
        return;
    }
    m_boardDisplay = config;
    setDirty(true);
    emit boardDisplayConfigChanged();
    // Unconditional: the Resource Analyzer relies on projectModified to refresh
    // even when the dirty flag was already set (mirrors setHardwareConfig).
    emit projectModified();
}

DeviceProfile Project::getDeviceProfile() const {
    // 1. Identify device or board identifier
    QString devId = m_hardwareConfig.deviceId.trimmed();
    QString boardId = m_hardwareConfig.boardId.trimmed();
    if (devId.isEmpty() && boardId.isEmpty()) {
        return DeviceProfile{};
    }

    CapabilityDatabase db;
    db.loadBuiltinDatabase();

    // 1. Try resolving deviceId directly
    if (!devId.isEmpty()) {
        QString devKey = devId.toLower();
        for (const QString& dName : db.listAllDevices()) {
            if (dName.trimmed().toLower() == devKey) {
                return db.getDeviceProfile(dName);
            }
        }
        auto devDef = Hardware::DeviceDatabase::instance().findDevice(devId);
        if (!devDef.partNumber.isEmpty()) {
            DeviceProfile prof;
            prof.name = devDef.partNumber;
            prof.family = devDef.family;
            prof.flash_kb = static_cast<uint32_t>(devDef.flashBytes / 1024);
            prof.ram_kb = static_cast<uint32_t>(devDef.ramBytes / 1024);
            prof.cpu_speed_mhz = static_cast<uint32_t>(devDef.maxClockMhz);
            prof.supported_frameworks = {"qt-for-mcus", "ugfx", "lvgl"};
            return prof;
        }
    }

    // 2. Try resolving boardId
    if (!boardId.isEmpty()) {
        BoardDef board = BoardRegistry::instance().board(boardId);
        if (board.isValid()) {
            QString mcuName = board.mcu.trimmed();
            for (const QString& dName : db.listAllDevices()) {
                if (dName.trimmed().compare(mcuName, Qt::CaseInsensitive) == 0 ||
                    (!dName.isEmpty() && mcuName.startsWith(dName, Qt::CaseInsensitive)) ||
                    dName.trimmed().compare(board.id, Qt::CaseInsensitive) == 0) {
                    return db.getDeviceProfile(dName);
                }
            }
            for (const QString& dName : db.listAllDevices()) {
                DeviceProfile p = db.getDeviceProfile(dName);
                if (!p.module_variant.isEmpty() && p.module_variant.compare(board.id, Qt::CaseInsensitive) == 0) {
                    return p;
                }
            }
            auto bDev = Hardware::DeviceDatabase::instance().findDevice(mcuName);
            if (!bDev.partNumber.isEmpty()) {
                DeviceProfile prof;
                prof.name = bDev.partNumber;
                prof.family = bDev.family;
                prof.flash_kb = static_cast<uint32_t>(bDev.flashBytes / 1024);
                prof.ram_kb = static_cast<uint32_t>(bDev.ramBytes / 1024);
                prof.cpu_speed_mhz = static_cast<uint32_t>(bDev.maxClockMhz);
                prof.supported_frameworks = {"qt-for-mcus", "ugfx", "lvgl"};
                return prof;
            }
            if (board.flashBytes > 0 || board.ramBytes > 0) {
                DeviceProfile prof;
                prof.name = !board.mcu.isEmpty() ? board.mcu : board.id;
                prof.family = board.family;
                prof.flash_kb = static_cast<uint32_t>(board.flashBytes / 1024);
                prof.ram_kb = static_cast<uint32_t>(board.ramBytes / 1024);
                prof.cpu_speed_mhz = static_cast<uint32_t>(board.clockMhz);
                prof.supported_frameworks = {"qt-for-mcus", "ugfx", "lvgl"};
                return prof;
            }
        }
    }

    // Fallback: return empty/invalid profile, NOT a made-up default MCU
    return DeviceProfile{};
}

// ── Multi-Screen Operations ──────────────────────────────────────────────

void Project::setActiveScreen(Screen* screen) {
    if (!screen || m_activeScreen == screen || !m_screens.contains(screen)) return;

    // Save current canvas state to outgoing screen
    if (m_activeScreen && m_scene) {
        m_activeScreen->setGuides(m_scene->hGuides(), m_scene->vGuides());
        m_activeScreen->setBackgroundColor(m_scene->screenBackgroundColor());
    }

    // Detach all items from canvas without deleting them
    if (m_scene) {
        m_scene->detachAllComponents();
    }

    m_activeScreen = screen;
    syncSceneToActiveScreen();

    emit activeScreenChanged(m_activeScreen);
    setDirty(true);
}

void Project::setActiveScreenById(const QString& screenId) {
    Screen* s = findScreen(screenId);
    if (s) {
        setActiveScreen(s);
    }
}

void Project::syncSceneToActiveScreen() {
    if (!m_scene || !m_activeScreen) return;

    DisplayConfig cfg = m_displayConfig;
    if (m_activeScreen->width() > 0) cfg.width = m_activeScreen->width();
    if (m_activeScreen->height() > 0) cfg.height = m_activeScreen->height();

    m_scene->setDisplayConfig(cfg);
    m_scene->setScreenBackgroundColor(m_activeScreen->backgroundColor());
    m_scene->setGuides(m_activeScreen->hGuides(), m_activeScreen->vGuides());

    // Re-attach active screen's components to scene
    for (UIComponent* comp : m_activeScreen->components()) {
        if (comp) {
            m_scene->addUIComponent(comp);
        }
    }

    // Live propagation of styles to active components
    for (UIComponent* comp : m_scene->uiComponents()) {
        const auto& refs = comp->colorStyleRefs();
        for (auto it = refs.begin(); it != refs.end(); ++it) {
            const QString styleName = it.value();
            if (hasColorStyle(styleName)) {
                comp->applyColorStyle(styleName, resolveColor(styleName));
            }
        }
    }
}

Screen* Project::addScreen(const QString& name, int width, int height) {
    int nextIdx = m_screens.size() + 1;
    QString id = QString("screen_%1").arg(nextIdx);
    while (findScreen(id) != nullptr) {
        ++nextIdx;
        id = QString("screen_%1").arg(nextIdx);
    }

    QString screenName = name.isEmpty() ? QString("Screen %1").arg(nextIdx) : name;
    int w = width > 0 ? width : m_displayConfig.width;
    int h = height > 0 ? height : m_displayConfig.height;

    Screen* screen = new Screen(id, screenName, this);
    screen->setWidth(w);
    screen->setHeight(h);
    screen->setBackgroundColor(m_scene ? m_scene->screenBackgroundColor() : Qt::white);
    connect(screen, &Screen::screenChanged, this, [this]() { setDirty(true); });

    m_screens.append(screen);
    setDirty(true);

    emit screenAdded(screen);
    emit screenListChanged();
    return screen;
}

void Project::addScreen(Screen* screen) {
    if (!screen || m_screens.contains(screen)) return;
    screen->setParent(this);
    connect(screen, &Screen::screenChanged, this, [this]() { setDirty(true); });
    m_screens.append(screen);
    setDirty(true);

    emit screenAdded(screen);
    emit screenListChanged();
}

Screen* Project::takeScreen(const QString& screenId) {
    if (m_screens.size() <= 1) {
        return nullptr;
    }

    int idx = -1;
    for (int i = 0; i < m_screens.size(); ++i) {
        if (m_screens[i]->id() == screenId) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return nullptr;

    Screen* screen = m_screens.at(idx);

    if (m_activeScreen == screen) {
        int nextActiveIdx = (idx > 0) ? (idx - 1) : 1;
        setActiveScreen(m_screens.at(nextActiveIdx));
    }

    m_screens.removeAt(idx);
    screen->setParent(nullptr);

    setDirty(true);
    emit screenRemoved(screenId);
    emit screenListChanged();
    return screen;
}

bool Project::removeScreen(const QString& screenId) {
    Screen* s = takeScreen(screenId);
    if (!s) return false;
    delete s;
    return true;
}

bool Project::renameScreen(const QString& screenId, const QString& newName) {
    Screen* s = findScreen(screenId);
    if (!s) return false;
    s->setName(newName);
    setDirty(true);
    emit screenRenamed(screenId, newName);
    emit screenListChanged();
    return true;
}

Screen* Project::duplicateScreen(const QString& screenId) {
    Screen* src = findScreen(screenId);
    if (!src) return nullptr;

    int nextIdx = m_screens.size() + 1;
    QString newId = QString("screen_%1").arg(nextIdx);
    while (findScreen(newId) != nullptr) {
        ++nextIdx;
        newId = QString("screen_%1").arg(nextIdx);
    }

    QString newName = src->name() + " (Copy)";
    Screen* clone = src->clone(newId, newName, this);
    connect(clone, &Screen::screenChanged, this, [this]() { setDirty(true); });

    m_screens.append(clone);
    setDirty(true);

    emit screenAdded(clone);
    emit screenListChanged();
    return clone;
}

bool Project::moveScreen(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= m_screens.size() ||
        toIndex < 0 || toIndex >= m_screens.size() ||
        fromIndex == toIndex) {
        return false;
    }

    Screen* s = m_screens.takeAt(fromIndex);
    m_screens.insert(toIndex, s);
    setDirty(true);
    emit screenListChanged();
    return true;
}

Screen* Project::findScreen(const QString& screenId) const {
    for (Screen* s : m_screens) {
        if (s && s->id() == screenId) return s;
    }
    return nullptr;
}

int Project::activeScreenIndex() const {
    return m_screens.indexOf(m_activeScreen);
}

// ── Project-level Data Sources ───────────────────────────────────────────

void Project::addDataSource(const DataSource& source) {
    for (int i = 0; i < m_dataSources.size(); ++i) {
        if (m_dataSources[i].id() == source.id()) {
            m_dataSources[i] = source;
            setDirty(true);
            emit dataSourcesChanged();
            return;
        }
    }
    m_dataSources.append(source);
    setDirty(true);
    emit dataSourcesChanged();
}

void Project::removeDataSource(const QString& sourceId) {
    for (int i = 0; i < m_dataSources.size(); ++i) {
        if (m_dataSources[i].id() == sourceId) {
            m_dataSources.removeAt(i);
            setDirty(true);
            emit dataSourcesChanged();
            return;
        }
    }
}

const DataSource* Project::findDataSource(const QString& sourceId) const {
    for (const auto& ds : m_dataSources) {
        if (ds.id() == sourceId) return &ds;
    }
    return nullptr;
}

bool Project::updateDataSourceValue(const QString& sourceId, const QVariant& value) {
    for (int i = 0; i < m_dataSources.size(); ++i) {
        if (m_dataSources[i].id() == sourceId) {
            m_dataSources[i].setValue(value);
            emit dataSourceValueChanged(sourceId, value);
            return true;
        }
    }
    return false;
}

void Project::setDataSources(const QList<DataSource>& sources) {
    m_dataSources = sources;
    setDirty(true);
    emit dataSourcesChanged();
}

// ── Project-level Data Bindings ──────────────────────────────────────────

void Project::addDataBinding(const DataBinding& binding) {
    for (int i = 0; i < m_dataBindings.size(); ++i) {
        if (m_dataBindings[i].componentId() == binding.componentId() &&
            m_dataBindings[i].propertyName() == binding.propertyName()) {
            m_dataBindings[i] = binding;
            setDirty(true);
            emit dataBindingsChanged();
            return;
        }
    }
    m_dataBindings.append(binding);
    setDirty(true);
    emit dataBindingsChanged();
}

void Project::removeDataBinding(const QString& componentId, const QString& propertyName) {
    for (int i = 0; i < m_dataBindings.size(); ++i) {
        if (m_dataBindings[i].componentId() == componentId &&
            (propertyName.isEmpty() || m_dataBindings[i].propertyName() == propertyName)) {
            m_dataBindings.removeAt(i);
            setDirty(true);
            emit dataBindingsChanged();
            return;
        }
    }
}

void Project::setDataBindings(const QList<DataBinding>& bindings) {
    m_dataBindings = bindings;
    setDirty(true);
    emit dataBindingsChanged();
}

// ── Serialization & Deserialization ──────────────────────────────────────

QJsonObject Project::toJson() const {
    // Save current active screen guides & background color before serializing
    if (m_activeScreen && m_scene) {
        m_activeScreen->setGuides(m_scene->hGuides(), m_scene->vGuides());
        m_activeScreen->setBackgroundColor(m_scene->screenBackgroundColor());
    }

    QJsonObject root;
    root["version"] = "2.0";
    root["projectVersion"] = 2;
    root["name"] = m_name;
    root["displayName"] = displayName();
    root["target"] = m_targetFramework;
    root["display"] = m_displayConfig.toJson();
    root["activeScreenId"] = m_activeScreen ? m_activeScreen->id() : "";

    // ── Named color styles ────────────────────────────────────────────────
    QJsonArray stylesArr;
    for (const ColorStyle& s : m_colorStyles)
        stylesArr.append(s.toJson());
    root["colorStyles"] = stylesArr;

    // ── Screens (Multi-screen Phase 1A) ──────────────────────────────────
    QJsonArray screensArr;
    for (Screen* s : m_screens) {
        if (s) {
            screensArr.append(s->toJson());
        }
    }
    root["screens"] = screensArr;

    // Backward compatibility mirror for v1 loaders
    root["pages"] = screensArr;

    // ── Project-level Data Sources ────────────────────────────────────────
    if (!m_dataSources.isEmpty()) {
        QJsonArray dsArr;
        for (const auto& ds : m_dataSources) {
            dsArr.append(ds.toJson());
        }
        root["dataSources"] = dsArr;
    }

    // ── Project-level Data Bindings ───────────────────────────────────────
    if (!m_dataBindings.isEmpty()) {
        QJsonArray dbArr;
        for (const auto& db : m_dataBindings) {
            dbArr.append(db.toJson());
        }
        root["bindings"] = dbArr;
    }

    QJsonArray assets;
    root["assets"] = assets;

    root["uiProjectDirectory"] = uiProjectDirectory();
    root["firmwareDirectory"] = m_firmwareDir;
    root["buildDirectory"] = m_buildDir;

    // Custom component library
    QJsonArray libArr;
    for (const ComponentDefinition& def : m_componentLibrary)
        libArr.append(def.toJson());
    root["componentLibrary"] = libArr;

    root["hardware"] = m_hardwareConfig.toJson();
    // Task 5: additive optional block; older builds simply ignore it.
    root["boardDisplay"] = m_boardDisplay.toJson();

    return root;
}

bool Project::fromJson(const QJsonObject& root) {
    m_name = root.value("name").toString("MyEmbeddedApp");
    m_displayName = root.value("displayName").toString(m_name);
    m_targetFramework = root.value("target").toString("qt-for-mcus");

    if (root.contains("hardware") && root.value("hardware").isObject()) {
        m_hardwareConfig = Hardware::HardwareConfig::fromJson(root.value("hardware").toObject());
    } else {
        m_hardwareConfig.clear();
    }
    emit hardwareConfigChanged();

    if (root.contains("display") && root.value("display").isObject()) {
        m_displayConfig = DisplayConfig::fromJson(root.value("display").toObject());
    }

    // Task 5: optional "boardDisplay" block; absence means "use the defaults"
    // (full framebuffer, single buffer) so old .euiproj files load unchanged.
    if (root.contains("boardDisplay") && root.value("boardDisplay").isObject()) {
        m_boardDisplay = BoardDisplayConfig::fromJson(root.value("boardDisplay").toObject());
    } else {
        m_boardDisplay = BoardDisplayConfig();
    }
    emit boardDisplayConfigChanged();

    // ── Named color styles ────────────────────────────────────────────────
    m_colorStyles.clear();
    if (root.contains("colorStyles") && root.value("colorStyles").isArray()) {
        for (const QJsonValue& v : root.value("colorStyles").toArray())
            m_colorStyles.append(ColorStyle::fromJson(v.toObject()));
    } else {
        initDefaultStyles();
    }
    emit colorStylesChanged();

    // ── Project-level Data Sources ────────────────────────────────────────
    m_dataSources.clear();
    if (root.contains("dataSources") && root.value("dataSources").isArray()) {
        for (const QJsonValue& v : root.value("dataSources").toArray()) {
            m_dataSources.append(DataSource::fromJson(v.toObject()));
        }
    }
    emit dataSourcesChanged();

    // ── Project-level Data Bindings ───────────────────────────────────────
    m_dataBindings.clear();
    if (root.contains("bindings") && root.value("bindings").isArray()) {
        for (const QJsonValue& v : root.value("bindings").toArray()) {
            m_dataBindings.append(DataBinding::fromJson(v.toObject()));
        }
    }
    emit dataBindingsChanged();

    // ── Multi-Screen Loading (Supports both "screens" v2 and "pages" v1) ──
    clearScreens();

    QJsonArray screensArr;
    if (root.contains("screens") && root.value("screens").isArray()) {
        screensArr = root.value("screens").toArray();
    } else if (root.contains("pages") && root.value("pages").isArray()) {
        screensArr = root.value("pages").toArray();
    }

    if (!screensArr.isEmpty()) {
        for (const QJsonValue& v : screensArr) {
            Screen* s = Screen::fromJson(v.toObject(), this);
            if (s) {
                if (s->width() <= 0) s->setWidth(m_displayConfig.width);
                if (s->height() <= 0) s->setHeight(m_displayConfig.height);
                connect(s, &Screen::screenChanged, this, [this]() { setDirty(true); });
                m_screens.append(s);
            }
        }
    }

    // Fallback if no screens were found
    if (m_screens.isEmpty()) {
        Screen* mainScreen = new Screen("screen_main", "Main Screen", this);
        mainScreen->setWidth(m_displayConfig.width);
        mainScreen->setHeight(m_displayConfig.height);
        mainScreen->setBackgroundColor(Qt::white);
        connect(mainScreen, &Screen::screenChanged, this, [this]() { setDirty(true); });
        m_screens.append(mainScreen);
    }

    // Restore active screen
    QString activeId = root.value("activeScreenId").toString();
    Screen* targetActive = findScreen(activeId);
    if (!targetActive) {
        targetActive = m_screens.first();
    }
    m_activeScreen = targetActive;

    // Synchronize canvas scene
    syncSceneToActiveScreen();

    m_dirty = false;
    emit screenListChanged();
    emit activeScreenChanged(m_activeScreen);
    emit projectLoaded();

    // Restore component library
    m_componentLibrary.clear();
    for (const QJsonValue& v : root.value("componentLibrary").toArray())
        m_componentLibrary.append(ComponentDefinition::fromJson(v.toObject()));

    if (root.contains("firmwareDirectory")) {
        m_firmwareDir = root.value("firmwareDirectory").toString();
    }
    if (root.contains("buildDirectory")) {
        m_buildDir = root.value("buildDirectory").toString();
    }
    if (root.contains("uiProjectDirectory")) {
        m_uiProjectDir = root.value("uiProjectDirectory").toString();
    }

    return true;
}

QString Project::uiProjectDirectory() const {
    if (!m_uiProjectDir.isEmpty() && QDir(m_uiProjectDir).exists()) {
        return m_uiProjectDir;
    }
    if (!m_filePath.isEmpty() && !m_filePath.startsWith(":/")) {
        return QFileInfo(m_filePath).absolutePath();
    }
    return QDir::currentPath();
}

void Project::setUiProjectDirectory(const QString& dir) {
    if (m_uiProjectDir != dir) {
        m_uiProjectDir = dir;
        setDirty(true);
    }
}

QString Project::firmwareDirectory() const {
    if (!m_firmwareDir.isEmpty() && QDir(m_firmwareDir).exists()) {
        return m_firmwareDir;
    }

    // Auto-detection for existing bare-metal firmware projects (project-relative):
    // Nested firmware directory inside the UI project
    QString subFw = QDir(uiProjectDirectory()).filePath("firmware");
    if (QDir(subFw).exists()) {
        return subFw;
    }

    // 3. If UI project directory itself contains firmware sources or Makefile
    QString uiDir = uiProjectDirectory();
    if (QFile::exists(QDir(uiDir).filePath("Makefile")) ||
        QFile::exists(QDir(uiDir).filePath("CMakeLists.txt")) ||
        QDir(QDir(uiDir).filePath("inc")).exists() ||
        QDir(QDir(uiDir).filePath("src")).exists() ||
        QDir(QDir(uiDir).filePath("scr")).exists()) {
        return uiDir;
    }

    return uiDir;
}

void Project::setFirmwareDirectory(const QString& dir) {
    if (m_firmwareDir != dir) {
        m_firmwareDir = dir;
        m_projectDir = dir;
        setDirty(true);
        emit firmwareDirectoryChanged(m_firmwareDir);
        emit projectDirectoryChanged(m_firmwareDir);
    }
}

QString Project::buildDirectory() const {
    if (!m_buildDir.isEmpty() && QDir(m_buildDir).exists()) {
        return m_buildDir;
    }
    QString defaultBuild = QDir(firmwareDirectory()).filePath("build");
    if (QDir(defaultBuild).exists()) {
        return defaultBuild;
    }
    return firmwareDirectory();
}

void Project::setBuildDirectory(const QString& dir) {
    if (m_buildDir != dir) {
        m_buildDir = dir;
        setDirty(true);
    }
}

QString Project::projectDirectory() const {
    return firmwareDirectory();
}

void Project::setProjectDirectory(const QString& dir) {
    setFirmwareDirectory(dir);
}

bool Project::syncFirmwareFiles(QString* errorMsg) {
    FirmwareExportManager mgr;
    ExportOptions opts;
    opts.outputDirectory = firmwareDirectory();
    opts.targetFramework = m_targetFramework.isEmpty() ? "ugfx" : m_targetFramework;
    opts.targetMcu = m_hardwareConfig.deviceId.isEmpty() ? "STM32F030R8" : m_hardwareConfig.deviceId;
    opts.generateCmake = true;
    opts.overwriteUserCode = false; // Always preserve user code!
    opts.optimizeAssets = true;

    ExportResult res = mgr.exportProject(this, opts);
    if (!res.success) {
        if (errorMsg) *errorMsg = res.errorMessage;
        return false;
    }
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
    m_projectDir = QFileInfo(filePath).absolutePath();
    m_dirty = false;
    emit projectSaved(filePath);
    emit projectDirectoryChanged(m_projectDir);
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
        if (!filePath.startsWith(":/")) {
            m_projectDir = QFileInfo(filePath).absolutePath();
            emit projectDirectoryChanged(m_projectDir);
        }
    }
    return success;
}

int Project::importFromFile(const QString& filePath, QPointF offset) {
    if (!m_scene || !m_activeScreen) return -1;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return -1;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) return -1;

    QJsonArray pages = document.object().value("screens").toArray();
    if (pages.isEmpty()) {
        pages = document.object().value("pages").toArray();
    }
    if (pages.isEmpty()) return 0;

    const QJsonArray components = pages.first().toObject().value("components").toArray();
    QSet<QString> usedIds;
    for (UIComponent* component : m_activeScreen->components()) usedIds.insert(component->componentId());

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
        component->setScreenId(m_activeScreen->id());
        m_activeScreen->addComponent(component);
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
    } else if (type == "ProgressBar" || type == "LinearProgress") {
        return new ProgressBarComponent(id);
    } else if (type == "CircularProgress") {
        return new CircularProgressComponent(id);
    } else if (type == "Gauge") {
        return new GaugeComponent(id);
    } else if (type == "Speedometer") {
        return new SpeedometerComponent(id);
    } else if (type == "Battery") {
        return new BatteryComponent(id);
    } else if (type == "Pressure") {
        return new PressureComponent(id);
    } else if (type == "RPM") {
        return new RpmComponent(id);
    } else if (type == "Temperature") {
        return new TemperatureComponent(id);
    } else if (type == "TabView") {
        return new TabViewComponent(id);
    } else if (type == "NavigationBar") {
        return new NavigationBarComponent(id);
    } else if (type == "List") {
        return new ListComponent(id);
    } else if (type == "Table") {
        return new TableComponent(id);
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
    } else if (type == "Group") {
        return new GroupComponent(id);
    }
    return nullptr;
}

// ── ComponentDefinition library ──────────────────────────────────────────

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

// ── Named Color Styles ───────────────────────────────────────────────────

void Project::initDefaultStyles() {
    m_colorStyles = {
        { "Primary",    QColor("#2196F3") },
        { "Background", QColor("#1E2026") },
        { "Accent",     QColor("#FF5722") },
        { "Text",       QColor("#FFFFFF") }
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

    // Live propagation to active scene components
    if (m_scene) {
        for (UIComponent* comp : m_scene->uiComponents()) {
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
    for (const ColorStyle& s : m_colorStyles) {
        if (s.name == styleName) return s.color;
    }
    return defaultColor;
}

bool Project::hasColorStyle(const QString& name) const {
    for (const ColorStyle& s : m_colorStyles) {
        if (s.name == name) return true;
    }
    return false;
}

void Project::setColorStyles(const QList<ColorStyle>& styles) {
    m_colorStyles = styles;
    setDirty(true);
    emit colorStylesChanged();
}
