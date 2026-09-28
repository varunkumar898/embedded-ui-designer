#include "MoveWidgetCommand.h"
#include "WidgetModel.h"

MoveWidgetCommand::MoveWidgetCommand(WidgetModel* widget, qreal oldX, qreal oldY, qreal newX, qreal newY, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_widget(widget)
    , m_oldX(oldX)
    , m_oldY(oldY)
    , m_newX(newX)
    , m_newY(newY)
{
    if (m_widget) {
        setText(QString("Move %1").arg(m_widget->id()));
    } else {
        setText("Move Widget");
    }
}

bool MoveWidgetCommand::mergeWith(const QUndoCommand* other) {
    if (other->id() != Id) return false;
    const auto* moveCmd = static_cast<const MoveWidgetCommand*>(other);
    if (moveCmd->m_widget != m_widget) return false;

    // Update target coordinate to the newer command's destination while keeping original start pos
    m_newX = moveCmd->m_newX;
    m_newY = moveCmd->m_newY;
    return true;
}

void MoveWidgetCommand::redo() {
    if (m_widget) {
        m_widget->setX(m_newX);
        m_widget->setY(m_newY);
    }
}

void MoveWidgetCommand::undo() {
    if (m_widget) {
        m_widget->setX(m_oldX);
        m_widget->setY(m_oldY);
    }
}
