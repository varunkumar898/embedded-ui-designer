#pragma once

#include <QUndoCommand>
#include <QJsonObject>

class UIComponent;

class PropertyChangeCommand : public QUndoCommand {
public:
    PropertyChangeCommand(UIComponent* comp, const QJsonObject& oldState, const QJsonObject& newState, const QString& desc = "Change Property", QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    UIComponent* m_comp = nullptr;
    QJsonObject m_oldState;
    QJsonObject m_newState;
};
