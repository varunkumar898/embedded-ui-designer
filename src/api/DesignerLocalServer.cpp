#include "DesignerLocalServer.h"
#include "DesignerController.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QHostAddress>
#include <QDebug>

DesignerLocalServer::DesignerLocalServer(DesignerController* controller, QObject* parent)
    : QObject(parent)
    , m_controller(controller)
    , m_tcpServer(new QTcpServer(this))
{
    connect(m_tcpServer, &QTcpServer::newConnection, this, &DesignerLocalServer::onNewConnection);
}

DesignerLocalServer::~DesignerLocalServer() {
    stop();
}

bool DesignerLocalServer::start(quint16 port) {
    if (m_tcpServer->isListening()) {
        m_tcpServer->close();
    }

    bool ok = m_tcpServer->listen(QHostAddress::LocalHost, port);
    if (ok) {
        qDebug() << "[DesignerLocalServer] Listening for MCP AI commands on 127.0.0.1:" << port;
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
        qDebug() << "[DesignerLocalServer] MCP client connected from" << socket->peerAddress().toString();
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
    m_buffers.remove(socket);

    emit clientDisconnected(socket->peerAddress().toString());
    qDebug() << "[DesignerLocalServer] MCP client disconnected";
    socket->deleteLater();
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

    QJsonObject req = doc.object();
    QJsonValue reqId = req.value("id");
    resp["id"] = reqId;

    QString method = req.value("method").toString();
    QJsonObject params;
    if (req.value("params").isObject()) {
        params = req.value("params").toObject();
    }

    emit commandExecuted(method);

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

    QByteArray respBytes = QJsonDocument(resp).toJson(QJsonDocument::Compact) + "\n";
    socket->write(respBytes);
    socket->flush();
}
