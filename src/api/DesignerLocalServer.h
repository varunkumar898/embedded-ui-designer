#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QByteArray>
#include <QMap>

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

signals:
    void clientConnected(const QString& address);
    void clientDisconnected(const QString& address);
    void commandExecuted(const QString& method);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    void processLine(QTcpSocket* socket, const QByteArray& line);

    DesignerController* m_controller = nullptr;
    QTcpServer* m_tcpServer = nullptr;
    QList<QTcpSocket*> m_clients;
    QMap<QTcpSocket*, QByteArray> m_buffers;
};
