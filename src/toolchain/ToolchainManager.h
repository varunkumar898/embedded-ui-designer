#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QSet>

namespace Toolchain {

struct ToolPack {
    QString id;
    QString name;
    QString version;
    QStringList platforms;
    QString sourceUrl;
    QString sha256;          // empty => manual-install-only
    qint64 sizeBytes = 0;
    QString license;
    QString licenseUrl;
    QString installMethod;   // archive-extract | upstream-installer-script | manual-install-only
    QStringList entryPoints;
    QStringList requiredRuntimes;
    QStringList families;
    bool manualOnly = false;
};

struct ToolStatus {
    QString id;
    QString state;           // missing | found | installed | outdated
    QString source;          // system | managed | user-path
    QString version;
    QString path;
};

class ToolchainManager : public QObject {
    Q_OBJECT
public:
    static ToolchainManager& instance();

    bool loadManifest(const QString& path);
    QList<ToolPack> packs() const { return m_packs; }
    ToolPack pack(const QString& id) const;

    // Detection: PATH + configured locations + managed directory.
    ToolStatus detect(const QString& id) const;
    QList<ToolStatus> detectAll() const;

    // "Use my own installation" path setting.
    void setUserPath(const QString& id, const QString& path);
    QString userPath(const QString& id) const;

    // Install requires explicit consent (caller shows dialog first).
    // Returns false on any failure; verifies sha256 before extraction.
    bool install(const QString& id, QString* error = nullptr,
                 std::function<void(qint64,qint64)> progress = nullptr);
    bool uninstall(const QString& id, QString* error = nullptr);

    // Verify by running --version without a shell.
    bool verify(const QString& id, QString* versionOut = nullptr, QString* error = nullptr) const;

    QString managedRoot() const;
    bool hasDiskSpace(qint64 bytes) const;

    // Running tools: argument array, no shell, allowlisted, timeout, captured output.
    struct RunResult { int exitCode = -1; QString stdoutText; QString stderrText; bool timedOut = false; };
    RunResult run(const QString& id, const QStringList& args, const QString& workingDir, int timeoutMs = 60000) const;

    bool isAllowlistedExecutable(const QString& exe) const;

signals:
    void toolInstalled(const QString& id);
    void toolRemoved(const QString& id);
    void toolStatusChanged(const QString& id);

private:
    explicit ToolchainManager(QObject* parent = nullptr);
    QList<ToolPack> m_packs;
    QMap<QString, QString> m_userPaths;
    QSet<QString> m_allowlisted;
    bool m_offline = false;

    QString runVersion(const QString& exe) const;
    QString verifyVersion(const QString& root, const ToolPack& p) const;
};

} // namespace Toolchain