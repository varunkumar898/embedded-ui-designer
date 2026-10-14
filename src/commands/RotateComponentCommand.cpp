#include "RotateComponentCommand.h"
#include "UIComponent.h"

RotateComponentCommand::RotateComponentCommand(UIComponent* comp, qreal oldAngle, qreal newAngle, QUndoCommand* parent)
    : QUndoCommand(QString("Rotate %1").arg(comp ? comp->componentId() : "Component"), parent)
    , m_comp(comp)
    , m_oldAngle(oldAngle)
    , m_newAngle(newAngle)
{
}

void RotateComponentCommand::undo() {
    if (m_comp) {
        m_comp->setRotationAngle(m_oldAngle);
    }
}

void RotateComponentCommand::redo() {
    if (m_comp) {
        m_comp->setRotationAngle(m_newAngle);
    }
}
