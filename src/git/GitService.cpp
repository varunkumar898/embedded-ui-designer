#include "GitService.h"
#include "analyzer/ai/CredentialManager.h"

#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace Git {

GitService& GitService::instance() {
    static GitService s_instance;
    return s_instance;
}

GitService::GitService(QObject* parent)
    : QObject(parent)
{
}

bool GitService::isGitInstalled() const {
    QProcess proc;
    proc.start("git", QStringList() << "--version");
    if (!proc.waitForFinished(3000)) return false;
    return proc.exitCode() == 0;
}

bool GitService::isGitRepository(const QString& repoPath) const {
    if (repoPath.isEmpty() || !QDir(repoPath).exists()) return false;
    QString out;
    return runGitCommand(repoPath, QStringList() << "rev-parse" << "--is-inside-work-tree", &out) && out.trimmed() == "true";
}

bool GitService::runGitCommand(const QString& repoPath, const QStringList& args, QString* stdOut, QString* stdErr, int timeoutMs) const {
    QProcess proc;
    if (!repoPath.isEmpty()) {
        proc.setWorkingDirectory(repoPath);
    }

    // Pass environment with credential helper / GIT_TERMINAL_PROMPT=0 so it never hangs indefinitely
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("GIT_TERMINAL_PROMPT", "0");
    proc.setProcessEnvironment(env);

    proc.start("git", args);
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        if (stdErr) *stdErr = "Git command timed out.";
        return false;
    }

    if (stdOut) *stdOut = QString::fromUtf8(proc.readAllStandardOutput());
    if (stdErr) *stdErr = QString::fromUtf8(proc.readAllStandardError());

    return proc.exitCode() == 0;
}

GitStatus GitService::getStatus(const QString& repoPath) const {
    GitStatus status;
    if (!isGitRepository(repoPath)) {
        status.isRepo = false;
        return status;
    }
    status.isRepo = true;

    QString out, err;
    if (!runGitCommand(repoPath, QStringList() << "status" << "--porcelain=v1" << "-b", &out, &err)) {
        return status;
    }

    QStringList lines = out.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        if (line.startsWith("## ")) {
            // Branch header, e.g., "## main...origin/main [ahead 1, behind 2]"
            QString header = line.mid(3).trimmed();
            int dots = header.indexOf("...");
            if (dots != -1) {
                status.currentBranch = header.left(dots);
                int aheadIdx = header.indexOf("ahead ");
                if (aheadIdx != -1) {
                    QString aheadStr = header.mid(aheadIdx + 6).section(QRegularExpression("[,|\\]]"), 0, 0);
                    status.ahead = aheadStr.toInt();
                }
                int behindIdx = header.indexOf("behind ");
                if (behindIdx != -1) {
                    QString behindStr = header.mid(behindIdx + 7).section(QRegularExpression("[,|\\]]"), 0, 0);
                    status.behind = behindStr.toInt();
                }
            } else {
                int bracket = header.indexOf(" [");
                if (bracket != -1) {
                    status.currentBranch = header.left(bracket);
                } else {
                    status.currentBranch = header;
                }
            }
        } else if (line.length() >= 3) {
            GitFileStatus f;
            f.indexStatus = line[0].toLatin1();
            f.workTreeStatus = line[1].toLatin1();
            f.path = line.mid(3).trimmed();
            // Handle quotes if path contains spaces
            if (f.path.startsWith('"') && f.path.endsWith('"')) {
                f.path = f.path.mid(1, f.path.length() - 2);
            }
            status.files.append(f);
        }
    }

    return status;
}

QStringList GitService::getBranches(const QString& repoPath) const {
    QStringList branches;
    QString out;
    if (runGitCommand(repoPath, QStringList() << "branch" << "--list", &out)) {
        for (const QString& line : out.split('\n', Qt::SkipEmptyParts)) {
            QString b = line.trimmed();
            if (b.startsWith('*')) b = b.mid(1).trimmed();
            if (!b.isEmpty()) branches.append(b);
        }
    }
    return branches;
}

QMap<QString, QString> GitService::getRemotes(const QString& repoPath) const {
    QMap<QString, QString> remotes;
    QString out;
    if (runGitCommand(repoPath, QStringList() << "remote" << "-v", &out)) {
        for (const QString& line : out.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2 && line.contains("(fetch)")) {
                remotes[parts[0]] = parts[1];
            }
        }
    }
    return remotes;
}

QList<GitCommit> GitService::getCommitHistory(const QString& repoPath, int count) const {
    QList<GitCommit> history;
    QString out;
    QString format = "%H%x09%h%x09%an%x09%ad%x09%s";
    if (runGitCommand(repoPath, QStringList() << "log" << QString("-n%1").arg(count) << QString("--format=%1").arg(format) << "--date=iso", &out)) {
        for (const QString& line : out.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split('\t');
            if (parts.size() >= 5) {
                GitCommit c;
                c.hash = parts[0];
                c.shortHash = parts[1];
                c.author = parts[2];
                c.date = QDateTime::fromString(parts[3], Qt::ISODate);
                c.message = parts[4];
                history.append(c);
            }
        }
    }
    return history;
}

QString GitService::getDiff(const QString& repoPath, const QString& filePath, bool cached) const {
    QStringList args;
    args << "diff";
    if (cached) args << "--cached";
    if (!filePath.isEmpty()) args << "--" << filePath;
    QString out;
    runGitCommand(repoPath, args, &out);
    return out;
}

