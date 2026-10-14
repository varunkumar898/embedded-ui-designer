#pragma once

#include <QUndoCommand>

class UIComponent;

class RotateComponentCommand : public QUndoCommand {
public:
    RotateComponentCommand(UIComponent* comp, qreal oldAngle, qreal newAngle, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    UIComponent* m_comp = nullptr;
    qreal m_oldAngle = 0.0;
    qreal m_newAngle = 0.0;
};
