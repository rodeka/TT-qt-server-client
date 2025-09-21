#pragma once
#include <QObject>
#include <QTcpServer>
#include <QMap>
#include <QMutex>

class ClientConnection;

class ServerManager : public QObject
{
    Q_OBJECT
public:
    explicit ServerManager(quint16 port, QObject *parent = nullptr);
    ~ServerManager() override;

public slots:
    void startListening();
    void stopListening();

    void sendToClient(const QString &clientId, const QJsonObject &obj);
    void broadcast(const QJsonObject &obj);

signals:
    void clientConnected(const QString &clientId, const QString &ip, quint16 port);
    void clientDisconnected(const QString &clientId);
    void dataReceived(const QString &clientId, const QJsonObject &obj);
    void logMessage(const QString &msg);

private slots:
    void onNewConnection();
    void onClientJson(const QString &clientId, const QJsonObject &obj);
    void onClientDisconnected(const QString &clientId);
    void onClientLog(const QString &msg);

private:
    QTcpServer *server_;
    quint16 port_;
    QMap<QString, ClientConnection*> clients_;
    QMutex mutex_;
};

