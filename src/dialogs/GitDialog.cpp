#include "GitDialog.h"
#include "git/GitService.h"
#include "theme/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QSplitter>
#include <QGroupBox>

namespace Git {

GitDialog::GitDialog(const QString& repoPath, QWidget* parent)
    : QDialog(parent)
    , m_repoPath(repoPath)
{
    setWindowTitle("Git & GitHub Integration");
    resize(860, 640);
    setupUI();
    refreshStatus();
}

void GitDialog::setupUI() {
    auto mainLayout = new QVBoxLayout(this);

    auto tabWidget = new QTabWidget(this);

    // -------------------------------------------------------------
    // Tab 1: Working Tree & Commit
    // -------------------------------------------------------------
    auto workingTreeWidget = new QWidget(this);
    auto wtLayout = new QVBoxLayout(workingTreeWidget);

    // Header info bar
    auto headerBox = new QGroupBox("Repository Status", workingTreeWidget);
    auto headerLayout = new QHBoxLayout(headerBox);
    m_branchLabel = new QLabel("Branch: checking...", headerBox);
    m_syncLabel = new QLabel("Sync: ...", headerBox);
    headerLayout->addWidget(m_branchLabel);
    headerLayout->addSpacing(20);
    headerLayout->addWidget(m_syncLabel);
    headerLayout->addStretch();

    m_fetchButton = new QPushButton("Fetch", headerBox);
    m_pullButton = new QPushButton("Pull", headerBox);
    m_pushButton = new QPushButton("Push", headerBox);
    connect(m_fetchButton, &QPushButton::clicked, this, &GitDialog::onFetch);
    connect(m_pullButton, &QPushButton::clicked, this, &GitDialog::onPull);
    connect(m_pushButton, &QPushButton::clicked, this, &GitDialog::onPush);

    headerLayout->addWidget(m_fetchButton);
    headerLayout->addWidget(m_pullButton);
    headerLayout->addWidget(m_pushButton);
    wtLayout->addWidget(headerBox);

    // Splitter for files and diff
    auto mainSplitter = new QSplitter(Qt::Horizontal, workingTreeWidget);

    // Left pane: Staged & Unstaged lists
    auto leftPane = new QWidget(mainSplitter);
    auto leftLayout = new QVBoxLayout(leftPane);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // Unstaged
    leftLayout->addWidget(new QLabel("Unstaged Changes:", leftPane));
    m_unstagedList = new QListWidget(leftPane);
    m_unstagedList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_unstagedList, &QListWidget::itemSelectionChanged, this, &GitDialog::onFileSelectionChanged);
    leftLayout->addWidget(m_unstagedList);

    auto btnRow1 = new QHBoxLayout();
    auto stageBtn = new QPushButton("Stage Selected", leftPane);
    auto stageAllBtn = new QPushButton("Stage All", leftPane);
    connect(stageBtn, &QPushButton::clicked, this, &GitDialog::onStageSelected);
    connect(stageAllBtn, &QPushButton::clicked, this, &GitDialog::onStageAll);
    btnRow1->addWidget(stageBtn);
    btnRow1->addWidget(stageAllBtn);
    leftLayout->addLayout(btnRow1);

    // Staged
    leftLayout->addWidget(new QLabel("Staged Changes:", leftPane));
    m_stagedList = new QListWidget(leftPane);
    m_stagedList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(m_stagedList, &QListWidget::itemSelectionChanged, this, &GitDialog::onFileSelectionChanged);
    leftLayout->addWidget(m_stagedList);

    auto btnRow2 = new QHBoxLayout();
    auto unstageBtn = new QPushButton("Unstage Selected", leftPane);
    connect(unstageBtn, &QPushButton::clicked, this, &GitDialog::onUnstageSelected);
    btnRow2->addWidget(unstageBtn);
    leftLayout->addLayout(btnRow2);

    mainSplitter->addWidget(leftPane);

    // Right pane: Diff Viewer & Commit box
    auto rightPane = new QWidget(mainSplitter);
    auto rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    rightLayout->addWidget(new QLabel("Diff Preview:", rightPane));
    m_diffViewer = new QTextEdit(rightPane);
    m_diffViewer->setReadOnly(true);
    QFont font("Monospace", 9);
    font.setStyleHint(QFont::TypeWriter);
    m_diffViewer->setFont(font);
    rightLayout->addWidget(m_diffViewer);

    rightLayout->addWidget(new QLabel("Commit Message:", rightPane));
    m_commitMessageEdit = new QTextEdit(rightPane);
    m_commitMessageEdit->setMaximumHeight(80);
    m_commitMessageEdit->setPlaceholderText("Enter a descriptive commit message...");
    rightLayout->addWidget(m_commitMessageEdit);

