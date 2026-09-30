#pragma once

#include <QUndoCommand>
#include <QList>
#include <QPointF>
#include "UIComponent.h"

/**
 * AlignDistributeCommand
 *
 * Compound undo command that captures the old and new positions of every
 * component in the selection produced by an Align or Distribute operation.
 * The entire set of moves is undone / redone atomically in a single Ctrl+Z step.
 */
class AlignDistributeCommand : public QUndoCommand {
public:
    struct Entry {
        UIComponent* comp = nullptr;
        QPointF oldPos;
        QPointF newPos;
    };

    explicit AlignDistributeCommand(const QList<Entry>& entries,
                                     const QString& description,
                                     QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    QList<Entry> m_entries;
};
