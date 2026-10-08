#include "Screen.h"
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

static UIComponent* createComponentInstanceHelper(const QString& type, const QString& id) {
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
    }
    return nullptr;
}

Screen::Screen(const QString& id, const QString& name, QObject* parent)
    : QObject(parent)
    , m_id(id)
    , m_name(name.isEmpty() ? id : name)
{
}

Screen::~Screen() {
    blockSignals(true);
    clearComponents(true);
}


void Screen::setId(const QString& id) {
    if (m_id != id) {
        m_id = id;
        emit screenChanged();
    }
}

void Screen::setName(const QString& name) {
    if (m_name != name) {
        m_name = name;
        emit screenChanged();
    }
}

void Screen::setWidth(int w) {
    if (m_width != w) {
        m_width = w;
        emit screenChanged();
    }
}

void Screen::setHeight(int h) {
    if (m_height != h) {
        m_height = h;
        emit screenChanged();
    }
}

void Screen::setBackgroundColor(const QColor& color) {
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        emit screenChanged();
    }
}

void Screen::setGuides(const QList<qreal>& hg, const QList<qreal>& vg) {
    m_hGuides = hg;
    m_vGuides = vg;
    emit screenChanged();
}

void Screen::addComponent(UIComponent* comp) {
    if (!comp || m_components.contains(comp)) return;
    m_components.append(comp);
    connect(comp, &UIComponent::propertyChanged, this, [this]() {
        emit screenChanged();
    });
    connect(comp, &UIComponent::geometryChangedSignal, this, [this]() {
        emit screenChanged();
    });
    emit componentAdded(comp);
    emit screenChanged();
}

void Screen::removeComponent(UIComponent* comp) {
    if (!comp) return;
    if (m_components.removeOne(comp)) {
        disconnect(comp, nullptr, this, nullptr);
        emit componentRemoved(comp);
        emit screenChanged();
    }
}

bool Screen::hasComponent(UIComponent* comp) const {
    return m_components.contains(comp);
}

UIComponent* Screen::findComponentById(const QString& compId) const {
    for (UIComponent* c : m_components) {
        if (c && c->componentId() == compId) return c;
    }
    return nullptr;
}

void Screen::clearComponents(bool deleteObjects) {
    QList<UIComponent*> copy = m_components;
    m_components.clear();
    for (UIComponent* comp : copy) {
        if (comp) {
            disconnect(comp, nullptr, this, nullptr);
            emit componentRemoved(comp);
            if (deleteObjects) {
                delete comp;
            }
        }
    }
    emit screenChanged();
}

void Screen::setComponents(const QList<UIComponent*>& comps) {
    clearComponents(false);
    for (UIComponent* comp : comps) {
        addComponent(comp);
    }
}

void Screen::addDataSource(const DataSource& source) {
    for (int i = 0; i < m_dataSources.size(); ++i) {
        if (m_dataSources[i].id() == source.id()) {
            m_dataSources[i] = source;
            emit screenChanged();
            return;
        }
    }
    m_dataSources.append(source);
    emit screenChanged();
}

void Screen::removeDataSource(const QString& sourceId) {
    for (int i = 0; i < m_dataSources.size(); ++i) {
        if (m_dataSources[i].id() == sourceId) {
            m_dataSources.removeAt(i);
            emit screenChanged();
            return;
        }
    }
}

const DataSource* Screen::findDataSource(const QString& sourceId) const {
    for (const auto& ds : m_dataSources) {
        if (ds.id() == sourceId) return &ds;
    }
    return nullptr;
}

void Screen::setDataSources(const QList<DataSource>& sources) {
    m_dataSources = sources;
    emit screenChanged();
}

void Screen::addDataBinding(const DataBinding& binding) {
    for (int i = 0; i < m_dataBindings.size(); ++i) {
        if (m_dataBindings[i].componentId() == binding.componentId() &&
            m_dataBindings[i].propertyName() == binding.propertyName()) {
            m_dataBindings[i] = binding;
            emit screenChanged();
            return;
        }
    }
    m_dataBindings.append(binding);
    emit screenChanged();
}

void Screen::removeDataBinding(const QString& componentId, const QString& propertyName) {
    for (int i = 0; i < m_dataBindings.size(); ++i) {
        if (m_dataBindings[i].componentId() == componentId &&
            (propertyName.isEmpty() || m_dataBindings[i].propertyName() == propertyName)) {
            m_dataBindings.removeAt(i);
            emit screenChanged();
            return;
        }
    }
}

