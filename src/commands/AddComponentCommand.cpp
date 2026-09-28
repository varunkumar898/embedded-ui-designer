#include "AddComponentCommand.h"
#include "CanvasScene.h"
#include "UIComponent.h"

AddComponentCommand::AddComponentCommand(CanvasScene* scene, UIComponent* comp, QUndoCommand* parent)
    : QUndoCommand(QString("Add %1").arg(comp ? comp->componentType() : "Component"), parent)
    , m_scene(scene)
    , m_comp(comp)
    , m_ownsComponent(false)
{
}

AddComponentCommand::~AddComponentCommand() {
    if (m_ownsComponent && m_comp) {
        delete m_comp;
    }
}

void AddComponentCommand::undo() {
    if (m_scene && m_comp) {
        m_scene->removeUIComponent(m_comp);
        m_ownsComponent = true;
    }
}

void AddComponentCommand::redo() {
    if (m_scene && m_comp) {
        m_scene->addUIComponent(m_comp);
        m_scene->clearSelection();
        m_comp->setSelected(true);
        m_ownsComponent = false;
    }
}
