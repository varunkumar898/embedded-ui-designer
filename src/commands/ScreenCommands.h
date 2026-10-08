#pragma once

#include <QUndoCommand>
#include <QString>
#include "Project.h"
#include "Screen.h"

class CreateScreenCommand : public QUndoCommand {
public:
    CreateScreenCommand(Project* project, const QString& name, int width = 0, int height = 0, QUndoCommand* parent = nullptr);
    ~CreateScreenCommand() override;

    void undo() override;
    void redo() override;

    Screen* createdScreen() const { return m_screen; }

private:
    Project* m_project = nullptr;
    QString m_name;
    int m_width = 0;
    int m_height = 0;
    Screen* m_screen = nullptr;
    bool m_ownsScreen = false;
};

class DeleteScreenCommand : public QUndoCommand {
public:
    DeleteScreenCommand(Project* project, const QString& screenId, QUndoCommand* parent = nullptr);
    ~DeleteScreenCommand() override;

    void undo() override;
    void redo() override;

private:
    Project* m_project = nullptr;
    QString m_screenId;
    Screen* m_screen = nullptr;
    int m_originalIndex = -1;
    bool m_wasActive = false;
    bool m_ownsScreen = false;
};

class RenameScreenCommand : public QUndoCommand {
public:
    RenameScreenCommand(Project* project, const QString& screenId, const QString& oldName, const QString& newName, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    Project* m_project = nullptr;
    QString m_screenId;
    QString m_oldName;
    QString m_newName;
};

class DuplicateScreenCommand : public QUndoCommand {
public:
    DuplicateScreenCommand(Project* project, const QString& sourceScreenId, QUndoCommand* parent = nullptr);
    ~DuplicateScreenCommand() override;

    void undo() override;
    void redo() override;

    Screen* clonedScreen() const { return m_clonedScreen; }

private:
    Project* m_project = nullptr;
    QString m_sourceScreenId;
    Screen* m_clonedScreen = nullptr;
    bool m_ownsScreen = false;
};

class ReorderScreenCommand : public QUndoCommand {
public:
    ReorderScreenCommand(Project* project, int fromIndex, int toIndex, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    Project* m_project = nullptr;
    int m_fromIndex = -1;
    int m_toIndex = -1;
};
