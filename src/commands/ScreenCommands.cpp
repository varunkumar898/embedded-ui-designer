#include "ScreenCommands.h"

// ── CreateScreenCommand ──────────────────────────────────────────────────

CreateScreenCommand::CreateScreenCommand(Project* project, const QString& name, int width, int height, QUndoCommand* parent)
    : QUndoCommand(QString("Create Screen %1").arg(name), parent)
    , m_project(project)
    , m_name(name)
    , m_width(width)
    , m_height(height)
{
}

CreateScreenCommand::~CreateScreenCommand() {
    if (m_ownsScreen && m_screen) {
        delete m_screen;
    }
}

void CreateScreenCommand::undo() {
    if (m_project && m_screen) {
        m_project->takeScreen(m_screen->id());
        m_ownsScreen = true;
    }
}

void CreateScreenCommand::redo() {
    if (!m_project) return;
    if (!m_screen) {
        m_screen = m_project->addScreen(m_name, m_width, m_height);
        m_ownsScreen = false;
    } else {
        m_project->addScreen(m_screen);
        m_ownsScreen = false;
    }
}

// ── DeleteScreenCommand ──────────────────────────────────────────────────

DeleteScreenCommand::DeleteScreenCommand(Project* project, const QString& screenId, QUndoCommand* parent)
    : QUndoCommand(QString("Delete Screen %1").arg(screenId), parent)
    , m_project(project)
    , m_screenId(screenId)
{
    if (m_project) {
        m_screen = m_project->findScreen(screenId);
        if (m_screen) {
            m_originalIndex = m_project->screens().indexOf(m_screen);
            m_wasActive = (m_project->activeScreen() == m_screen);
        }
    }
}

DeleteScreenCommand::~DeleteScreenCommand() {
    if (m_ownsScreen && m_screen) {
        delete m_screen;
    }
}

void DeleteScreenCommand::undo() {
    if (m_project && m_screen) {
        m_project->addScreen(m_screen);
        if (m_originalIndex >= 0 && m_originalIndex < m_project->screens().size()) {
            int curIdx = m_project->screens().indexOf(m_screen);
            if (curIdx != m_originalIndex) {
                m_project->moveScreen(curIdx, m_originalIndex);
            }
        }
        if (m_wasActive) {
            m_project->setActiveScreen(m_screen);
        }
        m_ownsScreen = false;
    }
}

void DeleteScreenCommand::redo() {
    if (m_project) {
        m_screen = m_project->takeScreen(m_screenId);
        m_ownsScreen = (m_screen != nullptr);
    }
}

// ── RenameScreenCommand ──────────────────────────────────────────────────

RenameScreenCommand::RenameScreenCommand(Project* project, const QString& screenId, const QString& oldName, const QString& newName, QUndoCommand* parent)
    : QUndoCommand(QString("Rename Screen to %1").arg(newName), parent)
    , m_project(project)
    , m_screenId(screenId)
    , m_oldName(oldName)
    , m_newName(newName)
{
}

void RenameScreenCommand::undo() {
    if (m_project) {
        m_project->renameScreen(m_screenId, m_oldName);
    }
}

void RenameScreenCommand::redo() {
    if (m_project) {
        m_project->renameScreen(m_screenId, m_newName);
    }
}

// ── DuplicateScreenCommand ───────────────────────────────────────────────

DuplicateScreenCommand::DuplicateScreenCommand(Project* project, const QString& sourceScreenId, QUndoCommand* parent)
    : QUndoCommand(QString("Duplicate Screen"), parent)
    , m_project(project)
    , m_sourceScreenId(sourceScreenId)
{
}

DuplicateScreenCommand::~DuplicateScreenCommand() {
    if (m_ownsScreen && m_clonedScreen) {
        delete m_clonedScreen;
    }
}

void DuplicateScreenCommand::undo() {
    if (m_project && m_clonedScreen) {
        m_project->takeScreen(m_clonedScreen->id());
        m_ownsScreen = true;
    }
}

void DuplicateScreenCommand::redo() {
    if (!m_project) return;
    if (!m_clonedScreen) {
        m_clonedScreen = m_project->duplicateScreen(m_sourceScreenId);
        m_ownsScreen = false;
    } else {
        m_project->addScreen(m_clonedScreen);
        m_ownsScreen = false;
    }
}

// ── ReorderScreenCommand ─────────────────────────────────────────────────

ReorderScreenCommand::ReorderScreenCommand(Project* project, int fromIndex, int toIndex, QUndoCommand* parent)
    : QUndoCommand(QString("Reorder Screen"), parent)
    , m_project(project)
    , m_fromIndex(fromIndex)
    , m_toIndex(toIndex)
{
}

void ReorderScreenCommand::undo() {
    if (m_project) {
        m_project->moveScreen(m_toIndex, m_fromIndex);
    }
}

void ReorderScreenCommand::redo() {
    if (m_project) {
        m_project->moveScreen(m_fromIndex, m_toIndex);
    }
}
