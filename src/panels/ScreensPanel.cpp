#include "ScreensPanel.h"
#include "ScreenCommands.h"
#include <QUndoStack>
#include <QInputDialog>
#include <QMessageBox>

ScreensPanel::ScreensPanel(Project* project, QWidget* parent)
    : QWidget(parent)
    , m_project(project)
{
    setupUi();
    if (m_project) {
        setProject(m_project);
    }
}

void ScreensPanel::setProject(Project* project) {
    if (m_project) {
        disconnect(m_project, nullptr, this, nullptr);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::screenListChanged, this, &ScreensPanel::refreshScreenList);
        connect(m_project, &Project::activeScreenChanged, this, [this](Screen*) {
            refreshScreenList();
        });
        connect(m_project, &Project::projectLoaded, this, &ScreensPanel::refreshScreenList);
    }
    refreshScreenList();
}

void ScreensPanel::setupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Header Toolbar
    auto toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(4);

    m_btnAdd = new QPushButton("+ Add", this);
    m_btnAdd->setToolTip("Create a new screen");
    m_btnAdd->setStyleSheet("QPushButton { font-weight: bold; background-color: #2563EB; color: white; border-radius: 4px; padding: 4px 8px; }"
                            "QPushButton:hover { background-color: #1D4ED8; }");

    m_btnDuplicate = new QPushButton("Clone", this);
    m_btnDuplicate->setToolTip("Duplicate selected screen");
    m_btnDuplicate->setStyleSheet("QPushButton { background-color: #374151; color: white; border-radius: 4px; padding: 4px 8px; }"
                                  "QPushButton:hover { background-color: #4B5563; }");

    m_btnDelete = new QPushButton("Delete", this);
    m_btnDelete->setToolTip("Delete selected screen");
    m_btnDelete->setStyleSheet("QPushButton { background-color: #7F1D1D; color: #FCA5A5; border-radius: 4px; padding: 4px 8px; }"
                               "QPushButton:hover { background-color: #991B1B; color: white; }"
                               "QPushButton:disabled { background-color: #2D333F; color: #555E6D; }");

    m_btnUp = new QPushButton("▲", this);
    m_btnUp->setToolTip("Move screen up");
    m_btnUp->setFixedWidth(28);

    m_btnDown = new QPushButton("▼", this);
    m_btnDown->setToolTip("Move screen down");
    m_btnDown->setFixedWidth(28);

    toolbarLayout->addWidget(m_btnAdd);
    toolbarLayout->addWidget(m_btnDuplicate);
    toolbarLayout->addWidget(m_btnDelete);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_btnUp);
    toolbarLayout->addWidget(m_btnDown);

    mainLayout->addLayout(toolbarLayout);

    // List of screens
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setStyleSheet(
        "QListWidget { background-color: #1A1C23; border: 1px solid #2D3342; border-radius: 6px; color: #E2E8F0; font-size: 12px; }"
        "QListWidget::item { padding: 8px 10px; border-bottom: 1px solid #222632; border-radius: 4px; margin: 2px 4px; }"
        "QListWidget::item:hover { background-color: #262B3B; }"
        "QListWidget::item:selected { background-color: #2563EB; color: white; font-weight: bold; }"
    );
    mainLayout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::currentItemChanged, this, &ScreensPanel::onScreenSelected);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &ScreensPanel::onScreenDoubleClicked);
    connect(m_listWidget, &QListWidget::itemChanged, this, &ScreensPanel::onScreenRenamed);

    connect(m_btnAdd, &QPushButton::clicked, this, &ScreensPanel::onAddScreen);
    connect(m_btnDelete, &QPushButton::clicked, this, &ScreensPanel::onDeleteScreen);
    connect(m_btnDuplicate, &QPushButton::clicked, this, &ScreensPanel::onDuplicateScreen);
    connect(m_btnUp, &QPushButton::clicked, this, &ScreensPanel::onMoveUp);
    connect(m_btnDown, &QPushButton::clicked, this, &ScreensPanel::onMoveDown);
}

