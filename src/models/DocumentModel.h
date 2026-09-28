#pragma once

#include <QAbstractListModel>
#include <QUndoStack>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>
#include "WidgetModel.h"
#include "ScreenModel.h"

/**
 * @brief Core Document Model (DOM & MVVM ViewModel)
 * 
 * Manages the multi-screen embedded project hierarchy.
 * Holds a list of ScreenModel instances, each of which contains a collection of WidgetModel items.
 * Implements QAbstractListModel delegating to the currently active ScreenModel so existing
 * canvas bindings remain fast, reactive, and fully functional.
 * 
 * Exposes activeScreenIndex and activeScreen properties for multi-screen navigation.
 * Integrates QUndoStack to guarantee safe Undo/Redo across all visual manipulations.
 */
class DocumentModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString projectName READ projectName WRITE setProjectName NOTIFY projectNameChanged)
    Q_PROPERTY(int screenWidth READ screenWidth WRITE setScreenWidth NOTIFY screenDimensionsChanged)
    Q_PROPERTY(int screenHeight READ screenHeight WRITE setScreenHeight NOTIFY screenDimensionsChanged)
    Q_PROPERTY(int colorDepth READ colorDepth WRITE setColorDepth NOTIFY colorDepthChanged)
    Q_PROPERTY(QString targetFramework READ targetFramework WRITE setTargetFramework NOTIFY targetFrameworkChanged)
    
    // Multi-Screen Properties
    Q_PROPERTY(int activeScreenIndex READ activeScreenIndex WRITE setActiveScreenIndex NOTIFY activeScreenIndexChanged)
    Q_PROPERTY(ScreenModel* activeScreen READ activeScreen NOTIFY activeScreenChanged)
    Q_PROPERTY(int screenCount READ screenCount NOTIFY screenCountChanged)
    Q_PROPERTY(QVariantList screensList READ screensList NOTIFY screensChanged)

    // Active Screen Widget Collection & Selection Properties
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(WidgetModel* selectedWidget READ selectedWidget NOTIFY selectedWidgetChanged)

    // QUndoStack Properties
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoStackChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoStackChanged)
    Q_PROPERTY(QString undoText READ undoText NOTIFY undoStackChanged)
    Q_PROPERTY(QString redoText READ redoText NOTIFY undoStackChanged)

public:
    enum WidgetRoles {
        IdRole = ScreenModel::IdRole,
        TypeRole = ScreenModel::TypeRole,
        XRole = ScreenModel::XRole,
        YRole = ScreenModel::YRole,
        WidthRole = ScreenModel::WidthRole,
        HeightRole = ScreenModel::HeightRole,
        ColorRole = ScreenModel::ColorRole,
        TextRole = ScreenModel::TextRole,
        TargetScreenIdRole = ScreenModel::TargetScreenIdRole,
        ImagePathRole = ScreenModel::ImagePathRole,
        WidgetRole = ScreenModel::WidgetRole
    };
    Q_ENUM(WidgetRoles)

    explicit DocumentModel(QObject* parent = nullptr);
    virtual ~DocumentModel() override;

    // QAbstractListModel interface (delegates to active screen)
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    // Document Properties
    QString projectName() const { return m_projectName; }
    void setProjectName(const QString& name);

    int screenWidth() const { return m_screenWidth; }
    void setScreenWidth(int w);

    int screenHeight() const { return m_screenHeight; }
    void setScreenHeight(int h);

    int colorDepth() const { return m_colorDepth; }
    void setColorDepth(int depth);

    QString targetFramework() const { return m_targetFramework; }
    void setTargetFramework(const QString& target);

    // Multi-Screen Management
    int activeScreenIndex() const { return m_activeScreenIndex; }
    void setActiveScreenIndex(int index);

    ScreenModel* activeScreen() const;
    int screenCount() const { return m_screens.size(); }
    QVariantList screensList() const;
    QList<ScreenModel*> screens() const { return m_screens; }

    Q_INVOKABLE ScreenModel* addScreen(const QString& name = QString(), const QString& id = QString());
    Q_INVOKABLE bool removeScreen(int index);
    Q_INVOKABLE ScreenModel* getScreen(int index) const;
    Q_INVOKABLE ScreenModel* getScreenById(const QString& id) const;

    // Active Screen Widget Accessors
    int count() const;
    int selectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int idx);
    WidgetModel* selectedWidget() const;

    // QUndoStack bindings
    QUndoStack* undoStack() { return &m_undoStack; }
    bool canUndo() const { return m_undoStack.canUndo(); }
    bool canRedo() const { return m_undoStack.canRedo(); }
    QString undoText() const { return m_undoStack.undoText(); }
    QString redoText() const { return m_undoStack.redoText(); }

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    // Undo-managed mutations on active screen
    Q_INVOKABLE void addWidgetWithUndo(const QString& type, qreal x, qreal y, qreal width = 120, qreal height = 40, const QColor& color = QColor(33, 150, 243), const QString& text = QString(), const QString& targetScreenId = QString());
    Q_INVOKABLE void moveWidgetWithUndo(int index, qreal newX, qreal newY);
    Q_INVOKABLE void removeWidgetWithUndo(int index);

    // Direct model mutations on active screen
    void insertWidget(int index, WidgetModel* widget);
    WidgetModel* takeWidget(int index);
    void clear();

    // Accessors for active screen widgets
    Q_INVOKABLE WidgetModel* getWidget(int index) const;
    QList<WidgetModel*> widgets() const;

    // Serialization
    Q_INVOKABLE bool saveToFile(const QString& filePath);
    Q_INVOKABLE bool loadFromFile(const QString& filePath);
    Q_INVOKABLE void loadSampleProject();
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);

    // Code Generation Export Helpers
    Q_INVOKABLE bool exportUgfx(const QString& outDir = "output_ugfx");
    Q_INVOKABLE bool exportQul(const QString& outDir = "output_qul");
    Q_INVOKABLE QString lastExportError() const { return m_lastExportError; }

signals:
    void projectNameChanged(const QString& name);
    void screenDimensionsChanged();
    void colorDepthChanged(int depth);
    void targetFrameworkChanged(const QString& target);
    void activeScreenIndexChanged(int index);
    void activeScreenChanged();
    void screenCountChanged(int count);
    void screensChanged();
    void countChanged(int newCount);
    void selectedIndexChanged(int index);
    void selectedWidgetChanged();
    void undoStackChanged();
    void documentModified();

private:
    QString m_projectName = "EmbeddedProject";
    int m_screenWidth = 320;
    int m_screenHeight = 240;
    int m_colorDepth = 16;
    QString m_targetFramework = "ugfx";

    int m_activeScreenIndex = 0;
    int m_selectedIndex = -1;

    QList<ScreenModel*> m_screens;
    QUndoStack m_undoStack;
    QString m_lastExportError;

    void connectScreenSignals(ScreenModel* screen);
    void hookActiveScreenSignals();
};
