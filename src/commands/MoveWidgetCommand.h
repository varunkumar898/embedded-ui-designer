#pragma once

#include <QUndoCommand>

class WidgetModel;

/**
 * @brief Command Pattern: Encapsulates moving a widget on the canvas.
 * Implements mergeWith to collapse continuous drag updates into a single atomic undo action.
 */
class MoveWidgetCommand : public QUndoCommand {
public:
    enum { Id = 1001 };

    MoveWidgetCommand(WidgetModel* widget, qreal oldX, qreal oldY, qreal newX, qreal newY, QUndoCommand* parent = nullptr);

    int id() const override { return Id; }
    bool mergeWith(const QUndoCommand* other) override;

    void undo() override;
    void redo() override;

private:
    WidgetModel* m_widget = nullptr;
    qreal m_oldX = 0;
    qreal m_oldY = 0;
    qreal m_newX = 0;
    qreal m_newY = 0;
};
