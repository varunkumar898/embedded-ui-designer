#pragma once

#include <QDialog>
#include <QString>

class QListWidget;
class QTextEdit;
class QLineEdit;
class QPushButton;
class QLabel;
class QTabWidget;
class QTableWidget;

namespace Git {

class GitDialog : public QDialog {
    Q_OBJECT

public:
    explicit GitDialog(const QString& repoPath, QWidget* parent = nullptr);
    ~GitDialog() override = default;

private slots:
    void refreshStatus();
    void onStageSelected();
    void onUnstageSelected();
    void onStageAll();
    void onCommit();
    void onPush();
    void onPull();
    void onFetch();
    void onFileSelectionChanged();
    void onSaveGitHubConfig();

private:
    void setupUI();
    void updateHistoryTab();

    QString m_repoPath;

    QLabel* m_branchLabel = nullptr;
    QLabel* m_syncLabel = nullptr;
    QListWidget* m_stagedList = nullptr;
    QListWidget* m_unstagedList = nullptr;
    QTextEdit* m_diffViewer = nullptr;
    QTextEdit* m_commitMessageEdit = nullptr;
    QPushButton* m_commitButton = nullptr;
    QPushButton* m_pushButton = nullptr;
    QPushButton* m_pullButton = nullptr;
    QPushButton* m_fetchButton = nullptr;

    // History Tab
    QTableWidget* m_historyTable = nullptr;

    // GitHub Settings Tab
    QLineEdit* m_remoteUrlEdit = nullptr;
    QLineEdit* m_tokenEdit = nullptr;
    QLabel* m_githubStatusLabel = nullptr;
};

} // namespace Git
