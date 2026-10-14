#include "GroupCommand.h"
#include <QUuid>
#include <algorithm>
#include <limits>

GroupCommand::GroupCommand(CanvasScene* scene, const QList<UIComponent*>& components, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_components(components)
{
    setText("Group Components");
    for (UIComponent* c : m_components) {
        if (c) m_originalScenePositions.append(c->pos());
    }
}

GroupCommand::~GroupCommand() {
    if (m_ownsGroup && m_group) {
        delete m_group;
    }
}

void GroupCommand::redo() {
    if (!m_scene || m_components.isEmpty()) return;

    if (!m_group) {
        QString gid = "group_" + QUuid::createUuid().toString(QUuid::WithoutBraces).left(6);
        m_group = new GroupComponent(gid);

        qreal minX = std::numeric_limits<qreal>::max();
        qreal minY = std::numeric_limits<qreal>::max();
        for (UIComponent* c : m_components) {
            if (c) {
                minX = std::min(minX, c->pos().x());
                minY = std::min(minY, c->pos().y());
            }
        }
        m_group->setCompPos(minX, minY);

        for (UIComponent* c : m_components) {
            if (c) {
                m_scene->removeUIComponent(c);
                c->setCompPos(c->pos().x() - minX, c->pos().y() - minY);
                m_group->addChild(c);
            }
        }
    } else {
        qreal gx = m_group->pos().x();
        qreal gy = m_group->pos().y();
        for (UIComponent* c : m_components) {
            if (c) {
                m_scene->removeUIComponent(c);
                c->setCompPos(c->pos().x() - gx, c->pos().y() - gy);
                m_group->addChild(c);
            }
        }
    }

    m_scene->addUIComponent(m_group);
    m_scene->clearSelection();
    m_group->setSelected(true);
    m_ownsGroup = false;
}

void GroupCommand::undo() {
    if (!m_scene || !m_group) return;

    qreal gx = m_group->pos().x();
    qreal gy = m_group->pos().y();

    m_scene->removeUIComponent(m_group);
    m_scene->clearSelection();

    for (int i = 0; i < m_components.size(); ++i) {
        UIComponent* c = m_components[i];
        if (c) {
            m_group->removeChild(c);
            c->setCompPos(gx + c->pos().x(), gy + c->pos().y());
            m_scene->addUIComponent(c);
            c->setSelected(true);
        }
    }
    m_ownsGroup = true;
}

// ── UngroupCommand ─────────────────────────────────────────────────────────

UngroupCommand::UngroupCommand(CanvasScene* scene, GroupComponent* group, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_scene(scene)
    , m_group(group)
{
    setText("Ungroup Components");
    if (m_group) {
        m_children = m_group->children();
        qreal gx = m_group->pos().x();
        qreal gy = m_group->pos().y();
        for (UIComponent* c : m_children) {
            if (c) m_scenePositions.append(QPointF(gx + c->pos().x(), gy + c->pos().y()));
        }
    }
}

UngroupCommand::~UngroupCommand() {
    if (m_ownsGroup && m_group) {
        delete m_group;
    }
}

void UngroupCommand::redo() {
    if (!m_scene || !m_group) return;

    qreal gx = m_group->pos().x();
    qreal gy = m_group->pos().y();

    m_scene->removeUIComponent(m_group);
    m_scene->clearSelection();

    for (int i = 0; i < m_children.size(); ++i) {
        UIComponent* c = m_children[i];
        if (c) {
            m_group->removeChild(c);
            c->setCompPos(gx + c->pos().x(), gy + c->pos().y());
            m_scene->addUIComponent(c);
            c->setSelected(true);
        }
    }
    m_ownsGroup = true;
}

void UngroupCommand::undo() {
    if (!m_scene || !m_group) return;

    qreal gx = m_group->pos().x();
    qreal gy = m_group->pos().y();

    for (UIComponent* c : m_children) {
        if (c) {
            m_scene->removeUIComponent(c);
            c->setCompPos(c->pos().x() - gx, c->pos().y() - gy);
            m_group->addChild(c);
        }
    }

    m_scene->addUIComponent(m_group);
    m_scene->clearSelection();
    m_group->setSelected(true);
    m_ownsGroup = false;
}
