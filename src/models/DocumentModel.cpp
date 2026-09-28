#include "DocumentModel.h"
#include "AddWidgetCommand.h"
#include "MoveWidgetCommand.h"
#include "UgfxExporter.h"
#include "QtMcuExporter.h"
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

DocumentModel::DocumentModel(QObject* parent)
    : QAbstractListModel(parent)
{
    connect(&m_undoStack, &QUndoStack::canUndoChanged, this, &DocumentModel::undoStackChanged);
    connect(&m_undoStack, &QUndoStack::canRedoChanged, this, &DocumentModel::undoStackChanged);
    connect(&m_undoStack, &QUndoStack::undoTextChanged, this, &DocumentModel::undoStackChanged);
    connect(&m_undoStack, &QUndoStack::redoTextChanged, this, &DocumentModel::undoStackChanged);
    connect(&m_undoStack, &QUndoStack::indexChanged, this, &DocumentModel::documentModified);

    // Initial default screen
    auto defaultScreen = new ScreenModel("MainScreen", "Main Screen", m_screenWidth, m_screenHeight, this);
    m_screens.append(defaultScreen);
    connectScreenSignals(defaultScreen);
    hookActiveScreenSignals();
}

DocumentModel::~DocumentModel() {
    clear();
}

int DocumentModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    ScreenModel* scr = activeScreen();
    return scr ? scr->rowCount(parent) : 0;
}

QVariant DocumentModel::data(const QModelIndex& index, int role) const {
    ScreenModel* scr = activeScreen();
    return scr ? scr->data(index, role) : QVariant();
}

bool DocumentModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    ScreenModel* scr = activeScreen();
    return scr ? scr->setData(index, value, role) : false;
}

QHash<int, QByteArray> DocumentModel::roleNames() const {
    ScreenModel* scr = activeScreen();
    if (scr) return scr->roleNames();

    QHash<int, QByteArray> roles;
    roles[IdRole] = "widgetId";
    roles[TypeRole] = "widgetType";
    roles[XRole] = "widgetX";
    roles[YRole] = "widgetY";
    roles[WidthRole] = "widgetWidth";
    roles[HeightRole] = "widgetHeight";
    roles[ColorRole] = "widgetColor";
    roles[TextRole] = "widgetText";
    roles[TargetScreenIdRole] = "widgetTargetScreenId";
    roles[ImagePathRole] = "widgetImagePath";
    roles[WidgetRole] = "widgetModel";
    return roles;
}

void DocumentModel::setProjectName(const QString& name) {
    if (m_projectName != name) {
        m_projectName = name;
        emit projectNameChanged(m_projectName);
        emit documentModified();
    }
}

void DocumentModel::setScreenWidth(int w) {
    if (m_screenWidth != w) {
        m_screenWidth = w;
        for (ScreenModel* s : m_screens) {
            if (s) s->setWidth(w);
        }
        emit screenDimensionsChanged();
        emit documentModified();
    }
}

void DocumentModel::setScreenHeight(int h) {
    if (m_screenHeight != h) {
        m_screenHeight = h;
        for (ScreenModel* s : m_screens) {
            if (s) s->setHeight(h);
        }
        emit screenDimensionsChanged();
        emit documentModified();
    }
}

void DocumentModel::setColorDepth(int depth) {
    if (m_colorDepth != depth) {
        m_colorDepth = depth;
        emit colorDepthChanged(m_colorDepth);
        emit documentModified();
    }
}

void DocumentModel::setTargetFramework(const QString& target) {
    if (m_targetFramework != target) {
        m_targetFramework = target;
        emit targetFrameworkChanged(m_targetFramework);
        emit documentModified();
    }
}

ScreenModel* DocumentModel::activeScreen() const {
    if (m_activeScreenIndex >= 0 && m_activeScreenIndex < m_screens.size()) {
        return m_screens.at(m_activeScreenIndex);
    }
    return nullptr;
}

void DocumentModel::setActiveScreenIndex(int index) {
    if (index >= 0 && index < m_screens.size() && index != m_activeScreenIndex) {
        beginResetModel();
        m_activeScreenIndex = index;
        m_selectedIndex = -1;
        endResetModel();

        hookActiveScreenSignals();

        emit activeScreenIndexChanged(m_activeScreenIndex);
        emit activeScreenChanged();
        emit countChanged(count());
        emit selectedIndexChanged(m_selectedIndex);
        emit selectedWidgetChanged();
        emit documentModified();
    }
}

