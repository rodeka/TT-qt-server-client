#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QJsonObject>

class ClientConnection : public QObject {
    Q_OBJECT
public:
    explicit ClientConnection(QTcpSocket *socket, QObject *parent = nullptr);
    ~ClientConnection() override;

    QString id() const { return client_id_; }
    QHostAddress peerAddress() const;
    quint16 peerPort() const;

public slots:
    void sendJson(const QJsonObject &obj);

signals:
    void jsonReceived(const QString &clientId, const QJsonObject &obj);
    void disconnected(const QString &clientId);
    void logMessage(const QString &msg);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket *socket_;
    QByteArray buffer_;
    QString client_id_;
};
