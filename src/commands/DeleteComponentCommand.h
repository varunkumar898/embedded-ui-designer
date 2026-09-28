#pragma once

#include <QUndoCommand>
#include <QList>

class CanvasScene;
class UIComponent;

class DeleteComponentCommand : public QUndoCommand {
public:
    DeleteComponentCommand(CanvasScene* scene, const QList<UIComponent*>& comps, QUndoCommand* parent = nullptr);
    virtual ~DeleteComponentCommand() override;

    void undo() override;
    void redo() override;

private:
    CanvasScene* m_scene = nullptr;
    QList<UIComponent*> m_comps;
    bool m_ownsComponents = true;
};
