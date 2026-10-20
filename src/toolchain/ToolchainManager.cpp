#include "ToolchainManager.h"
#include <QStandardPaths>
#include <QDirIterator>
#include <QStorageInfo>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace Toolchain {

ToolchainManager& ToolchainManager::instance() {
    static ToolchainManager inst;
    return inst;
}

ToolchainManager::ToolchainManager(QObject* parent) : QObject(parent) {
    m_allowlisted = {"arm-none-eabi-gcc", "openocd", "esptool", "idf.py",
                     "picotool", "cmake", "ninja", "make", "python3", "git",
                     "unzip", "tar"};
}

bool ToolchainManager::loadManifest(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return false;
    QJsonArray packs = doc.object().value("packs").toArray();
    m_packs.clear();
    for (const QJsonValue& v : packs) {
        QJsonObject o = v.toObject();
        ToolPack p;
        p.id = o.value("id").toString();
        p.name = o.value("name").toString();
        p.version = o.value("version").toString();
        for (const QJsonValue& pl : o.value("platforms").toArray()) p.platforms << pl.toString();
        p.sourceUrl = o.value("sourceUrl").toString();
        p.sha256 = o.value("sha256").toString();
        p.sizeBytes = o.value("sizeBytes").toVariant().toLongLong();
        p.license = o.value("license").toString();
        p.licenseUrl = o.value("licenseUrl").toString();
        p.installMethod = o.value("installMethod").toString();
        for (const QJsonValue& e : o.value("entryPoints").toArray()) p.entryPoints << e.toString();
        for (const QJsonValue& r : o.value("requiredRuntimes").toArray()) p.requiredRuntimes << r.toString();
        for (const QJsonValue& f : o.value("families").toArray()) p.families << f.toString();
        p.manualOnly = o.value("checksumStatus").toString() == "manual-install-only" || p.sha256.isEmpty();
        if (!p.id.isEmpty()) m_packs.append(p);
    }
    return !m_packs.isEmpty();
}

ToolPack ToolchainManager::pack(const QString& id) const {
    for (const ToolPack& p : m_packs) if (p.id == id) return p;
    return ToolPack();
}

QString ToolchainManager::managedRoot() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/toolchains";
}

bool ToolchainManager::hasDiskSpace(qint64 bytes) const {
    // Use available disk space on the managed root's filesystem.
    QStorageInfo info(managedRoot());
    if (!info.isValid()) info = QStorageInfo(QDir::homePath());
    return info.bytesAvailable() >= bytes;
}

void ToolchainManager::setUserPath(const QString& id, const QString& path) { m_userPaths[id] = path; }
QString ToolchainManager::userPath(const QString& id) const { return m_userPaths.value(id); }

ToolStatus ToolchainManager::detect(const QString& id) const {
    ToolStatus s; s.id = id;
    ToolPack p = pack(id);
    // 1. user path
    QString up = userPath(id);
    if (!up.isEmpty() && QFileInfo::exists(up)) {
        s.state = "found"; s.source = "user-path"; s.path = up; s.version = verifyVersion(up, p);
        return s;
    }
    // 2. managed dir
    QString managed = managedRoot() + "/" + id;
    if (QDir(managed).exists()) {
        s.state = "installed"; s.source = "managed"; s.path = managed;
        s.version = verifyVersion(managed, p);
        return s;
    }
    // 3. PATH
    for (const QString& ep : p.entryPoints) {
        QString base = QFileInfo(ep).fileName();
        QString exe = QStandardPaths::findExecutable(base);
        if (!exe.isEmpty()) {
            s.state = "found"; s.source = "system"; s.path = exe;
            s.version = runVersion(exe);
            return s;
        }
    }
    s.state = "missing"; s.source = "none";
    return s;
}

QList<ToolStatus> ToolchainManager::detectAll() const {
    QList<ToolStatus> out;
    for (const ToolPack& p : m_packs) out.append(detect(p.id));
    return out;
}

QString ToolchainManager::runVersion(const QString& exe) const {
    QProcess proc;
    proc.start(exe, {"--version"});
    if (!proc.waitForFinished(10000)) { proc.kill(); return QString(); }
    QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
    if (out.isEmpty()) out = QString::fromUtf8(proc.readAllStandardError()).trimmed();
    return out.split('\n').value(0).left(80);
}

QString ToolchainManager::verifyVersion(const QString& root, const ToolPack& p) const {
    if (p.entryPoints.isEmpty()) return QString();
    QString exe = root + "/" + p.entryPoints.first();
    if (!QFileInfo::exists(exe)) return QString();
    return runVersion(exe);
}

bool ToolchainManager::verify(const QString& id, QString* versionOut, QString* error) const {
    ToolPack p = pack(id);
    ToolStatus s = detect(id);
    if (s.state == "missing") { if (error) *error = "not found"; return false; }
    QString v = runVersion(s.path + "/" + (p.entryPoints.isEmpty() ? QString() : QFileInfo(p.entryPoints.first()).fileName()));
    if (v.isEmpty()) { if (error) *error = "version check failed"; return false; }
    if (versionOut) *versionOut = v;
    return true;
}

