#pragma once

#include <QString>
#include <QByteArray>

namespace Security {

/**
 * @brief Platform-neutral secure credential storage manager.
 * Protects AI provider API keys, GitHub authentication tokens, and sensitive secrets.
 * Prevents plain-text / trivial Base64 credential exposure.
 */
class CredentialManager {
public:
    static CredentialManager& instance();

    bool storeCredential(const QString& service, const QString& account, const QString& secret);
    QString retrieveCredential(const QString& service, const QString& account);
    bool deleteCredential(const QString& service, const QString& account);
    bool hasCredential(const QString& service, const QString& account);

    QString backendName() const;

    // Cryptographic payload encryption tied to host machine entropy
    static QByteArray encryptPayload(const QString& plainText);
    static QString decryptPayload(const QByteArray& cipherBytes);

private:
    CredentialManager() = default;
    ~CredentialManager() = default;
    CredentialManager(const CredentialManager&) = delete;
    CredentialManager& operator=(const CredentialManager&) = delete;

    QString storageFilePath() const;
    void ensureStorageDirectory() const;
    static QByteArray getMachineKey();
};

} // namespace Security

namespace AI {
using Security::CredentialManager;
}

