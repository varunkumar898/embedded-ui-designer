#pragma once

#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QColor>
#include <QJsonObject>
#include <QJsonArray>
#include "Project.h"
#include "Screen.h"
#include "UIComponent.h"
#include "DataSource.h"
#include "DataBinding.h"
#include "ComponentState.h"

namespace CodeGen {

struct IRWidget {
    QString id;
    QString type;
    qreal x = 0;
    qreal y = 0;
    qreal width = 100;
    qreal height = 40;
    int zOrder = 0;
    QString parentScreenId;
    QJsonObject properties;
    QMap<QString, ComponentStateStyle> stateStyles;
    QList<DataBinding> bindings;
    QJsonArray interactions;
};

struct IRScreen {
    QString id;
    QString name;
    int width = 320;
    int height = 240;
    QColor backgroundColor = Qt::white;
    QList<IRWidget> widgets;
};

struct IRDataSource {
    QString id;
    QString name;
    DataSourceType type;
    DataDirection direction;
    DataType dataType;
    QString hardwareRef;
    QVariant initialValue;
};

struct IRBinding {
    QString componentId;
    QString propertyName;
    QString sourceId;
    BindingDirection direction;
    QString transformExpression;
};

struct IRProject {
    QString name;
    QString framework;
    QString targetFamily;
    QString targetBoard;
    int displayWidth = 320;
    int displayHeight = 240;
    bool isRound = false;
    int colorDepth = 16;
    QList<IRScreen> screens;
    QList<IRDataSource> dataSources;
    QList<IRBinding> bindings;
};

class GeneratorIR {
public:
    static IRProject buildFromProject(const Project* project);
};

} // namespace CodeGen
