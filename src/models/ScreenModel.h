#pragma once

#include <QAbstractListModel>
#include <QColor>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include "WidgetModel.h"

/**
 * @brief Represents a single screen/page in the multi-screen embedded project.
 * 
 * Implements QAbstractListModel so QML views can bind directly to the screen's widgets.
 */
class ScreenModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(QString id READ id WRITE setId NOTIFY idChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY sizeChanged)
    Q_PROPERTY(int height READ height WRITE setHeight NOTIFY sizeChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum WidgetRoles {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        XRole,
        YRole,
        WidthRole,
        HeightRole,
        ColorRole,
        TextRole,
        TargetScreenIdRole,
        ImagePathRole,
        WidgetRole
    };
    Q_ENUM(WidgetRoles)

    explicit ScreenModel(const QString& id = "MainScreen", const QString& name = "Main Screen", int width = 320, int height = 240, QObject* parent = nullptr);
    virtual ~ScreenModel() override;

    // QAbstractListModel overrides
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    // Screen Properties
    QString id() const { return m_id; }
    void setId(const QString& id);

    QString name() const { return m_name; }
    void setName(const QString& name);

    int width() const { return m_width; }
    void setWidth(int w);

    int height() const { return m_height; }
    void setHeight(int h);

    QColor backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const QColor& color);

    int count() const { return m_widgets.size(); }

    // Direct widget list mutations
    Q_INVOKABLE void insertWidget(int index, WidgetModel* widget);
    Q_INVOKABLE WidgetModel* takeWidget(int index);
    Q_INVOKABLE void clear();

    // Accessors
    Q_INVOKABLE WidgetModel* getWidget(int index) const;
    QList<WidgetModel*> widgets() const { return m_widgets; }

    // Serialization
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);

signals:
    void idChanged(const QString& id);
    void nameChanged(const QString& name);
    void sizeChanged();
    void backgroundColorChanged(const QColor& color);
    void countChanged(int newCount);
    void screenModified();

private:
    QString m_id = "MainScreen";
    QString m_name = "Main Screen";
    int m_width = 320;
    int m_height = 240;
    QColor m_backgroundColor = QColor("#181A1F");

    QList<WidgetModel*> m_widgets;

    void connectWidgetSignals(WidgetModel* widget);
};
