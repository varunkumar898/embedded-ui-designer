#pragma once

#include <QUndoCommand>

class CanvasScene;
class UIComponent;

class AddComponentCommand : public QUndoCommand {
public:
    AddComponentCommand(CanvasScene* scene, UIComponent* comp, QUndoCommand* parent = nullptr);
    virtual ~AddComponentCommand() override;

    void undo() override;
    void redo() override;

private:
    CanvasScene* m_scene = nullptr;
    UIComponent* m_comp = nullptr;
    bool m_ownsComponent = false;
};
