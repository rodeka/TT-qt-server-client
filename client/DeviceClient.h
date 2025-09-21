#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QRandomGenerator>

class DeviceClient : public QObject
{
    Q_OBJECT
public:
    explicit DeviceClient(const QString &host = "127.0.0.1", quint16 port = 12345, QObject *parent = nullptr);
    ~DeviceClient() override;

private slots:
    void tryConnect();
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onSendTick();

private:
    void handleServerJson(const QJsonObject &obj);
    void startSending();
    void stopSending();
    QString randomString(int length);
    QJsonObject produceRandomMessage();

    QTcpSocket *socket_;
    QString host_;
    quint16 port_;
    QTimer retryTimer_;
    QTimer sendTimer_;
    QByteArray buffer_;
    bool started_;
    QString clientId_;

    int critLatencyMs_;
    double critPacketLoss_;
};
