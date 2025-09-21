#include "DeviceClient.h"
#include "MessageFraming.h"
#include <QCoreApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QTextStream>
#include <QDebug>

// generate double [minVal : maxVal]
inline double randRange(double minVal, double maxVal) {
    return minVal + QRandomGenerator::global()->generateDouble() * (maxVal - minVal);
}

DeviceClient::DeviceClient(const QString &host, quint16 port, QObject *parent)
    : QObject(parent),
    socket_(new QTcpSocket(this)),
    host_(host),
    port_(port),
    started_(false),
    critLatencyMs_(100),
    critPacketLoss_(0.05)
{
    connect(&retryTimer_, &QTimer::timeout, this, &DeviceClient::tryConnect);
    retryTimer_.setInterval(5000);

    connect(socket_, &QTcpSocket::connected, this, &DeviceClient::onConnected);
    connect(socket_, &QTcpSocket::readyRead, this, &DeviceClient::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &DeviceClient::onDisconnected);

    connect(&sendTimer_, &QTimer::timeout, this, &DeviceClient::onSendTick);

    tryConnect();
}

DeviceClient::~DeviceClient() {
    socket_->close();
}

void DeviceClient::tryConnect() {
    if (socket_->state() == QAbstractSocket::ConnectedState ||
        socket_->state() == QAbstractSocket::ConnectingState) return;
    qInfo() << "AAttempting to connect to" << host_ << port_;
    socket_->connectToHost(host_, port_);
}

void DeviceClient::onConnected() {
    qInfo() << "Connected to server";
    retryTimer_.stop();
}

void DeviceClient::onReadyRead() {
    buffer_.append(socket_->readAll());
    QJsonDocument doc;
    while(tryExtractOne(buffer_, doc)) {
        if(doc.isObject()) {
            handleServerJson(doc.object());
        }
        else {
            qWarning() << "Invalid JSON from server";
        }
    }
}

void DeviceClient::onDisconnected() {
    qInfo() << "Disconnected from server, will retry in 5s";
    started_ = false;
    sendTimer_.stop();
    retryTimer_.start();
}

void DeviceClient::handleServerJson(const QJsonObject &obj) {
    QString type = obj.value("type").toString();
    if(type == "ConnectAck") {
        clientId_ = obj.value("client_id").toString();
        qInfo() << "Got ConnectAck, client_id = " << clientId_;
    }
    else if(type == "Command") {
        QString cmd = obj.value("command").toString().toUpper();
        if(cmd == "START") {
            qInfo() << "Received START command";
            startSending();
        }
        else if (cmd == "STOP"){
            qInfo() << "Received STOP command";
            stopSending();
        }
    }
    else if(type == "Config") {
        if (obj.contains("crit_latency_ms")) critLatencyMs_ = obj.value("crit_latency_ms").toInt(critLatencyMs_);
        if (obj.contains("crit_packet_loss")) critPacketLoss_ = obj.value("crit_packet_loss").toDouble(critPacketLoss_);
        qInfo() << "Applied config: latency" << critLatencyMs_ << "pl" << critPacketLoss_;
    } else {
        qInfo() << "Server message:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    }
}

void DeviceClient::startSending() {
    if(started_) return;
    started_ = true;
    int ms = QRandomGenerator::global()->bounded(10, 100);
    sendTimer_.start(ms);
    qInfo() << "Started sending data. Interval randomized between 10..100ms per message";
}

void DeviceClient::stopSending() {
    started_ = false;
    sendTimer_.stop();
}

QString DeviceClient::randomString(int length) {
    static const char charset[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        " ./-_:,;!@#%^&*()[]{}";
    QString s;
    s.reserve(length);
    for(int i = 0; i < length; i++) {
        int idx = QRandomGenerator::global()->bounded((int)(sizeof(charset) - 1));
        s.append(charset[idx]);
    }
    return s;
}

QJsonObject DeviceClient::produceRandomMessage() {
    int r = QRandomGenerator::global()->bounded(100);
    if (r < 33) {
        // NetworkMetrics
        double bw = randRange(1.0, 1000.0); // Mbps
        double lat = randRange(0.1, 200.0); // ms
        double pl = randRange(0.0, 0.2); // 0..0.2
        QJsonObject obj;
        obj["type"] = "NetworkMetrics";
        obj["bandwidth"] = bw;
        obj["latency"] = lat;
        obj["packet_loss"] = pl;
        return obj;
    } else if (r < 67) {
        // DeviceStatus
        QJsonObject obj;
        obj["type"] = "DeviceStatus";
        obj["uptime"] = QRandomGenerator::global()->bounded(0, 1000000);
        obj["cpu_usage"] = QRandomGenerator::global()->bounded(0, 100);
        obj["memory_usage"] = QRandomGenerator::global()->bounded(0, 100);
        return obj;
    } else {
        // Log
        int lenBucket = QRandomGenerator::global()->bounded(3);
        int len;
        if (lenBucket == 0) len = QRandomGenerator::global()->bounded(5, 50);
        else if (lenBucket == 1) len = QRandomGenerator::global()->bounded(50, 200);
        else len = QRandomGenerator::global()->bounded(200, 800);
        QString message = randomString(len);
        QJsonObject obj;
        obj["type"] = "Log";
        obj["message"] = message;
        obj["severity"] = "INFO";
        return obj;
    }
}

void DeviceClient::onSendTick() {
    if (!started_ || socket_->state() != QTcpSocket::ConnectedState) {
        sendTimer_.stop();
        return;
    }
    int nextMs = QRandomGenerator::global()->bounded(10, 100);
    sendTimer_.start(nextMs);

    QJsonObject msg = produceRandomMessage();

    if (msg.value("type").toString() == "NetworkMetrics") {
        double lat = msg.value("latency").toDouble();
        double pl = msg.value("packet_loss").toDouble();
        if (lat > critLatencyMs_ || pl > critPacketLoss_) {
            // send warning log
            QJsonObject warning;
            warning["type"] = "Log";
            warning["severity"] = "WARNING";
            warning["message"] = QStringLiteral("Threshold exceeded: latency=%1 pl=%2").arg(lat).arg(pl);
            QByteArray wb = packJson(warning);
            socket_->write(wb);
        }
    }

    QByteArray out = packJson(msg);
    socket_->write(out);
    qInfo().noquote() << "Sent:" << msg.value("type").toString();
}
