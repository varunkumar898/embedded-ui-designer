#include "ComponentLibrary.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDebug>

ComponentLibrary& ComponentLibrary::instance() {
    static ComponentLibrary s_instance;
    return s_instance;
}

ComponentLibrary::ComponentLibrary(QObject* parent)
    : QObject(parent)
{
    loadLibrary();
}

bool ComponentLibrary::loadLibrary(const QString& jsonPath) {
    QString path = jsonPath;
    if (path.isEmpty()) {
        QString appDir = QCoreApplication::applicationDirPath();
        QStringList candidates = {
            "config/components/lvgl_components.json",
            appDir + "/config/components/lvgl_components.json",
            appDir + "/../config/components/lvgl_components.json",
            "/home/cherry/embedded-ui-designer/config/components/lvgl_components.json"
        };
        for (const QString& candidate : candidates) {
            if (QFile::exists(candidate)) {
                path = candidate;
                break;
            }
        }
    }

    if (!path.isEmpty()) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            if (parseJson(data)) {
                qDebug() << "[ComponentLibrary] Loaded" << m_components.size() << "LVGL components from" << path;
                return true;
            }
        }
    }

    populateBuiltins();
    return !m_components.isEmpty();
}

bool ComponentLibrary::parseJson(const QByteArray& data) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray compArray = root["components"].toArray();
    if (compArray.isEmpty()) return false;

    m_components.clear();
    m_componentsById.clear();

    for (const auto& item : compArray) {
        if (!item.isObject()) continue;
        QJsonObject obj = item.toObject();

        LVGLComponentMeta meta;
        meta.id = obj["id"].toString();
        meta.name = obj["name"].toString();
        meta.lvglType = obj["lvgl_type"].toString();
        meta.category = obj["category"].toString();
        meta.description = obj["description"].toString();
        meta.defaultWidth = obj["default_width"].toInt(100);
        meta.defaultHeight = obj["default_height"].toInt(40);
        meta.codeTemplate = obj["c_create"].toString();

        QJsonArray props = obj["properties"].toArray();
        for (const auto& pVal : props) {
            if (!pVal.isObject()) continue;
            QJsonObject pObj = pVal.toObject();
            LVGLPropertyMeta prop;
            prop.name = pObj["name"].toString();
            prop.type = pObj["type"].toString();
            prop.defaultValue = pObj["default"].toVariant();
            QJsonArray opts = pObj["options"].toArray();
            for (const auto& opt : opts) prop.options.append(opt.toString());
            meta.properties.append(prop);
        }

        m_components.append(meta);
        m_componentsById[meta.id] = meta;
    }

    emit libraryLoaded();
    return !m_components.isEmpty();
}

void ComponentLibrary::populateBuiltins() {
    if (!m_components.isEmpty()) return;

    QStringList ids = {"lv_container", "lv_button", "lv_label", "lv_slider", "lv_switch", "lv_gauge", "lv_chart", "lv_spinbox", "lv_keyboard"};
    QStringList names = {"Container", "Button", "Label", "Slider", "Switch", "Gauge", "Chart", "Spinbox", "Keyboard"};

    for (int i = 0; i < ids.size(); ++i) {
        LVGLComponentMeta meta;
        meta.id = ids[i];
        meta.name = names[i];
        meta.category = "LVGL";
        m_components.append(meta);
        m_componentsById[meta.id] = meta;
    }
}

QList<LVGLComponentMeta> ComponentLibrary::allComponents() const {
    return m_components;
}

LVGLComponentMeta ComponentLibrary::findComponentById(const QString& id) const {
    return m_componentsById.value(id);
}

LVGLComponentMeta ComponentLibrary::findComponentByName(const QString& name) const {
    for (const auto& c : m_components) {
        if (c.name.compare(name, Qt::CaseInsensitive) == 0 ||
            c.id.compare(name, Qt::CaseInsensitive) == 0) {
            return c;
        }
    }
    return LVGLComponentMeta();
}

bool ComponentLibrary::isBoardCompatible(const QString& componentId, const QString& boardId) const {
    // Keyboard requires at least 32KB RAM (so not small STM32F0 with 8KB RAM)
    if (componentId == "lv_keyboard" && boardId.contains("f030", Qt::CaseInsensitive)) {
        return false;
    }
    return true;
}

QString ComponentLibrary::generateWidgetCode(const QString& componentId,
                                            const QString& widgetName,
                                            int x, int y, int width, int height,
                                            const QVariantMap& customProps,
                                            const QString& parentObj) const {
    LVGLComponentMeta meta = findComponentById(componentId);
    if (!meta.isValid()) {
        meta = findComponentByName(componentId);
    }
    if (!meta.isValid() || meta.codeTemplate.isEmpty()) {
        return QString("    // LVGL widget %1\n    lv_obj_t* %1 = lv_obj_create(%2);\n    lv_obj_set_pos(%1, %3, %4);\n    lv_obj_set_size(%1, %5, %6);")
            .arg(widgetName, parentObj, QString::number(x), QString::number(y), QString::number(width), QString::number(height));
    }

    QString code = meta.codeTemplate;
    code.replace("%name%", widgetName);
    code.replace("%parent%", parentObj);
    code.replace("%x%", QString::number(x));
    code.replace("%y%", QString::number(y));
    code.replace("%width%", QString::number(width));
    code.replace("%height%", QString::number(height));

    for (const auto& prop : meta.properties) {
        QString key = "%" + prop.name + "%";
        QVariant val = customProps.value(prop.name, prop.defaultValue);
        code.replace(key, val.toString());
    }

    return code;
}
