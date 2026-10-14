#include "GroupComponent.h"
#include "Project.h"
#include <QPainter>
#include <limits>
#include <algorithm>

GroupComponent::GroupComponent(const QString& id, QGraphicsItem* parent)
    : UIComponent(id, "Group", parent)
{
    m_width = 100;
    m_height = 100;
}

GroupComponent::~GroupComponent() {
    clearChildren();
}

void GroupComponent::addChild(UIComponent* child) {
    if (!child || m_children.contains(child)) return;
    m_children.append(child);
    connect(child, &UIComponent::geometryChangedSignal, this, &GroupComponent::recalculateBounds);
    connect(child, &UIComponent::propertyChanged, this, [this]() {
        update();
        emit propertyChanged(this);
    });
    recalculateBounds();
    emit propertyChanged(this);
}

void GroupComponent::removeChild(UIComponent* child) {
    if (!child) return;
    if (m_children.removeAll(child) > 0) {
        disconnect(child, nullptr, this, nullptr);
        recalculateBounds();
        emit propertyChanged(this);
    }
}

void GroupComponent::clearChildren() {
    for (UIComponent* child : m_children) {
        if (child) {
            disconnect(child, nullptr, this, nullptr);
            delete child;
        }
    }
    m_children.clear();
}

void GroupComponent::recalculateBounds() {
    if (m_children.isEmpty()) return;

    qreal minX = std::numeric_limits<qreal>::max();
    qreal minY = std::numeric_limits<qreal>::max();
    qreal maxX = std::numeric_limits<qreal>::lowest();
    qreal maxY = std::numeric_limits<qreal>::lowest();

    for (UIComponent* c : m_children) {
        if (!c) continue;
        minX = std::min(minX, c->pos().x());
        minY = std::min(minY, c->pos().y());
        maxX = std::max(maxX, c->pos().x() + c->compWidth());
        maxY = std::max(maxY, c->pos().y() + c->compHeight());
    }

    if (minX <= maxX && minY <= maxY) {
        qreal w = std::max(10.0, maxX - minX);
        qreal h = std::max(10.0, maxY - minY);
        if (m_width != w || m_height != h) {
            prepareGeometryChange();
            m_width = w;
            m_height = h;
            setTransformOriginPoint(QPointF(w / 2.0, h / 2.0));
            update();
        }
    }
}

void GroupComponent::paintComponent(QPainter* painter) {
    for (UIComponent* child : m_children) {
        if (!child || !child->isVisible()) continue;
        painter->save();
        painter->translate(child->pos().x(), child->pos().y());
        if (child->rotationAngle() != 0.0) {
            painter->translate(child->compWidth() / 2.0, child->compHeight() / 2.0);
            painter->rotate(child->rotationAngle());
            painter->translate(-child->compWidth() / 2.0, -child->compHeight() / 2.0);
        }
        if (child->flipHorizontal() || child->flipVertical()) {
            painter->translate(child->compWidth() / 2.0, child->compHeight() / 2.0);
            painter->scale(child->flipHorizontal() ? -1.0 : 1.0, child->flipVertical() ? -1.0 : 1.0);
            painter->translate(-child->compWidth() / 2.0, -child->compHeight() / 2.0);
        }
        child->paint(painter, nullptr, nullptr);
        painter->restore();
    }
}

QJsonObject GroupComponent::toJson() const {
    QJsonObject obj = UIComponent::toJson();
    QJsonArray childrenArr;
    for (UIComponent* child : m_children) {
        if (child) {
            childrenArr.append(child->toJson());
        }
    }
    obj["children"] = childrenArr;
    return obj;
}

void GroupComponent::fromJson(const QJsonObject& json) {
    UIComponent::fromJson(json);
    clearChildren();
    if (json.contains("children") && json.value("children").isArray()) {
        QJsonArray arr = json.value("children").toArray();
        for (const QJsonValue& v : arr) {
            if (v.isObject()) {
                QJsonObject childObj = v.toObject();
                QString type = childObj.value("type").toString();
                QString id = childObj.value("id").toString();
                UIComponent* child = Project::createComponentInstance(type, id);
                if (child) {
                    child->fromJson(childObj);
                    addChild(child);
                }
            }
        }
    }
}

QString GroupComponent::toQmlSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString qml;
    qml += QString("%1Item {\n").arg(indent);
    qml += QString("%1    id: %2\n").arg(indent, m_id);
    qml += QString("%1    x: %2; y: %3\n").arg(indent).arg(static_cast<int>(pos().x())).arg(static_cast<int>(pos().y()));
    qml += QString("%1    width: %2; height: %3\n").arg(indent).arg(static_cast<int>(m_width)).arg(static_cast<int>(m_height));
    for (UIComponent* child : m_children) {
        if (child) {
            qml += child->toQmlSnippet(indentSpaces + 4);
        }
    }
    qml += QString("%1}\n").arg(indent);
    return qml;
}

QString GroupComponent::toUgfxSnippet(int indentSpaces) const {
    QString indent(indentSpaces, ' ');
    QString code;
    code += QString("%1// Group: %2\n").arg(indent, m_id);
    for (UIComponent* child : m_children) {
        if (child) {
            code += child->toUgfxSnippet(indentSpaces);
        }
    }
    return code;
}
