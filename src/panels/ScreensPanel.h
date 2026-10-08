#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "Project.h"

class ScreensPanel : public QWidget {
    Q_OBJECT

public:
    explicit ScreensPanel(Project* project, QWidget* parent = nullptr);

    void setProject(Project* project);
    void setUndoStack(class QUndoStack* stack) { m_undoStack = stack; }

    void refreshScreenList();

signals:
    void activeScreenChanged(Screen* screen);

private slots:
    void onScreenSelected(QListWidgetItem* current, QListWidgetItem* previous);
    void onAddScreen();
    void onDeleteScreen();
    void onDuplicateScreen();
    void onMoveUp();
    void onMoveDown();
    void onScreenDoubleClicked(QListWidgetItem* item);
    void onScreenRenamed(QListWidgetItem* item);

private:
    void setupUi();

    Project* m_project = nullptr;
    class QUndoStack* m_undoStack = nullptr;

    QListWidget* m_listWidget = nullptr;
    QPushButton* m_btnAdd = nullptr;
    QPushButton* m_btnDelete = nullptr;
    QPushButton* m_btnDuplicate = nullptr;
    QPushButton* m_btnUp = nullptr;
    QPushButton* m_btnDown = nullptr;

    bool m_syncing = false;
};