void ScreensPanel::refreshScreenList() {
    if (!m_project || m_syncing) return;
    m_syncing = true;

    m_listWidget->clear();

    const auto& screens = m_project->screens();
    Screen* active = m_project->activeScreen();
    int activeRow = -1;

    for (int i = 0; i < screens.size(); ++i) {
        Screen* s = screens[i];
        QString itemText = QString("%1 (%2x%3 - %4 items)")
                               .arg(s->name())
                               .arg(s->width())
                               .arg(s->height())
                               .arg(s->components().size());

        auto item = new QListWidgetItem(itemText, m_listWidget);
        item->setData(Qt::UserRole, s->id());
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        if (s == active) {
            activeRow = i;
        }
    }

    if (activeRow >= 0 && activeRow < m_listWidget->count()) {
        m_listWidget->setCurrentRow(activeRow);
    }

    m_btnDelete->setEnabled(screens.size() > 1);

    m_syncing = false;
}

void ScreensPanel::onScreenSelected(QListWidgetItem* current, QListWidgetItem* previous) {
    Q_UNUSED(previous);
    if (!current || !m_project || m_syncing) return;

    QString screenId = current->data(Qt::UserRole).toString();
    Screen* s = m_project->findScreen(screenId);
    if (s && s != m_project->activeScreen()) {
        m_project->setActiveScreen(s);
        emit activeScreenChanged(s);
    }
}

void ScreensPanel::onAddScreen() {
    if (!m_project) return;
    if (m_undoStack) {
        m_undoStack->push(new CreateScreenCommand(m_project, ""));
    } else {
        m_project->addScreen();
    }
}

void ScreensPanel::onDeleteScreen() {
    if (!m_project || m_project->screens().size() <= 1) return;

    QListWidgetItem* current = m_listWidget->currentItem();
    if (!current) return;
    QString screenId = current->data(Qt::UserRole).toString();

    if (m_undoStack) {
        m_undoStack->push(new DeleteScreenCommand(m_project, screenId));
    } else {
        m_project->removeScreen(screenId);
    }
}

void ScreensPanel::onDuplicateScreen() {
    if (!m_project) return;

    QListWidgetItem* current = m_listWidget->currentItem();
    if (!current) return;
    QString screenId = current->data(Qt::UserRole).toString();

    if (m_undoStack) {
        m_undoStack->push(new DuplicateScreenCommand(m_project, screenId));
    } else {
        m_project->duplicateScreen(screenId);
    }
}

void ScreensPanel::onMoveUp() {
    if (!m_project) return;
    int row = m_listWidget->currentRow();
    if (row > 0) {
        if (m_undoStack) {
            m_undoStack->push(new ReorderScreenCommand(m_project, row, row - 1));
        } else {
            m_project->moveScreen(row, row - 1);
        }
        m_listWidget->setCurrentRow(row - 1);
    }
}

void ScreensPanel::onMoveDown() {
    if (!m_project) return;
    int row = m_listWidget->currentRow();
    if (row >= 0 && row < m_listWidget->count() - 1) {
        if (m_undoStack) {
            m_undoStack->push(new ReorderScreenCommand(m_project, row, row + 1));
        } else {
            m_project->moveScreen(row, row + 1);
        }
        m_listWidget->setCurrentRow(row + 1);
    }
}

void ScreensPanel::onScreenDoubleClicked(QListWidgetItem* item) {
    if (!item || !m_project) return;
    m_listWidget->editItem(item);
}

void ScreensPanel::onScreenRenamed(QListWidgetItem* item) {
    if (!item || !m_project || m_syncing) return;
    QString screenId = item->data(Qt::UserRole).toString();
    Screen* s = m_project->findScreen(screenId);
    if (!s) return;

    QString newRawText = item->text();
    // Strip the appended stats if present
    int idx = newRawText.indexOf(" (");
    QString newName = (idx > 0) ? newRawText.left(idx).trimmed() : newRawText.trimmed();
    if (newName.isEmpty()) newName = s->name();

    if (newName != s->name()) {
        if (m_undoStack) {
            m_undoStack->push(new RenameScreenCommand(m_project, screenId, s->name(), newName));
        } else {
            m_project->renameScreen(screenId, newName);
        }
    }
}
