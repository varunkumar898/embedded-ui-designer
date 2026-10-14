#pragma once

#include <QUndoCommand>
#include <QList>
#include "UIComponent.h"
#include "GroupComponent.h"
#include "CanvasScene.h"

class GroupCommand : public QUndoCommand {
public:
    GroupCommand(CanvasScene* scene, const QList<UIComponent*>& components, QUndoCommand* parent = nullptr);
    ~GroupCommand() override;

    void undo() override;
    void redo() override;

private:
    CanvasScene* m_scene;
    QList<UIComponent*> m_components;
    GroupComponent* m_group = nullptr;
    QList<QPointF> m_originalScenePositions;
    bool m_ownsGroup = false;
};

class UngroupCommand : public QUndoCommand {
public:
    UngroupCommand(CanvasScene* scene, GroupComponent* group, QUndoCommand* parent = nullptr);
    ~UngroupCommand() override;

    void undo() override;
    void redo() override;

private:
    CanvasScene* m_scene;
    GroupComponent* m_group;
    QList<UIComponent*> m_children;
    QList<QPointF> m_scenePositions;
    bool m_ownsGroup = false;
};