QVariantList DocumentModel::screensList() const {
    QVariantList list;
    for (int i = 0; i < m_screens.size(); ++i) {
        ScreenModel* s = m_screens.at(i);
        if (s) {
            QVariantMap map;
            map["index"] = i;
            map["id"] = s->id();
            map["name"] = s->name();
            map["width"] = s->width();
            map["height"] = s->height();
            list.append(map);
        }
    }
    return list;
}

ScreenModel* DocumentModel::addScreen(const QString& name, const QString& id) {
    int nextIdx = m_screens.size() + 1;
    QString screenId = id.isEmpty() ? QString("Screen_%1").arg(nextIdx) : id;
    QString screenName = name.isEmpty() ? QString("Screen %1").arg(nextIdx) : name;

    auto screen = new ScreenModel(screenId, screenName, m_screenWidth, m_screenHeight, this);
    m_screens.append(screen);
    connectScreenSignals(screen);

    emit screenCountChanged(m_screens.size());
    emit screensChanged();
    emit documentModified();

    // Auto-switch to newly created screen
    setActiveScreenIndex(m_screens.size() - 1);
    return screen;
}

bool DocumentModel::removeScreen(int index) {
    if (index < 0 || index >= m_screens.size() || m_screens.size() <= 1) {
        return false; // Preserve at least one active screen
    }

    ScreenModel* screenToRemove = m_screens.at(index);
    if (m_activeScreenIndex == index) {
        int nextActive = (index > 0) ? index - 1 : 0;
        setActiveScreenIndex(nextActive);
    } else if (m_activeScreenIndex > index) {
        m_activeScreenIndex--;
        emit activeScreenIndexChanged(m_activeScreenIndex);
    }

    m_screens.removeAt(index);
    screenToRemove->deleteLater();

    emit screenCountChanged(m_screens.size());
    emit screensChanged();
    emit documentModified();
    return true;
}

ScreenModel* DocumentModel::getScreen(int index) const {
    if (index >= 0 && index < m_screens.size()) {
        return m_screens.at(index);
    }
    return nullptr;
}

ScreenModel* DocumentModel::getScreenById(const QString& id) const {
    for (ScreenModel* s : m_screens) {
        if (s && s->id() == id) {
            return s;
        }
    }
    return nullptr;
}

int DocumentModel::count() const {
    ScreenModel* scr = activeScreen();
    return scr ? scr->count() : 0;
}

void DocumentModel::setSelectedIndex(int idx) {
    if (m_selectedIndex != idx) {
        m_selectedIndex = idx;
        emit selectedIndexChanged(m_selectedIndex);
        emit selectedWidgetChanged();
    }
}

WidgetModel* DocumentModel::selectedWidget() const {
    ScreenModel* scr = activeScreen();
    if (scr && m_selectedIndex >= 0 && m_selectedIndex < scr->count()) {
        return scr->getWidget(m_selectedIndex);
    }
    return nullptr;
}

void DocumentModel::undo() {
    m_undoStack.undo();
}

void DocumentModel::redo() {
    m_undoStack.redo();
}

void DocumentModel::addWidgetWithUndo(const QString& type, qreal x, qreal y, qreal width, qreal height, const QColor& color, const QString& text, const QString& targetScreenId) {
    ScreenModel* scr = activeScreen();
    if (!scr) return;

    static int idCounter = 1;
    QString id = QString("%1_%2").arg(type.toLower()).arg(idCounter++);
    auto w = new WidgetModel(id, type, x, y, width, height, color);
    if (!text.isEmpty()) {
        w->setText(text);
    } else {
        w->setText(type);
    }
    if (!targetScreenId.isEmpty()) {
        w->setTargetScreenId(targetScreenId);
    }

    m_undoStack.push(new AddWidgetCommand(scr, w, scr->count()));
    setSelectedIndex(scr->count() - 1);
}

void DocumentModel::moveWidgetWithUndo(int index, qreal newX, qreal newY) {
    WidgetModel* w = getWidget(index);
    if (!w) return;
    if (w->x() == newX && w->y() == newY) return;

    m_undoStack.push(new MoveWidgetCommand(w, w->x(), w->y(), newX, newY));
}

