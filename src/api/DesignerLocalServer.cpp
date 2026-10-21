#include "DesignerLocalServer.h"
#include "DesignerController.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QHostAddress>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QCryptographicHash>
#include <QDebug>

DesignerLocalServer::DesignerLocalServer(DesignerController* controller, QObject* parent)
    : QObject(parent)
    , m_controller(controller)
    , m_tcpServer(new QTcpServer(this))
{
    ensureGeneratedAuthToken();
    connect(m_tcpServer, &QTcpServer::newConnection, this, &DesignerLocalServer::onNewConnection);
}

DesignerLocalServer::~DesignerLocalServer() {
    stop();
}

// Canonical local-auth token location, shared with the Python MCP proxy in
// mcp/embedded_ui_mcp.py (load_token_candidates). Keep both in sync.
//
//   $EUIDESIGNER_MCP_TOKEN_FILE  (highest precedence, for tests/CI)
//   GenericConfigLocation/embedded-ui-designer/mcp_token:
//     Linux   ~/.config/embedded-ui-designer/mcp_token
//     Windows %APPDATA%\embedded-ui-designer\mcp_token
//     macOS   ~/Library/Preferences/embedded-ui-designer/mcp_token
//   fallback  ~/.config/embedded-ui-designer/mcp_token
//
// Legacy locations are imported once so upgrades keep working
// (see findLegacyTokenFiles()).
QString DesignerLocalServer::tokenFilePath() const {
    const QByteArray override = qgetenv("EUIDESIGNER_MCP_TOKEN_FILE");
    if (!override.isEmpty()) {
        return QString::fromUtf8(override);
    }
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (configDir.isEmpty()) {
        configDir = QDir::homePath() + "/.config";
    }
    return QDir(configDir).filePath("embedded-ui-designer/mcp_token");
}

QStringList DesignerLocalServer::findLegacyTokenFiles() const {
    QStringList candidates;
    // Pre-hotfix location: QStandardPaths::AppConfigLocation (org-scoped Qt dir)
    QString legacyConfig = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!legacyConfig.isEmpty()) {
        candidates << QDir(legacyConfig).filePath("mcp_token");
    }
    // Old home-dir layout some clients read
    candidates << QDir::homePath() + "/.config/EmbeddedDev/EmbeddedUIDesigner/mcp_token";
    // Historic shared path (was the Python proxy's only guess)
    candidates << QDir::homePath() + "/.config/embedded-ui-designer/mcp_token";

    QStringList found;
    for (const QString& candidate : candidates) {
        QFileInfo info(candidate);
        if (info.exists() && info.isReadable()) {
            found << candidate;
        }
    }
    return found;
}

void DesignerLocalServer::ensureGeneratedAuthToken() {
    if (!m_authToken.isEmpty()) {
        return;
    }

    QString path = tokenFilePath();
    QFile fileIn(path);
    if (fileIn.open(QIODevice::ReadOnly)) {
        QString existing = QString::fromUtf8(fileIn.readAll()).trimmed();
        fileIn.close();
        if (!existing.isEmpty()) {
            m_authToken = existing;
            return;
        }
    }

    // Migration: reuse an existing token from a legacy location so that a
    // running Python proxy / stored credentials do not break after the update.
    const QStringList legacyFiles = findLegacyTokenFiles();
    for (const QString& legacy : legacyFiles) {
        QFile legacyIn(legacy);
        if (legacyIn.open(QIODevice::ReadOnly)) {
            QString imported = QString::fromUtf8(legacyIn.readAll()).trimmed();
            legacyIn.close();
            if (imported.isEmpty()) {
                continue;
            }
            m_authToken = imported;
            QDir dir = QFileInfo(path).dir();
            if (!dir.exists()) {
                dir.mkpath(".");
            }
            QFile fileOut(path);
            if (fileOut.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                fileOut.write(m_authToken.toUtf8());
                fileOut.close();
                fileOut.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            }
            qInfo() << "[DesignerLocalServer] imported MCP auth token from legacy location:" << legacy;
            return;
        }
    }

    // Generate strong 32-byte session token
    QByteArray rawUuid = QUuid::createUuid().toByteArray() + QUuid::createUuid().toByteArray();
    m_authToken = QCryptographicHash::hash(rawUuid, QCryptographicHash::Sha256).toHex();

    QDir dir = QFileInfo(path).dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile fileOut(path);
    if (fileOut.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fileOut.write(m_authToken.toUtf8());
        fileOut.close();
        fileOut.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }
}

void DesignerLocalServer::setAuthToken(const QString& token) {
    m_authToken = token;
}

QString DesignerLocalServer::authToken() const {
    return m_authToken;
}

void DesignerLocalServer::setRequireAuth(bool require) {
    m_requireAuth = require;
}

bool DesignerLocalServer::requiresAuth() const {
    return m_requireAuth;
}

bool DesignerLocalServer::isClientAuthenticated(QTcpSocket* socket) const {
    return !m_requireAuth || m_authenticatedClients.contains(socket);
}

bool DesignerLocalServer::start(quint16 port) {
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }

    ensureGeneratedAuthToken();

    // STRICT SECURITY: Localhost only. Never listen on Any or AnyIPv4.
    bool ok = m_tcpServer->listen(QHostAddress::LocalHost, port);
    if (ok) {
        qDebug() << "[DesignerLocalServer] Listening for MCP AI commands on 127.0.0.1:" << port
                 << "(Token auth enabled)";
    } else {
        qWarning() << "[DesignerLocalServer] Failed to listen on port" << port << ":" << m_tcpServer->errorString();
    }
    return ok;
}

