#include "ResizeComponentCommand.h"
#include "UIComponent.h"

ResizeComponentCommand::ResizeComponentCommand(UIComponent* comp, const QRectF& oldGeom, const QRectF& newGeom, QUndoCommand* parent)
    : QUndoCommand(QString("Resize %1").arg(comp ? comp->componentId() : "Component"), parent)
    , m_comp(comp)
    , m_oldGeom(oldGeom)
    , m_newGeom(newGeom)
{
}

void ResizeComponentCommand::undo() {
    if (m_comp) {
        m_comp->setCompPos(m_oldGeom.x(), m_oldGeom.y());
        m_comp->setCompSize(m_oldGeom.width(), m_oldGeom.height());
    }
}

void ResizeComponentCommand::redo() {
    if (m_comp) {
        m_comp->setCompPos(m_newGeom.x(), m_newGeom.y());
        m_comp->setCompSize(m_newGeom.width(), m_newGeom.height());
    }
}