void Screen::setDataBindings(const QList<DataBinding>& bindings) {
    m_dataBindings = bindings;
    emit screenChanged();
}

Screen* Screen::clone(const QString& newId, const QString& newName, QObject* parent) const {
    Screen* copy = new Screen(newId, newName.isEmpty() ? (m_name + " (Copy)") : newName, parent);
    copy->m_width = m_width;
    copy->m_height = m_height;
    copy->m_backgroundColor = m_backgroundColor;
    copy->m_hGuides = m_hGuides;
    copy->m_vGuides = m_vGuides;
    copy->m_dataSources = m_dataSources;

    // Clone components with new/unique IDs
    QMap<QString, QString> idMap;
    for (UIComponent* comp : m_components) {
        if (!comp) continue;
        QJsonObject compJson = comp->toJson();
        QString oldCompId = comp->componentId();
        QString newCompId = oldCompId + "_copy";
        compJson["id"] = newCompId;
        idMap[oldCompId] = newCompId;

        UIComponent* newComp = createComponentInstanceHelper(comp->componentType(), newCompId);
        if (newComp) {
            newComp->fromJson(compJson);
            copy->addComponent(newComp);
        }
    }

    // Clone bindings with updated component IDs
    for (const DataBinding& b : m_dataBindings) {
        DataBinding copyB = b;
        if (idMap.contains(b.componentId())) {
            copyB.setComponentId(idMap[b.componentId()]);
        }
        copy->addDataBinding(copyB);
    }

    return copy;
}

QJsonObject Screen::toJson() const {
    QJsonObject obj;
    obj["id"] = m_id;
    obj["name"] = m_name;
    obj["width"] = m_width;
    obj["height"] = m_height;
    obj["backgroundColor"] = m_backgroundColor.name(QColor::HexArgb);

    // Guides
    QJsonArray hg, vg;
    for (qreal y : m_hGuides) hg.append(y);
    for (qreal x : m_vGuides) vg.append(x);
    obj["hGuides"] = hg;
    obj["vGuides"] = vg;

    // Components
    QJsonArray compsArr;
    for (UIComponent* comp : m_components) {
        if (comp) {
            compsArr.append(comp->toJson());
        }
    }
    obj["components"] = compsArr;

    // Data sources
    if (!m_dataSources.isEmpty()) {
        QJsonArray dsArr;
        for (const auto& ds : m_dataSources) {
            dsArr.append(ds.toJson());
        }
        obj["dataSources"] = dsArr;
    }

    // Data bindings
    if (!m_dataBindings.isEmpty()) {
        QJsonArray dbArr;
        for (const auto& db : m_dataBindings) {
            dbArr.append(db.toJson());
        }
        obj["bindings"] = dbArr;
    }

    return obj;
}

Screen* Screen::fromJson(const QJsonObject& json, QObject* parent) {
    QString id = json.value("id").toString("screen_1");
    QString name = json.value("name").toString(id);
    Screen* s = new Screen(id, name, parent);
    s->m_width = json.value("width").toInt(320);
    s->m_height = json.value("height").toInt(240);
    if (json.contains("backgroundColor")) {
        s->m_backgroundColor = QColor(json.value("backgroundColor").toString());
    }

    // Ruler guides
    for (const QJsonValue& v : json.value("hGuides").toArray()) {
        s->m_hGuides.append(v.toDouble());
    }
    for (const QJsonValue& v : json.value("vGuides").toArray()) {
        s->m_vGuides.append(v.toDouble());
    }

    // Components
    QJsonArray compsArr = json.value("components").toArray();
    for (int i = 0; i < compsArr.size(); ++i) {
        QJsonObject compObj = compsArr[i].toObject();
        QString type = compObj.value("type").toString();
        QString compId = compObj.value("id").toString();
        UIComponent* comp = createComponentInstanceHelper(type, compId);
        if (comp) {
            comp->fromJson(compObj);
            s->addComponent(comp);
        }
    }

    // Data Sources
    if (json.contains("dataSources") && json.value("dataSources").isArray()) {
        for (const QJsonValue& v : json.value("dataSources").toArray()) {
            s->m_dataSources.append(DataSource::fromJson(v.toObject()));
        }
    }

    // Data Bindings
    if (json.contains("bindings") && json.value("bindings").isArray()) {
        for (const QJsonValue& v : json.value("bindings").toArray()) {
            s->m_dataBindings.append(DataBinding::fromJson(v.toObject()));
        }
    }

    return s;
}
