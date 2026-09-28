#include "ScreenModel.h"

ScreenModel::ScreenModel(const QString& id, const QString& name, int width, int height, QObject* parent)
    : QAbstractListModel(parent)
    , m_id(id)
    , m_name(name)
    , m_width(width)
    , m_height(height)
{
}

ScreenModel::~ScreenModel() {
    clear();
}

int ScreenModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_widgets.size();
}

QVariant ScreenModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_widgets.size()) {
        return QVariant();
    }

    WidgetModel* w = m_widgets.at(index.row());
    if (!w) return QVariant();

    switch (role) {
        case IdRole: return w->id();
        case TypeRole: return w->type();
        case XRole: return w->x();
        case YRole: return w->y();
        case WidthRole: return w->width();
        case HeightRole: return w->height();
        case ColorRole: return w->color();
        case TextRole: return w->text();
        case TargetScreenIdRole: return w->targetScreenId();
        case ImagePathRole: return w->imagePath();
        case WidgetRole: return QVariant::fromValue(w);
        case Qt::DisplayRole: return w->id();
        default: return QVariant();
    }
}

bool ScreenModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_widgets.size()) {
        return false;
    }

    WidgetModel* w = m_widgets.at(index.row());
    if (!w) return false;

    bool changed = false;
    switch (role) {
        case IdRole:
            if (w->id() != value.toString()) { w->setId(value.toString()); changed = true; }
            break;
        case TypeRole:
            if (w->type() != value.toString()) { w->setType(value.toString()); changed = true; }
            break;
        case XRole:
            if (w->x() != value.toReal()) { w->setX(value.toReal()); changed = true; }
            break;
        case YRole:
            if (w->y() != value.toReal()) { w->setY(value.toReal()); changed = true; }
            break;
        case WidthRole:
            if (w->width() != value.toReal()) { w->setWidth(value.toReal()); changed = true; }
            break;
        case HeightRole:
            if (w->height() != value.toReal()) { w->setHeight(value.toReal()); changed = true; }
            break;
        case ColorRole:
            if (w->color() != value.value<QColor>()) { w->setColor(value.value<QColor>()); changed = true; }
            break;
        case TextRole:
            if (w->text() != value.toString()) { w->setText(value.toString()); changed = true; }
            break;
        case TargetScreenIdRole:
            if (w->targetScreenId() != value.toString()) { w->setTargetScreenId(value.toString()); changed = true; }
            break;
        case ImagePathRole:
            if (w->imagePath() != value.toString()) { w->setImagePath(value.toString()); changed = true; }
            break;
        default:
            break;
    }

    if (changed) {
        emit dataChanged(index, index, {role});
        emit screenModified();
    }
    return changed;
}

QHash<int, QByteArray> ScreenModel::roleNames() const {
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

void ScreenModel::setId(const QString& id) {
    if (m_id != id) {
        m_id = id;
        emit idChanged(m_id);
        emit screenModified();
    }
}

void ScreenModel::setName(const QString& name) {
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
        emit screenModified();
    }
}

void ScreenModel::setWidth(int w) {
    if (m_width != w) {
        m_width = w;
        emit sizeChanged();
        emit screenModified();
    }
}

void ScreenModel::setHeight(int h) {
    if (m_height != h) {
        m_height = h;
        emit sizeChanged();
        emit screenModified();
    }
}

void ScreenModel::setBackgroundColor(const QColor& color) {
    if (m_backgroundColor != color) {
        m_backgroundColor = color;
        emit backgroundColorChanged(m_backgroundColor);
        emit screenModified();
    }
}

void ScreenModel::insertWidget(int index, WidgetModel* widget) {
    if (!widget) return;

    if (index < 0 || index > m_widgets.size()) {
        index = m_widgets.size();
    }

    beginInsertRows(QModelIndex(), index, index);
    widget->setParent(this);
    m_widgets.insert(index, widget);
    connectWidgetSignals(widget);
    endInsertRows();

    emit countChanged(m_widgets.size());
    emit screenModified();
}

