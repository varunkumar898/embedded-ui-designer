#include "MoveComponentCommand.h"
#include "UIComponent.h"

MoveComponentCommand::MoveComponentCommand(UIComponent* comp, const QPointF& oldPos, const QPointF& newPos, QUndoCommand* parent)
    : QUndoCommand(QString("Move %1").arg(comp ? comp->componentId() : "Component"), parent)
{
    m_moves.append({comp, oldPos, newPos});
}

MoveComponentCommand::MoveComponentCommand(const QList<MoveEntry>& moves, QUndoCommand* parent)
    : QUndoCommand(QString("Move %1 Component(s)").arg(moves.size()), parent)
    , m_moves(moves)
{
}

bool MoveComponentCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != Id) return false;
    const auto* nextMove = static_cast<const MoveComponentCommand*>(other);
    if (m_moves.size() != 1 || nextMove->m_moves.size() != 1) return false;
    if (m_moves[0].comp != nextMove->m_moves[0].comp) return false;

    m_moves[0].newPos = nextMove->m_moves[0].newPos;
    return true;
}

void MoveComponentCommand::undo() {
    for (const auto& entry : m_moves) {
        if (entry.comp) {
            entry.comp->setCompPos(entry.oldPos.x(), entry.oldPos.y());
        }
    }
}

void MoveComponentCommand::redo() {
    for (const auto& entry : m_moves) {
        if (entry.comp) {
            entry.comp->setCompPos(entry.newPos.x(), entry.newPos.y());
        }
    }
}
