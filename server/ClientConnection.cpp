#include "ClientConnection.h"
#include "MessageFraming.h"
#include <QJsonDocument>
#include <QUuid>
#include <QDataStream>

ClientConnection::ClientConnection(QTcpSocket *socket, QObject *parent)
    : QObject(parent),
    socket_(socket)
{
    socket_->setParent(this);
    client_id_ = QUuid::createUuid().toString(QUuid::WithoutBraces);

    connect(socket_, &QTcpSocket::readyRead, this, &ClientConnection::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &ClientConnection::onDisconnected);
    emit logMessage(QStringLiteral("ClientConnection created: %1 (%2:%3)")
        .arg(client_id_)
        .arg(socket_->peerAddress().toString())
        .arg(socket_->peerPort()));

    QJsonObject ack;
    ack["type"] = QStringLiteral("ConnectAck");
    ack["client_id"] = client_id_;
    sendJson(ack);
}

ClientConnection::~ClientConnection(){
    if(socket_){
        socket_->close();
    }
}

QHostAddress ClientConnection::peerAddress() const {
    if(socket_) return socket_->peerAddress();
    return QHostAddress();
}

quint16 ClientConnection::peerPort() const {
    if(socket_) return socket_->peerPort();
}

void ClientConnection::sendJson(const QJsonObject &obj) {
    if(!socket_) return;
    QByteArray out = packJson(obj);
    socket_->write(out);
}

void ClientConnection::onReadyRead() {
    buffer_.append(socket_->readAll());
    QJsonDocument doc;
    while (tryExtractOne(buffer_, doc)) {
        if(!doc.isNull() && doc.isObject()) {
            emit jsonReceived(client_id_, doc.object());
        }
        else {
            emit logMessage(QStringLiteral("Received invalid JSON from %1").arg(client_id_));
        }
    }
}

void ClientConnection::onDisconnected() {
    emit logMessage(QStringLiteral("Client disconnected: %1").arg(client_id_));
    emit disconnected(client_id_);
    socket_->deleteLater();
    this->deleteLater();
}