bool GitService::stageFile(const QString& repoPath, const QString& filePath, QString* errorOut) {
    if (filePath.isEmpty()) {
        if (errorOut) *errorOut = "File path cannot be empty.";
        return false;
    }
    QString out, err;
    bool ok = runGitCommand(repoPath, QStringList() << "add" << filePath, &out, &err);
    if (!ok && errorOut) *errorOut = err;
    emit statusUpdated();
    return ok;
}

bool GitService::stageAll(const QString& repoPath, QString* errorOut) {
    QString out, err;
    bool ok = runGitCommand(repoPath, QStringList() << "add" << "-A", &out, &err);
    if (!ok && errorOut) *errorOut = err;
    emit statusUpdated();
    return ok;
}

bool GitService::unstageFile(const QString& repoPath, const QString& filePath, QString* errorOut) {
    if (filePath.isEmpty()) {
        if (errorOut) *errorOut = "File path cannot be empty.";
        return false;
    }
    QString out, err;
    bool ok = runGitCommand(repoPath, QStringList() << "restore" << "--staged" << filePath, &out, &err);
    if (!ok) {
        // Fallback to git reset HEAD for older git versions
        ok = runGitCommand(repoPath, QStringList() << "reset" << "HEAD" << "--" << filePath, &out, &err);
    }
    if (!ok && errorOut) *errorOut = err;
    emit statusUpdated();
    return ok;
}

bool GitService::commit(const QString& repoPath, const QString& message, QString* errorOut) {
    if (message.trimmed().isEmpty()) {
        if (errorOut) *errorOut = "Commit message cannot be empty.";
        return false;
    }
    QString out, err;
    bool ok = runGitCommand(repoPath, QStringList() << "commit" << "-m" << message.trimmed(), &out, &err);
    if (!ok && errorOut) *errorOut = err;
    emit statusUpdated();
    emit operationCompleted("commit", ok, ok ? "Committed successfully" : err);
    return ok;
}

bool GitService::fetch(const QString& repoPath, const QString& remote, QString* errorOut) {
    QString out, err;
    bool ok = runGitCommand(repoPath, QStringList() << "fetch" << remote, &out, &err);
    if (!ok && errorOut) *errorOut = err;
    emit statusUpdated();
    return ok;
}

bool GitService::pull(const QString& repoPath, const QString& remote, const QString& branch, QString* errorOut) {
    QStringList args;
    args << "pull" << remote;
    if (!branch.isEmpty()) args << branch;
    QString out, err;
    bool ok = runGitCommand(repoPath, args, &out, &err);
    if (!ok && errorOut) *errorOut = err;
    emit statusUpdated();
    emit operationCompleted("pull", ok, ok ? "Pulled successfully" : err);
    return ok;
}

bool GitService::push(const QString& repoPath, const QString& remote, const QString& branch, bool force, QString* errorOut) {
    // Phase 6O: Force push is strictly prohibited without safety policy validation
    if (force) {
        if (errorOut) *errorOut = "Safety Violation: Force push is blocked to prevent accidental upstream data loss.";
        return false;
    }

    QStringList args;
    args << "push" << remote;
    if (!branch.isEmpty()) args << branch;
    QString out, err;
    bool ok = runGitCommand(repoPath, args, &out, &err, 30000); // 30s timeout
    if (!ok && errorOut) *errorOut = err;
    emit statusUpdated();
    emit operationCompleted("push", ok, ok ? "Pushed successfully" : err);
    return ok;
}

bool GitService::configureRemote(const QString& repoPath, const QString& remoteName, const QString& url, QString* errorOut) {
    if (remoteName.isEmpty() || url.isEmpty()) {
        if (errorOut) *errorOut = "Remote name and URL cannot be empty.";
        return false;
    }
    auto remotes = getRemotes(repoPath);
    QStringList args;
    if (remotes.contains(remoteName)) {
        args << "remote" << "set-url" << remoteName << url;
    } else {
        args << "remote" << "add" << remoteName << url;
    }
    QString out, err;
    bool ok = runGitCommand(repoPath, args, &out, &err);
    if (!ok && errorOut) *errorOut = err;
    return ok;
}

QString GitService::getGitHubToken() const {
    return Security::CredentialManager::instance().retrieveCredential("github", "personal_access_token");
}

void GitService::setGitHubToken(const QString& token) {
    if (token.isEmpty()) {
        Security::CredentialManager::instance().deleteCredential("github", "personal_access_token");
    } else {
        Security::CredentialManager::instance().storeCredential("github", "personal_access_token", token);
    }
}

QString GitService::getAISummary(const QString& repoPath) const {
    GitStatus s = getStatus(repoPath);
    if (!s.isRepo) return "Not a Git repository.";

    QString md;
    md += QString("### Git Status\n");
    md += QString("- **Branch**: `%1`\n").arg(s.currentBranch);
    md += QString("- **Ahead / Behind**: +%1 / -%2\n").arg(s.ahead).arg(s.behind);
    if (s.isClean()) {
        md += "- **Working tree**: Clean\n";
    } else {
        int staged = 0, unstaged = 0, untracked = 0;
        for (const auto& f : s.files) {
            if (f.isStaged()) ++staged;
            else if (f.isUntracked()) ++untracked;
            else ++unstaged;
        }
        md += QString("- **Changed Files**: %1 staged, %2 unstaged, %3 untracked\n").arg(staged).arg(unstaged).arg(untracked);
    }
    return md;
}

} // namespace Git
