#include "PropertyChangeCommand.h"
#include "UIComponent.h"

PropertyChangeCommand::PropertyChangeCommand(UIComponent* comp, const QJsonObject& oldState, const QJsonObject& newState, const QString& desc, QUndoCommand* parent)
    : QUndoCommand(desc, parent)
    , m_comp(comp)
    , m_oldState(oldState)
    , m_newState(newState)
{
}

void PropertyChangeCommand::undo() {
    if (m_comp) {
        m_comp->fromJson(m_oldState);
        emit m_comp->propertyChanged(m_comp);
    }
}

void PropertyChangeCommand::redo() {
    if (m_comp) {
        m_comp->fromJson(m_newState);
        emit m_comp->propertyChanged(m_comp);
    }
}
