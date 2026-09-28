#pragma once

#include <QUndoCommand>
#include <QRectF>

class UIComponent;

class ResizeComponentCommand : public QUndoCommand {
public:
    ResizeComponentCommand(UIComponent* comp, const QRectF& oldGeom, const QRectF& newGeom, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    UIComponent* m_comp = nullptr;
    QRectF m_oldGeom;
    QRectF m_newGeom;
};