bool ToolchainManager::install(const QString& id, QString* error,
                               std::function<void(qint64,qint64)> progress) {
    ToolPack p = pack(id);
    if (p.id.isEmpty()) { if (error) *error = "unknown pack"; return false; }
    if (p.manualOnly) { if (error) *error = "manual install only (no verified checksum)"; return false; }
    if (!p.sourceUrl.startsWith("https://")) { if (error) *error = "source URL must be HTTPS"; return false; }
    if (p.sha256.isEmpty()) { if (error) *error = "no checksum available (manual install only)"; return false; }
    if (!hasDiskSpace(p.sizeBytes > 0 ? p.sizeBytes * 2 : 50 * 1024 * 1024)) {
        if (error) *error = "insufficient disk space"; return false;
    }

    // Download
    QNetworkAccessManager nam;
    QNetworkRequest req(QUrl(p.sourceUrl));
    QNetworkReply* reply = nam.get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    // progress
    QObject::connect(reply, &QNetworkReply::downloadProgress, [&](qint64 rx, qint64 total) {
        if (progress) progress(rx, total);
    });
    loop.exec();
    if (reply->error() != QNetworkReply::NoError) {
        QString e = reply->errorString();
        reply->deleteLater();
        if (error) *error = "download failed: " + e;
        return false;
    }
    QByteArray data = reply->readAll();
    reply->deleteLater();

    // Verify sha256
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
    if (hash != p.sha256.toLatin1()) {
        if (error) *error = "checksum mismatch (expected " + p.sha256 + ", got " + QString::fromLatin1(hash) + ")";
        return false;
    }

    // Safe extraction into managed dir (zip-slip / symlink protection).
    QString target = managedRoot() + "/" + p.id;
    QDir().mkpath(target);
    // Write archive to temp file
    QString archivePath = managedRoot() + "/" + p.id + ".archive";
    QFile af(archivePath);
    if (!af.open(QIODevice::WriteOnly)) { if (error) *error = "cannot write archive"; return false; }
    af.write(data); af.close();

    // Extract with system tar/unzip (argument array, no shell), then validate paths.
    QStringList args;
    QString tool;
    if (p.sourceUrl.endsWith(".zip")) { tool = "unzip"; args = {"-o", archivePath, "-d", target}; }
    else { tool = "tar"; args = {"-xf", archivePath, "-C", target}; }
    if (!isAllowlistedExecutable(tool)) { if (error) *error = "extractor not allowlisted"; return false; }
    QProcess proc;
    proc.start(tool, args);
    if (!proc.waitForFinished(300000)) { proc.kill(); if (error) *error = "extraction timed out"; return false; }
    if (proc.exitCode() != 0) {
        if (error) *error = "extraction failed: " + QString::fromUtf8(proc.readAllStandardError());
        return false;
    }
    // Zip-slip / symlink check: ensure every file under target resolves inside target.
    QDirIterator it(target, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QString baseCanonical = QDir(target).canonicalPath();
    while (it.hasNext()) {
        it.next();
        QString canon = QFileInfo(it.filePath()).canonicalFilePath();
        if (!canon.startsWith(baseCanonical + "/") && canon != baseCanonical) {
            // escape detected -> remove target
            QDir(target).removeRecursively();
            if (error) *error = "unsafe archive (path escape): " + it.filePath();
            return false;
        }
    }
    QFile::remove(archivePath);
    emit toolInstalled(p.id);
    return true;
}

bool ToolchainManager::uninstall(const QString& id, QString* error) {
    QString target = managedRoot() + "/" + id;
    if (QDir(target).exists()) {
        if (!QDir(target).removeRecursively()) {
            if (error) *error = "failed to remove " + target;
            return false;
        }
    }
    emit toolRemoved(id);
    return true;
}

ToolchainManager::RunResult ToolchainManager::run(const QString& id, const QStringList& args,
                                                  const QString& workingDir, int timeoutMs) const {
    RunResult r;
    // Resolve executable from managed dir or PATH; must be allowlisted.
    ToolPack p = pack(id);
    QString exe;
    ToolStatus s = detect(id);
    if (s.path.isEmpty()) { r.exitCode = -1; r.stderrText = "tool not found"; return r; }
    // find entry point
    if (!p.entryPoints.isEmpty()) {
        QFileInfo fi(s.path);
        if (fi.isFile()) exe = s.path;
        else exe = s.path + "/" + QFileInfo(p.entryPoints.first()).fileName();
    } else {
        exe = s.path;
    }
    QString base = QFileInfo(exe).fileName();
    // allowlist check on basename
    ToolchainManager* self = const_cast<ToolchainManager*>(this);
    if (!self->isAllowlistedExecutable(base)) {
        r.exitCode = -1; r.stderrText = "executable not allowlisted: " + base; return r;
    }
    // working dir must be inside a project (we just use it as given, but require non-empty)
    if (workingDir.isEmpty()) { r.exitCode = -1; r.stderrText = "working directory required"; return r; }

    QProcess proc;
    proc.setWorkingDirectory(workingDir);
    proc.start(exe, args);
    if (!proc.waitForStarted(5000)) { r.exitCode = -1; r.stderrText = proc.errorString(); return r; }
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill(); proc.waitForFinished(1000); r.timedOut = true; r.exitCode = -1;
        r.stderrText = "timeout"; return r;
    }
    r.exitCode = proc.exitCode();
    r.stdoutText = QString::fromUtf8(proc.readAllStandardOutput());
    r.stderrText = QString::fromUtf8(proc.readAllStandardError());
    return r;
}

bool ToolchainManager::isAllowlistedExecutable(const QString& exe) const {
    return m_allowlisted.contains(exe);
}

} // namespace Toolchain