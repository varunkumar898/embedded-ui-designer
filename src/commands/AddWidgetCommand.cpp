#include "AddWidgetCommand.h"
#include "ScreenModel.h"
#include "WidgetModel.h"

AddWidgetCommand::AddWidgetCommand(ScreenModel* screen, WidgetModel* widget, int index, QUndoCommand* parent)
    : QUndoCommand(parent)
    , m_screen(screen)
    , m_widget(widget)
    , m_index(index)
{
    if (m_widget) {
        setText(QString("Add %1 (%2)").arg(m_widget->type(), m_widget->id()));
    } else {
        setText("Add Widget");
    }
}

AddWidgetCommand::~AddWidgetCommand() {
    if (!m_inScreen && m_widget) {
        delete m_widget;
    }
}

void AddWidgetCommand::redo() {
    if (!m_screen || !m_widget) return;
    m_screen->insertWidget(m_index, m_widget);
    m_inScreen = true;
}

void AddWidgetCommand::undo() {
    if (!m_screen || !m_widget) return;
    int actualIndex = m_screen->widgets().indexOf(m_widget);
    if (actualIndex != -1) {
        m_index = actualIndex;
        m_screen->takeWidget(actualIndex);
        m_inScreen = false;
    }
}