void DocumentModel::removeWidgetWithUndo(int index) {
    ScreenModel* scr = activeScreen();
    if (!scr) return;
    WidgetModel* w = scr->getWidget(index);
    if (!w) return;

    class RemoveWidgetCommand : public QUndoCommand {
    public:
        RemoveWidgetCommand(ScreenModel* screen, int idx) : m_screen(screen), m_index(idx) {
            m_widget = m_screen->getWidget(idx);
            setText(QString("Delete %1").arg(m_widget ? m_widget->id() : "Widget"));
        }
        void redo() override {
            m_widget = m_screen->takeWidget(m_index);
        }
        void undo() override {
            m_screen->insertWidget(m_index, m_widget);
        }
    private:
        ScreenModel* m_screen;
        WidgetModel* m_widget;
        int m_index;
    };

    m_undoStack.push(new RemoveWidgetCommand(scr, index));
}

void DocumentModel::insertWidget(int index, WidgetModel* widget) {
    ScreenModel* scr = activeScreen();
    if (scr) {
        scr->insertWidget(index, widget);
    }
}

WidgetModel* DocumentModel::takeWidget(int index) {
    ScreenModel* scr = activeScreen();
    return scr ? scr->takeWidget(index) : nullptr;
}

void DocumentModel::clear() {
    beginResetModel();
    qDeleteAll(m_screens);
    m_screens.clear();
    m_activeScreenIndex = 0;
    m_selectedIndex = -1;
    endResetModel();

    m_undoStack.clear();
    emit screenCountChanged(0);
    emit screensChanged();
    emit countChanged(0);
    emit selectedIndexChanged(-1);
    emit selectedWidgetChanged();
}

WidgetModel* DocumentModel::getWidget(int index) const {
    ScreenModel* scr = activeScreen();
    return scr ? scr->getWidget(index) : nullptr;
}

QList<WidgetModel*> DocumentModel::widgets() const {
    ScreenModel* scr = activeScreen();
    return scr ? scr->widgets() : QList<WidgetModel*>();
}

void DocumentModel::connectScreenSignals(ScreenModel* screen) {
    if (!screen) return;
    connect(screen, &ScreenModel::screenModified, this, &DocumentModel::documentModified);
}

void DocumentModel::hookActiveScreenSignals() {
    ScreenModel* scr = activeScreen();
    if (!scr) return;

    connect(scr, &ScreenModel::dataChanged, this, [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles) {
        emit dataChanged(topLeft, bottomRight, roles);
    });

    connect(scr, &ScreenModel::rowsInserted, this, [this](const QModelIndex& parent, int first, int last) {
        emit rowsInserted(parent, first, last);
        emit countChanged(count());
    });

    connect(scr, &ScreenModel::rowsRemoved, this, [this](const QModelIndex& parent, int first, int last) {
        emit rowsRemoved(parent, first, last);
        emit countChanged(count());
    });

    connect(scr, &ScreenModel::modelReset, this, [this]() {
        emit modelReset();
        emit countChanged(count());
    });
}

QJsonObject DocumentModel::toJson() const {
    QJsonObject root;
    root["version"] = "1.0";
    root["name"] = m_projectName;
    root["target"] = m_targetFramework;

    QJsonObject disp;
    disp["width"] = m_screenWidth;
    disp["height"] = m_screenHeight;
    disp["colorDepth"] = m_colorDepth;
    disp["type"] = "LCD";
    disp["dpi"] = 96;
    root["display"] = disp;

    // Loop through screens first, then widgets
    QJsonArray pages;
    for (ScreenModel* screen : m_screens) {
        if (screen) {
            pages.append(screen->toJson());
        }
    }
    root["pages"] = pages;

    return root;
}

