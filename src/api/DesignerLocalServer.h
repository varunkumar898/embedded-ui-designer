#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QSet>
#include <QByteArray>
#include <QMap>
#include <QString>

class DesignerController;

class DesignerLocalServer : public QObject {
    Q_OBJECT

public:
    explicit DesignerLocalServer(DesignerController* controller, QObject* parent = nullptr);
    ~DesignerLocalServer() override;

    bool start(quint16 port = 8765);
    void stop();
    bool isListening() const;
    quint16 serverPort() const;

    // Authentication token management (Phase 6A)
    void setAuthToken(const QString& token);
    QString authToken() const;
    QString sessionToken() const { return authToken(); }
    void setRequireAuth(bool require);
    bool requiresAuth() const;
    bool isClientAuthenticated(QTcpSocket* socket) const;

    // JSON-RPC processing (exposed for deterministic testing and socket dispatch)
    QJsonObject processJsonRpcRequest(const QJsonObject& req, bool* inOutAuthenticated = nullptr);

signals:
    void clientConnected(const QString& address);
    void clientDisconnected(const QString& address);
    void commandExecuted(const QString& method);
    void authenticationAttempted(const QString& address, bool success);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    void processLine(QTcpSocket* socket, const QByteArray& line);
    void ensureGeneratedAuthToken();
    QString tokenFilePath() const;

    DesignerController* m_controller = nullptr;
    QTcpServer* m_tcpServer = nullptr;
    QList<QTcpSocket*> m_clients;
    QSet<QTcpSocket*> m_authenticatedClients;
    QMap<QTcpSocket*, QByteArray> m_buffers;
    QString m_authToken;
    bool m_requireAuth = true;
};