    m_commitButton = new QPushButton("Commit Changes", rightPane);
    m_commitButton->setStyleSheet(QString("QPushButton { font-weight: bold; background-color: %1; color: %2; padding: 6px; border-radius: %3px; }").arg(Theme::Accent, Theme::BgBase, QString::number(Theme::RadiusSm)));
    connect(m_commitButton, &QPushButton::clicked, this, &GitDialog::onCommit);
    rightLayout->addWidget(m_commitButton);

    mainSplitter->addWidget(rightPane);
    mainSplitter->setStretchFactor(0, 1);
    mainSplitter->setStretchFactor(1, 2);

    wtLayout->addWidget(mainSplitter);
    tabWidget->addTab(workingTreeWidget, "Working Tree & Commit");

    // -------------------------------------------------------------
    // Tab 2: Commit History
    // -------------------------------------------------------------
    auto historyWidget = new QWidget(this);
    auto histLayout = new QVBoxLayout(historyWidget);
    m_historyTable = new QTableWidget(0, 4, historyWidget);
    m_historyTable->setHorizontalHeaderLabels(QStringList() << "Hash" << "Author" << "Date" << "Message");
    m_historyTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_historyTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    histLayout->addWidget(m_historyTable);
    tabWidget->addTab(historyWidget, "Commit History");

    // -------------------------------------------------------------
    // Tab 3: GitHub Settings
    // -------------------------------------------------------------
    auto githubWidget = new QWidget(this);
    auto ghLayout = new QVBoxLayout(githubWidget);

    auto ghBox = new QGroupBox("GitHub Remote & Credentials", githubWidget);
    auto ghForm = new QGridLayout(ghBox);

    ghForm->addWidget(new QLabel("Remote URL (origin):", ghBox), 0, 0);
    m_remoteUrlEdit = new QLineEdit(ghBox);
    ghForm->addWidget(m_remoteUrlEdit, 0, 1);

    ghForm->addWidget(new QLabel("Personal Access Token:", ghBox), 1, 0);
    m_tokenEdit = new QLineEdit(ghBox);
    m_tokenEdit->setEchoMode(QLineEdit::Password);
    m_tokenEdit->setPlaceholderText("ghp_... (stored encrypted in CredentialManager)");
    ghForm->addWidget(m_tokenEdit, 1, 1);

    auto saveGhBtn = new QPushButton("Save GitHub Settings", ghBox);
    connect(saveGhBtn, &QPushButton::clicked, this, &GitDialog::onSaveGitHubConfig);
    ghForm->addWidget(saveGhBtn, 2, 1);

    m_githubStatusLabel = new QLabel("", ghBox);
    ghForm->addWidget(m_githubStatusLabel, 3, 1);

    ghLayout->addWidget(ghBox);
    ghLayout->addStretch();
    tabWidget->addTab(githubWidget, "GitHub Settings");

    mainLayout->addWidget(tabWidget);

    auto closeRow = new QHBoxLayout();
    closeRow->addStretch();
    auto refreshBtn = new QPushButton("Refresh Status", this);
    connect(refreshBtn, &QPushButton::clicked, this, &GitDialog::refreshStatus);
    auto closeBtn = new QPushButton("Close", this);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeRow->addWidget(refreshBtn);
    closeRow->addWidget(closeBtn);
    mainLayout->addLayout(closeRow);
}

void GitDialog::refreshStatus() {
    auto& git = GitService::instance();
    if (!git.isGitRepository(m_repoPath)) {
        m_branchLabel->setText("Not a Git repository.");
        m_syncLabel->setText("");
        m_stagedList->clear();
        m_unstagedList->clear();
        m_commitButton->setEnabled(false);
        m_pushButton->setEnabled(false);
        m_pullButton->setEnabled(false);
        m_fetchButton->setEnabled(false);
        return;
    }

    GitStatus s = git.getStatus(m_repoPath);
    m_branchLabel->setText(QString("Branch: <b>%1</b>").arg(s.currentBranch));
    m_syncLabel->setText(QString("Sync: +%1 / -%2").arg(s.ahead).arg(s.behind));

    m_stagedList->clear();
    m_unstagedList->clear();

    for (const auto& file : s.files) {
        if (file.isStaged()) {
            m_stagedList->addItem(file.path);
        }
        if (file.isModified() || file.isUntracked()) {
            m_unstagedList->addItem(file.path);
        }
    }

    m_commitButton->setEnabled(m_stagedList->count() > 0);
    m_pushButton->setEnabled(true);
    m_pullButton->setEnabled(true);
    m_fetchButton->setEnabled(true);

    updateHistoryTab();

    // Load GitHub settings
    auto remotes = git.getRemotes(m_repoPath);
    if (remotes.contains("origin")) {
        m_remoteUrlEdit->setText(remotes["origin"]);
    }
    m_tokenEdit->setText(git.getGitHubToken());
}

