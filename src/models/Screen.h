#pragma once

#include <QObject>
#include <QString>
#include <QColor>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include "UIComponent.h"
#include "DataBinding.h"
#include "DataSource.h"

class Screen : public QObject {
    Q_OBJECT

public:
    explicit Screen(const QString& id, const QString& name = QString(), QObject* parent = nullptr);
    ~Screen() override;

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

    const QList<qreal>& hGuides() const { return m_hGuides; }
    const QList<qreal>& vGuides() const { return m_vGuides; }
    void setGuides(const QList<qreal>& hg, const QList<qreal>& vg);

    // Component management
    const QList<UIComponent*>& components() const { return m_components; }
    void addComponent(UIComponent* comp);
    void removeComponent(UIComponent* comp);
    bool hasComponent(UIComponent* comp) const;
    UIComponent* findComponentById(const QString& compId) const;
    void clearComponents(bool deleteObjects = true);
    void setComponents(const QList<UIComponent*>& comps);

    // Screen-level Data Sources & Bindings
    const QList<DataSource>& dataSources() const { return m_dataSources; }
    void addDataSource(const DataSource& source);
    void removeDataSource(const QString& sourceId);
    const DataSource* findDataSource(const QString& sourceId) const;
    void setDataSources(const QList<DataSource>& sources);

    const QList<DataBinding>& dataBindings() const { return m_dataBindings; }
    void addDataBinding(const DataBinding& binding);
    void removeDataBinding(const QString& componentId, const QString& propertyName);
    void setDataBindings(const QList<DataBinding>& bindings);

    // Deep clone (for duplicating screens)
    Screen* clone(const QString& newId, const QString& newName, QObject* parent = nullptr) const;

    // Serialization
    QJsonObject toJson() const;
    static Screen* fromJson(const QJsonObject& json, QObject* parent = nullptr);

signals:
    void screenChanged();
    void componentAdded(UIComponent* comp);
    void componentRemoved(UIComponent* comp);

private:
    QString m_id;
    QString m_name;
    int m_width = 320;
    int m_height = 240;
    QColor m_backgroundColor = Qt::white;
    QList<qreal> m_hGuides;
    QList<qreal> m_vGuides;
    QList<UIComponent*> m_components;
    QList<DataSource> m_dataSources;
    QList<DataBinding> m_dataBindings;
};
