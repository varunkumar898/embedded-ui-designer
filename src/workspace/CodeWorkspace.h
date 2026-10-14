#pragma once

#include "CodeWorkspaceView.h"
#include <QStringList>

class QMainWindow;
class Project;
class DeviceManager;

/**
 * @brief CodeWorkspace is the full firmware IDE workspace containing project explorer,
 * tabbed code editor with C/C++ syntax highlighting, terminal, serial monitor, and problems list.
 */
class CodeWorkspace : public CodeWorkspaceView {
    Q_OBJECT

public:
    explicit CodeWorkspace(QMainWindow* parent, Project* project);
    explicit CodeWorkspace(Project* project = nullptr, DeviceManager* deviceManager = nullptr, QWidget* parent = nullptr);
    ~CodeWorkspace() override = default;

    QWidget* getWidget() const { return const_cast<CodeWorkspace*>(this); }

    void saveState(QStringList& openFiles, int& activeFile);
    void restoreState(const QStringList& openFiles, int activeFile);
};
