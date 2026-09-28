#pragma once

#include <QUndoCommand>
#include <QPointF>
#include <QList>

class UIComponent;

class MoveComponentCommand : public QUndoCommand {
public:
    enum { Id = 1002 };

    struct MoveEntry {
        UIComponent* comp = nullptr;
        QPointF oldPos;
        QPointF newPos;
    };

    MoveComponentCommand(UIComponent* comp, const QPointF& oldPos, const QPointF& newPos, QUndoCommand* parent = nullptr);
    MoveComponentCommand(const QList<MoveEntry>& moves, QUndoCommand* parent = nullptr);

    int id() const override { return Id; }
    bool mergeWith(const QUndoCommand* other) override;

    void undo() override;
    void redo() override;

private:
    QList<MoveEntry> m_moves;
};