bool DocumentModel::fromJson(const QJsonObject& root) {
    clear();

    m_projectName = root.value("name").toString("EmbeddedProject");
    emit projectNameChanged(m_projectName);

    m_targetFramework = root.value("target").toString("ugfx");
    emit targetFrameworkChanged(m_targetFramework);

    if (root.contains("display") && root.value("display").isObject()) {
        QJsonObject disp = root.value("display").toObject();
        m_screenWidth = disp.value("width").toInt(320);
        m_screenHeight = disp.value("height").toInt(240);
        m_colorDepth = disp.value("colorDepth").toInt(16);
        emit screenDimensionsChanged();
        emit colorDepthChanged(m_colorDepth);
    }

    // Loop through screens first, then widgets
    if (root.contains("pages") && root.value("pages").isArray()) {
        QJsonArray pages = root.value("pages").toArray();
        for (int i = 0; i < pages.size(); ++i) {
            QJsonObject pageObj = pages.at(i).toObject();
            auto screen = new ScreenModel(QString(), QString(), m_screenWidth, m_screenHeight, this);
            screen->fromJson(pageObj);
            m_screens.append(screen);
            connectScreenSignals(screen);
        }
    }

    if (m_screens.isEmpty()) {
        auto defaultScreen = new ScreenModel("MainScreen", "Main Screen", m_screenWidth, m_screenHeight, this);
        m_screens.append(defaultScreen);
        connectScreenSignals(defaultScreen);
    }

    m_activeScreenIndex = 0;
    hookActiveScreenSignals();

    emit screenCountChanged(m_screens.size());
    emit screensChanged();
    emit activeScreenIndexChanged(0);
    emit activeScreenChanged();
    emit countChanged(count());

    m_undoStack.clear();
    setSelectedIndex(-1);
    emit documentModified();
    return true;
}

bool DocumentModel::saveToFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QJsonDocument doc(toJson());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool DocumentModel::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for reading:" << filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON document:" << filePath;
        return false;
    }

    return fromJson(doc.object());
}

void DocumentModel::loadSampleProject() {
    if (QFile::exists(":/examples/sample_dashboard.euiproj") && loadFromFile(":/examples/sample_dashboard.euiproj")) {
        return;
    }

    clear();
    setProjectName("EmbeddedDemo");
    setTargetFramework("ugfx");
    setScreenWidth(320);
    setScreenHeight(240);
    setColorDepth(16);

    // Screen 1: Dashboard
    auto screen1 = new ScreenModel("MainScreen", "Dashboard", 320, 240, this);
    auto lbl1 = new WidgetModel("lbl_title", "Label", 20, 20, 200, 30, QColor("#FFFFFF"), screen1);
    lbl1->setText("MCU Dashboard");
    screen1->insertWidget(0, lbl1);

    auto btn1 = new WidgetModel("btn_nav_settings", "Button", 20, 70, 160, 40, QColor("#2196F3"), screen1);
    btn1->setText("Open Settings ->");
    btn1->setTargetScreenId("SettingsScreen");
    screen1->insertWidget(1, btn1);

    auto prog1 = new WidgetModel("prog_cpu", "ProgressBar", 20, 130, 200, 20, QColor("#4CAF50"), screen1);
    screen1->insertWidget(2, prog1);

    m_screens.append(screen1);
    connectScreenSignals(screen1);

    // Screen 2: Settings
    auto screen2 = new ScreenModel("SettingsScreen", "Settings", 320, 240, this);
    auto lbl2 = new WidgetModel("lbl_settings", "Label", 20, 20, 200, 30, QColor("#FFFFFF"), screen2);
    lbl2->setText("System Config");
    screen2->insertWidget(0, lbl2);

    auto btnBack = new WidgetModel("btn_nav_home", "Button", 20, 70, 160, 40, QColor("#E06C75"), screen2);
    btnBack->setText("<- Back to Home");
    btnBack->setTargetScreenId("MainScreen");
    screen2->insertWidget(1, btnBack);

    auto rectBox = new WidgetModel("rect_card", "Rectangle", 20, 130, 280, 80, QColor("#2C313A"), screen2);
    screen2->insertWidget(2, rectBox);

    m_screens.append(screen2);
    connectScreenSignals(screen2);

    setActiveScreenIndex(0);
    emit screenCountChanged(m_screens.size());
    emit screensChanged();
    m_undoStack.clear();
    setSelectedIndex(-1);
    emit documentModified();
}

bool DocumentModel::exportUgfx(const QString& outDir) {
    UgfxExporter exporter;
    bool ok = exporter.exportProject(this, outDir);
    if (!ok) {
        m_lastExportError = exporter.lastError();
    }
    return ok;
}

bool DocumentModel::exportQul(const QString& outDir) {
    QtMcuExporter exporter;
    bool ok = exporter.exportProject(this, outDir);
    if (!ok) {
        m_lastExportError = exporter.lastError();
    }
    return ok;
}
