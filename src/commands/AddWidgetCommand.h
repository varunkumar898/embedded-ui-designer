#pragma once

#include <QUndoCommand>

class ScreenModel;
class WidgetModel;

/**
 * @brief Command Pattern: Encapsulates adding a widget to a ScreenModel.
 * Supports safe Undo and Redo without memory leaks or dangling pointers.
 */
class AddWidgetCommand : public QUndoCommand {
public:
    AddWidgetCommand(ScreenModel* screen, WidgetModel* widget, int index = -1, QUndoCommand* parent = nullptr);
    virtual ~AddWidgetCommand() override;

    void undo() override;
    void redo() override;

private:
    ScreenModel* m_screen = nullptr;
    WidgetModel* m_widget = nullptr;
    int m_index = -1;
    bool m_inScreen = false;
};
