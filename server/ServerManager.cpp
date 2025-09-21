#include "ServerManager.h"
#include "ClientConnection.h"
#include "MessageFraming.h"
#include <QThread>
#include <QTcpSocket>
#include <QJsonObject>

ServerManager::ServerManager(quint16 port, QObject *parent)
    : QObject(parent),
    server_(new QTcpServer(this)),
    port_(port)
{
    connect(server_, &QTcpServer::newConnection, this, &ServerManager::onNewConnection);
}

ServerManager::~ServerManager() {
    stopListening();
}

void ServerManager::startListening() {
    if(!server_->isListening()) {
        if(server_->listen(QHostAddress::Any, port_)) {
            emit logMessage(QStringLiteral("Server listening on port %1").arg(port_));
        }
        else {
            emit logMessage(QStringLiteral("Failed to listen on port %1 : %2").arg(port_).arg(server_->errorString()));
        }
    }
}

void ServerManager::stopListening() {
    if(server_->isListening()) {
        server_->close();
    }

    QMutexLocker loccker(&mutex_);
    for (auto c : clients_) {
        if(c) {
            QMetaObject::invokeMethod(c, "deleteLater", Qt::QueuedConnection);
        }
    }
    clients_.clear();
    emit logMessage("Server stopped");
}

void ServerManager::onNewConnection() {
    while (server_->hasPendingConnections()) {
        QTcpSocket *sock = server_->nextPendingConnection();
        ClientConnection *cc = new ClientConnection(sock);

        connect(cc, &ClientConnection::jsonReceived, this, &ServerManager::onClientJson);
        connect(cc, &ClientConnection::disconnected, this, &ServerManager::onClientDisconnected);
        connect(cc, &ClientConnection::logMessage, this, &ServerManager::onClientLog);

        {
            QMutexLocker locker(&mutex_);
            clients_.insert(cc->id(), cc);
        }
        emit clientConnected(cc->id(), cc->peerAddress().toString(), cc->peerPort());
    }
}

void ServerManager::onClientJson(const QString &clientId, const QJsonObject &obj) {
    emit dataReceived(clientId, obj);
}

void ServerManager::onClientDisconnected(const QString &clientId) {
    {
        QMutexLocker locker(&mutex_);
        clients_.remove(clientId);
    }
    emit clientDisconnected(clientId);
}

void ServerManager::onClientLog(const QString &msg) {
    emit logMessage(msg);
}

void ServerManager::sendToClient(const QString &clientId, const QJsonObject &obj) {
    QMutexLocker locker(&mutex_);
    ClientConnection *c = clients_.value(clientId, nullptr);
    if(!c) return;
    QMetaObject::invokeMethod(c, "sendJson", Qt::QueuedConnection, Q_ARG(QJsonObject, obj));
}

void ServerManager::broadcast(const QJsonObject &obj) {
    QMutexLocker locker(&mutex_);
    for(auto c: clients_) {
        if(c) {
            QMetaObject::invokeMethod(c, "sendJson", Qt::QueuedConnection, Q_ARG(QJsonObject, obj));
        }
    }
}
