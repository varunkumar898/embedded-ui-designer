#include "AlignDistributeCommand.h"

AlignDistributeCommand::AlignDistributeCommand(const QList<Entry>& entries,
                                                 const QString& description,
                                                 QUndoCommand* parent)
    : QUndoCommand(description, parent)
    , m_entries(entries)
{
}

void AlignDistributeCommand::undo() {
    for (const Entry& e : m_entries) {
        if (e.comp) {
            e.comp->setCompPos(e.oldPos.x(), e.oldPos.y());
        }
    }
}

void AlignDistributeCommand::redo() {
    for (const Entry& e : m_entries) {
        if (e.comp) {
            e.comp->setCompPos(e.newPos.x(), e.newPos.y());
        }
    }
}
