#include "DeleteComponentCommand.h"
#include "CanvasScene.h"
#include "UIComponent.h"

DeleteComponentCommand::DeleteComponentCommand(CanvasScene* scene, const QList<UIComponent*>& comps, QUndoCommand* parent)
    : QUndoCommand(QString("Delete %1 Component(s)").arg(comps.size()), parent)
    , m_scene(scene)
    , m_comps(comps)
    , m_ownsComponents(false)
{
}

DeleteComponentCommand::~DeleteComponentCommand() {
    if (m_ownsComponents) {
        for (UIComponent* comp : m_comps) {
            delete comp;
        }
    }
}

void DeleteComponentCommand::undo() {
    if (m_scene) {
        m_scene->clearSelection();
        for (UIComponent* comp : m_comps) {
            m_scene->addUIComponent(comp);
            comp->setSelected(true);
        }
        m_ownsComponents = false;
    }
}

void DeleteComponentCommand::redo() {
    if (m_scene) {
        for (UIComponent* comp : m_comps) {
            m_scene->removeUIComponent(comp);
        }
        m_ownsComponents = true;
    }
}
