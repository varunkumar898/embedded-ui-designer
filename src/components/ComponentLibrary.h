#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QVariantMap>

struct LVGLPropertyMeta {
    QString name;
    QString type; // "string", "int", "bool", "enum"
    QStringList options;
    QVariant defaultValue;
};

struct LVGLComponentMeta {
    QString id;
    QString name;
    QString lvglType;
    QString category;
    QString description;
    int defaultWidth = 100;
    int defaultHeight = 40;
    QList<LVGLPropertyMeta> properties;
    QString codeTemplate;

    bool isValid() const { return !id.isEmpty(); }
};

/**
 * @brief ComponentLibrary manages the registered LVGL embedded UI component presets,
 * metadata schemas, and target board compatibility matrices.
 */
class ComponentLibrary : public QObject {
    Q_OBJECT

public:
    static ComponentLibrary& instance();

    explicit ComponentLibrary(QObject* parent = nullptr);
    ~ComponentLibrary() override = default;

    bool loadLibrary(const QString& jsonPath = "");
    QList<LVGLComponentMeta> allComponents() const;
    int componentCount() const { return m_components.size(); }

    LVGLComponentMeta findComponentById(const QString& id) const;
    LVGLComponentMeta findComponentByName(const QString& name) const;

    // Board compatibility matrix check
    bool isBoardCompatible(const QString& componentId, const QString& boardId) const;

    // Code generation expansion
    QString generateWidgetCode(const QString& componentId,
                               const QString& widgetName,
                               int x, int y, int width, int height,
                               const QVariantMap& customProps = QVariantMap(),
                               const QString& parentObj = "lv_scr_act()") const;

signals:
    void libraryLoaded();

private:
    void populateBuiltins();
    bool parseJson(const QByteArray& data);

    QList<LVGLComponentMeta> m_components;
    QMap<QString, LVGLComponentMeta> m_componentsById;
    QMap<QString, QStringList> m_compatibilityMatrix;
};