void DesignerLocalServer::stop() {
    for (QTcpSocket* s : m_clients) {
        if (s) {
            s->disconnect(this);
            s->close();
            s->deleteLater();
        }
    }
    m_clients.clear();
    m_authenticatedClients.clear();
    m_buffers.clear();

    if (m_tcpServer && m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
}

bool DesignerLocalServer::isListening() const {
    return m_tcpServer && m_tcpServer->isListening();
}

quint16 DesignerLocalServer::serverPort() const {
    return m_tcpServer ? m_tcpServer->serverPort() : 0;
}

void DesignerLocalServer::onNewConnection() {
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket* socket = m_tcpServer->nextPendingConnection();
        if (!socket) continue;

        m_clients.append(socket);
        m_buffers[socket] = QByteArray();

        connect(socket, &QTcpSocket::readyRead, this, &DesignerLocalServer::onClientReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &DesignerLocalServer::onClientDisconnected);

        emit clientConnected(socket->peerAddress().toString());
    }
}

void DesignerLocalServer::onClientReadyRead() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    QByteArray& buffer = m_buffers[socket];
    buffer.append(socket->readAll());

    int newlineIdx = -1;
    while ((newlineIdx = buffer.indexOf('\n')) >= 0) {
        QByteArray line = buffer.left(newlineIdx).trimmed();
        buffer.remove(0, newlineIdx + 1);

        if (!line.isEmpty()) {
            processLine(socket, line);
        }
    }
}

void DesignerLocalServer::onClientDisconnected() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    m_clients.removeAll(socket);
    m_authenticatedClients.remove(socket);
    m_buffers.remove(socket);

    emit clientDisconnected(socket->peerAddress().toString());
    socket->deleteLater();
}

QJsonObject DesignerLocalServer::processJsonRpcRequest(const QJsonObject& req, bool* inOutAuthenticated) {
    QJsonObject resp;
    resp["jsonrpc"] = "2.0";
    QJsonValue reqId = req.value("id");
    resp["id"] = reqId;

    QString method = req.value("method").toString();
    QJsonObject params;
    if (req.value("params").isObject()) {
        params = req.value("params").toObject();
    }

    bool isAuthed = inOutAuthenticated ? *inOutAuthenticated : false;

    // Check for inline token or authenticate handshake method
    QString suppliedToken;
    if (params.contains("token")) {
        suppliedToken = params.value("token").toString();
    } else if (params.contains("auth_token")) {
        suppliedToken = params.value("auth_token").toString();
    } else if (req.contains("token")) {
        suppliedToken = req.value("token").toString();
    } else if (req.contains("auth_token")) {
        suppliedToken = req.value("auth_token").toString();
    }

    if (!suppliedToken.isEmpty() && !m_authToken.isEmpty() && suppliedToken == m_authToken) {
        isAuthed = true;
    }

    // Handle explicit handshake authentication method
    if (method == "authenticate" || method == "auth") {
        if (!suppliedToken.isEmpty() && suppliedToken == m_authToken) {
            isAuthed = true;
            if (inOutAuthenticated) *inOutAuthenticated = true;
            QJsonObject res;
            res["authenticated"] = true;
            res["message"] = "Authentication successful";
            resp["result"] = res;
        } else {
            QJsonObject errObj;
            errObj["code"] = -32001;
            errObj["message"] = "Authentication failed: invalid token";
            resp["error"] = errObj;
        }
        return resp;
    }

    // Enforce authentication gate on all other MCP methods
    if (m_requireAuth && !isAuthed) {
        QJsonObject errObj;
        errObj["code"] = -32001;
        errObj["message"] = "Unauthorized: MCP client must authenticate with valid session token first";
        resp["error"] = errObj;
        return resp;
    }

    if (inOutAuthenticated) *inOutAuthenticated = isAuthed;
    emit commandExecuted(method);

    if (m_controller) {
        QJsonObject result = m_controller->dispatch(method, params);
        if (result.value("success").toBool(true) || !result.contains("error")) {
            resp["result"] = result;
        } else {
            QJsonObject errObj;
            errObj["code"] = -32603;
            errObj["message"] = result.value("error").toString("Execution failed");
            errObj["data"] = result;
            resp["error"] = errObj;
        }
    } else {
        resp["result"] = QJsonObject();
    }

    return resp;
}

void DesignerLocalServer::processLine(QTcpSocket* socket, const QByteArray& line) {
    if (!m_controller) return;

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(line, &parseErr);

    QJsonObject resp;
    resp["jsonrpc"] = "2.0";

    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        QJsonObject errObj;
        errObj["code"] = -32700;
        errObj["message"] = "Parse error: " + parseErr.errorString();
        resp["id"] = QJsonValue::Null;
        resp["error"] = errObj;

        QByteArray respBytes = QJsonDocument(resp).toJson(QJsonDocument::Compact) + "\n";
        socket->write(respBytes);
        socket->flush();
        return;
    }

    bool isAuthed = m_authenticatedClients.contains(socket);
    QJsonObject req = doc.object();
    resp = processJsonRpcRequest(req, &isAuthed);

    if (isAuthed) {
        m_authenticatedClients.insert(socket);
    }

    QString method = req.value("method").toString();
    if (method == "authenticate" || method == "auth") {
        emit authenticationAttempted(socket->peerAddress().toString(), isAuthed);
    }

    QByteArray respBytes = QJsonDocument(resp).toJson(QJsonDocument::Compact) + "\n";
    socket->write(respBytes);
    socket->flush();
}