WidgetModel* ScreenModel::takeWidget(int index) {
    if (index < 0 || index >= m_widgets.size()) return nullptr;

    beginRemoveRows(QModelIndex(), index, index);
    WidgetModel* widget = m_widgets.takeAt(index);
    widget->disconnect(this);
    endRemoveRows();

    emit countChanged(m_widgets.size());
    emit screenModified();
    return widget;
}

void ScreenModel::clear() {
    if (m_widgets.isEmpty()) return;

    beginResetModel();
    qDeleteAll(m_widgets);
    m_widgets.clear();
    endResetModel();

    emit countChanged(0);
    emit screenModified();
}

WidgetModel* ScreenModel::getWidget(int index) const {
    if (index >= 0 && index < m_widgets.size()) {
        return m_widgets.at(index);
    }
    return nullptr;
}

void ScreenModel::connectWidgetSignals(WidgetModel* widget) {
    connect(widget, &WidgetModel::geometryChanged, this, [this, widget]() {
        int r = m_widgets.indexOf(widget);
        if (r != -1) {
            emit dataChanged(index(r), index(r), {XRole, YRole, WidthRole, HeightRole});
            emit screenModified();
        }
    });

    connect(widget, &WidgetModel::colorChanged, this, [this, widget]() {
        int r = m_widgets.indexOf(widget);
        if (r != -1) {
            emit dataChanged(index(r), index(r), {ColorRole});
            emit screenModified();
        }
    });

    connect(widget, &WidgetModel::textChanged, this, [this, widget]() {
        int r = m_widgets.indexOf(widget);
        if (r != -1) {
            emit dataChanged(index(r), index(r), {TextRole});
            emit screenModified();
        }
    });

    connect(widget, &WidgetModel::targetScreenIdChanged, this, [this, widget]() {
        int r = m_widgets.indexOf(widget);
        if (r != -1) {
            emit dataChanged(index(r), index(r), {TargetScreenIdRole});
            emit screenModified();
        }
    });

    connect(widget, &WidgetModel::imagePathChanged, this, [this, widget]() {
        int r = m_widgets.indexOf(widget);
        if (r != -1) {
            emit dataChanged(index(r), index(r), {ImagePathRole});
            emit screenModified();
        }
    });
}

QJsonObject ScreenModel::toJson() const {
    QJsonObject page;
    page["id"] = m_id;
    page["name"] = m_name;
    page["width"] = m_width;
    page["height"] = m_height;
    page["backgroundColor"] = m_backgroundColor.name();

    QJsonArray comps;
    for (WidgetModel* w : m_widgets) {
        if (w) comps.append(w->toJson());
    }
    page["components"] = comps;

    return page;
}

bool ScreenModel::fromJson(const QJsonObject& json) {
    clear();

    m_id = json.value("id").toString(m_id);
    m_name = json.value("name").toString(m_name);
    m_width = json.value("width").toInt(m_width);
    m_height = json.value("height").toInt(m_height);
    if (json.contains("backgroundColor")) {
        m_backgroundColor = QColor(json.value("backgroundColor").toString());
    }

    emit idChanged(m_id);
    emit nameChanged(m_name);
    emit sizeChanged();
    emit backgroundColorChanged(m_backgroundColor);

    if (json.contains("components") && json.value("components").isArray()) {
        QJsonArray comps = json.value("components").toArray();
        if (!comps.isEmpty()) {
            beginInsertRows(QModelIndex(), 0, comps.size() - 1);
            for (int i = 0; i < comps.size(); ++i) {
                QJsonObject cObj = comps.at(i).toObject();
                auto w = new WidgetModel(this);
                w->fromJson(cObj);
                m_widgets.append(w);
                connectWidgetSignals(w);
            }
            endInsertRows();
            emit countChanged(m_widgets.size());
        }
    }

    return true;
}
