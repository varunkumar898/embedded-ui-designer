#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDateTime>

namespace Git {

struct GitFileStatus {
    QString path;
    char indexStatus = ' ';
    char workTreeStatus = ' ';
    bool isStaged() const { return indexStatus != ' ' && indexStatus != '?'; }
    bool isModified() const { return workTreeStatus == 'M'; }
    bool isUntracked() const { return indexStatus == '?' && workTreeStatus == '?'; }
};

struct GitStatus {
    bool isRepo = false;
    QString currentBranch;
    int ahead = 0;
    int behind = 0;
    QList<GitFileStatus> files;
    bool isClean() const { return files.isEmpty(); }
};

struct GitCommit {
    QString hash;
    QString shortHash;
    QString message;
    QString author;
    QDateTime date;
};

class GitService : public QObject {
    Q_OBJECT

public:
    static GitService& instance();

    bool isGitInstalled() const;
    bool isGitRepository(const QString& repoPath) const;

    GitStatus getStatus(const QString& repoPath) const;
    QStringList getBranches(const QString& repoPath) const;
    QMap<QString, QString> getRemotes(const QString& repoPath) const;
    QList<GitCommit> getCommitHistory(const QString& repoPath, int count = 15) const;
    QString getDiff(const QString& repoPath, const QString& filePath = QString(), bool cached = false) const;

    // Controlled Mutations (Protected by Safety Policies)
    bool stageFile(const QString& repoPath, const QString& filePath, QString* errorOut = nullptr);
    bool stageAll(const QString& repoPath, QString* errorOut = nullptr);
    bool unstageFile(const QString& repoPath, const QString& filePath, QString* errorOut = nullptr);
    bool commit(const QString& repoPath, const QString& message, QString* errorOut = nullptr);

    // Remote operations
    bool fetch(const QString& repoPath, const QString& remote = "origin", QString* errorOut = nullptr);
    bool pull(const QString& repoPath, const QString& remote = "origin", const QString& branch = QString(), QString* errorOut = nullptr);
    bool push(const QString& repoPath, const QString& remote = "origin", const QString& branch = QString(), bool force = false, QString* errorOut = nullptr);

    // GitHub configuration
    bool configureRemote(const QString& repoPath, const QString& remoteName, const QString& url, QString* errorOut = nullptr);
    QString getGitHubToken() const;
    void setGitHubToken(const QString& token);

    // AI Git summary helper
    QString getAISummary(const QString& repoPath) const;

signals:
    void statusUpdated();
    void operationCompleted(const QString& opName, bool success, const QString& detail);

private:
    explicit GitService(QObject* parent = nullptr);
    ~GitService() override = default;
    GitService(const GitService&) = delete;
    GitService& operator=(const GitService&) = delete;

    bool runGitCommand(const QString& repoPath, const QStringList& args, QString* stdOut = nullptr, QString* stdErr = nullptr, int timeoutMs = 15000) const;
};

} // namespace Git