void GitDialog::updateHistoryTab() {
    auto& git = GitService::instance();
    auto history = git.getCommitHistory(m_repoPath, 20);
    m_historyTable->setRowCount(history.size());
    for (int i = 0; i < history.size(); ++i) {
        const auto& c = history[i];
        m_historyTable->setItem(i, 0, new QTableWidgetItem(c.shortHash));
        m_historyTable->setItem(i, 1, new QTableWidgetItem(c.author));
        m_historyTable->setItem(i, 2, new QTableWidgetItem(c.date.toString("yyyy-MM-dd hh:mm")));
        m_historyTable->setItem(i, 3, new QTableWidgetItem(c.message));
    }
}

void GitDialog::onStageSelected() {
    auto items = m_unstagedList->selectedItems();
    if (items.isEmpty()) return;
    QString err;
    for (auto* item : items) {
        GitService::instance().stageFile(m_repoPath, item->text(), &err);
    }
    refreshStatus();
}

void GitDialog::onUnstageSelected() {
    auto items = m_stagedList->selectedItems();
    if (items.isEmpty()) return;
    QString err;
    for (auto* item : items) {
        GitService::instance().unstageFile(m_repoPath, item->text(), &err);
    }
    refreshStatus();
}

void GitDialog::onStageAll() {
    QString err;
    GitService::instance().stageAll(m_repoPath, &err);
    refreshStatus();
}

void GitDialog::onFileSelectionChanged() {
    QString filePath;
    bool cached = false;
    if (m_stagedList->currentItem() && m_stagedList->hasFocus()) {
        filePath = m_stagedList->currentItem()->text();
        cached = true;
    } else if (m_unstagedList->currentItem()) {
        filePath = m_unstagedList->currentItem()->text();
        cached = false;
    }

    if (!filePath.isEmpty()) {
        m_diffViewer->setPlainText(GitService::instance().getDiff(m_repoPath, filePath, cached));
    }
}

void GitDialog::onCommit() {
    QString msg = m_commitMessageEdit->toPlainText().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, "Commit Warning", "Please enter a commit message before committing.");
        return;
    }

    // Phase 6O: Require explicit confirmation for commit
    auto reply = QMessageBox::question(this, "Confirm Commit",
        QString("Are you sure you want to commit %1 staged file(s)?\n\nMessage: \"%2\"")
            .arg(m_stagedList->count()).arg(msg),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    QString err;
    if (GitService::instance().commit(m_repoPath, msg, &err)) {
        m_commitMessageEdit->clear();
        refreshStatus();
        QMessageBox::information(this, "Commit Succeeded", "Changes committed successfully.");
    } else {
        QMessageBox::critical(this, "Commit Failed", "Failed to commit changes: " + err);
    }
}

void GitDialog::onPush() {
    // Phase 6O: Require explicit user confirmation for push
    auto reply = QMessageBox::question(this, "Confirm Push",
        "Are you sure you want to push current branch commits to the remote repository?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    QString err;
    if (GitService::instance().push(m_repoPath, "origin", QString(), false, &err)) {
        refreshStatus();
        QMessageBox::information(this, "Push Succeeded", "Commits pushed successfully to origin.");
    } else {
        QMessageBox::critical(this, "Push Failed", "Failed to push to remote: " + err);
    }
}

void GitDialog::onPull() {
    QString err;
    if (GitService::instance().pull(m_repoPath, "origin", QString(), &err)) {
        refreshStatus();
        QMessageBox::information(this, "Pull Succeeded", "Pulled latest changes from origin.");
    } else {
        QMessageBox::critical(this, "Pull Failed", "Failed to pull from remote: " + err);
    }
}

void GitDialog::onFetch() {
    QString err;
    if (GitService::instance().fetch(m_repoPath, "origin", &err)) {
        refreshStatus();
    } else {
        QMessageBox::critical(this, "Fetch Failed", "Failed to fetch from remote: " + err);
    }
}

void GitDialog::onSaveGitHubConfig() {
    QString url = m_remoteUrlEdit->text().trimmed();
    QString token = m_tokenEdit->text().trimmed();

    if (!url.isEmpty()) {
        QString err;
        GitService::instance().configureRemote(m_repoPath, "origin", url, &err);
    }

    GitService::instance().setGitHubToken(token);
    m_githubStatusLabel->setText("<font color='green'>GitHub settings saved securely.</font>");
}

} // namespace Git
