#include "CodeWorkspace.h"
#include "TabbedCodeEditor.h"
#include "CodeEditor.h"
#include "Project.h"
#include <QMainWindow>

CodeWorkspace::CodeWorkspace(QMainWindow* parent, Project* project)
    : CodeWorkspaceView(project, nullptr, parent)
{
}

CodeWorkspace::CodeWorkspace(Project* project, DeviceManager* deviceManager, QWidget* parent)
    : CodeWorkspaceView(project, deviceManager, parent)
{
}

void CodeWorkspace::saveState(QStringList& openFiles, int& activeFile) {
    openFiles.clear();
    activeFile = -1;
    if (tabbedEditor()) {
        for (int i = 0; i < tabbedEditor()->openFileCount(); ++i) {
            CodeEditor* ed = tabbedEditor()->editorAt(i);
            if (ed && !ed->filePath().isEmpty()) {
                openFiles.append(ed->filePath());
            }
        }
        activeFile = tabbedEditor()->currentIndex();
    }
}

void CodeWorkspace::restoreState(const QStringList& openFiles, int activeFile) {
    if (tabbedEditor()) {
        for (const QString& filePath : openFiles) {
            tabbedEditor()->openFile(filePath);
        }
        if (activeFile >= 0 && activeFile < tabbedEditor()->openFileCount()) {
            tabbedEditor()->gotoLocation(tabbedEditor()->currentFilePath(), 1, 1);
        }
    }
}
