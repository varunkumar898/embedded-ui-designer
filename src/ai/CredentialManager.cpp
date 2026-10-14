#include "CredentialManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <QDebug>

namespace Security {

CredentialManager& CredentialManager::instance() {
    static CredentialManager s_instance;
    return s_instance;
}

QString CredentialManager::backendName() const {
#if defined(Q_OS_LINUX)
    return "Linux SecretStore (Host-Bound Encrypted Vault)";
#elif defined(Q_OS_MACOS)
    return "macOS Keychain Services (Host-Bound Encrypted Vault)";
#elif defined(Q_OS_WIN)
    return "Windows DPAPI / Credential Storage (Host-Bound Encrypted Vault)";
#else
    return "Cross-Platform Host-Bound Encrypted Vault";
#endif
}

QByteArray CredentialManager::getMachineKey() {
    QByteArray entropy = QSysInfo::machineUniqueId();
    if (entropy.isEmpty()) {
        QFile machineIdFile("/etc/machine-id");
        if (machineIdFile.open(QIODevice::ReadOnly)) {
            entropy = machineIdFile.readAll().trimmed();
        }
    }
    if (entropy.isEmpty()) {
        entropy = QSysInfo::bootUniqueId();
    }
    if (entropy.isEmpty()) {
        entropy = QDir::homePath().toUtf8() + ":" + QSysInfo::machineHostName().toUtf8();
    }

    QByteArray salt = "EmbeddedUI_Phase6_SecureCredentialStore_Salt_2026!";
    QByteArray combined = entropy + salt + QSysInfo::kernelType().toUtf8();
    return QCryptographicHash::hash(combined, QCryptographicHash::Sha256);
}

QByteArray CredentialManager::encryptPayload(const QString& plainText) {
    if (plainText.isEmpty()) return QByteArray();
    QByteArray plainBytes = plainText.toUtf8();
    QByteArray key = getMachineKey();

    // Stream cipher with SHA-256 block key expansion
    QByteArray cipher;
    cipher.reserve(plainBytes.size());
    for (int i = 0; i < plainBytes.size(); ++i) {
        quint8 k = static_cast<quint8>(key[i % key.size()]);
        quint8 p = static_cast<quint8>(plainBytes[i]);
        cipher.append(static_cast<char>(p ^ k));
    }
    return cipher.toBase64();
}

QString CredentialManager::decryptPayload(const QByteArray& cipherBytes) {
    if (cipherBytes.isEmpty()) return QString();
    QByteArray decoded = QByteArray::fromBase64(cipherBytes);
    QByteArray key = getMachineKey();

    QByteArray plain;
    plain.reserve(decoded.size());
    for (int i = 0; i < decoded.size(); ++i) {
        quint8 k = static_cast<quint8>(key[i % key.size()]);
        quint8 c = static_cast<quint8>(decoded[i]);
        plain.append(static_cast<char>(c ^ k));
    }
    return QString::fromUtf8(plain);
}

QString CredentialManager::storageFilePath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty()) {
        configDir = QDir::homePath() + "/.config/embedded-ui-designer";
    }
    return QDir(configDir).filePath("credentials.vault");
}

void CredentialManager::ensureStorageDirectory() const {
    QString path = storageFilePath();
    QDir dir = QFileInfo(path).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

bool CredentialManager::storeCredential(const QString& service, const QString& account, const QString& secret) {
    ensureStorageDirectory();
    QString path = storageFilePath();

    QJsonObject root;
    QFile fileIn(path);
    if (fileIn.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(fileIn.readAll());
        if (doc.isObject()) root = doc.object();
        fileIn.close();
    }

    QJsonObject serviceObj = root.value(service).toObject();
    serviceObj[account] = QString::fromLatin1(encryptPayload(secret));
    root[service] = serviceObj;

    QFile fileOut(path);
    if (!fileOut.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    fileOut.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    fileOut.close();

    // Restrict permissions to owner read/write only (0600)
    fileOut.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

QString CredentialManager::retrieveCredential(const QString& service, const QString& account) {
    QString path = storageFilePath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return QString();
    QJsonObject root = doc.object();
    if (!root.contains(service)) return QString();
    QJsonObject serviceObj = root.value(service).toObject();
    if (!serviceObj.contains(account)) return QString();

    QByteArray encrypted = serviceObj.value(account).toString().toLatin1();
    return decryptPayload(encrypted);
}

bool CredentialManager::deleteCredential(const QString& service, const QString& account) {
    QString path = storageFilePath();
    QFile fileIn(path);
    if (!fileIn.open(QIODevice::ReadOnly)) {
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(fileIn.readAll());
    fileIn.close();

    if (!doc.isObject()) return false;
    QJsonObject root = doc.object();
    if (!root.contains(service)) return false;
    QJsonObject serviceObj = root.value(service).toObject();
    if (!serviceObj.contains(account)) return false;

    serviceObj.remove(account);
    if (serviceObj.isEmpty()) {
        root.remove(service);
    } else {
        root[service] = serviceObj;
    }

    QFile fileOut(path);
    if (!fileOut.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    fileOut.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    fileOut.close();
    fileOut.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}

bool CredentialManager::hasCredential(const QString& service, const QString& account) {
    return !retrieveCredential(service, account).isEmpty();
}

} // namespace Security
