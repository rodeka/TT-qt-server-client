#include "MainWindow.h"
#include "ServerManager.h"
#include <QTableWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    serverManager_(nullptr),
    serverThread_(nullptr),
    listenPort_(12345)
{
    setupUi();
}

MainWindow::~MainWindow() {
    if(serverManager_) {
        QMetaObject::invokeMethod(serverManager_, "stopListening", Qt::BlockingQueuedConnection);
        serverThread_->quit();
        serverThread_->wait();
        delete serverThread_;
    }
}

void MainWindow::setupUi() {
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // top buttons
    QHBoxLayout *topButtons = new QHBoxLayout;
    startServerBtn_ = new QPushButton("Start Server");
    stopServerBtn_ = new QPushButton("Stop Server");
    startClientsBtn_ = new QPushButton("Start Clients");
    stopClientsBtn_ = new QPushButton("Stop Clients");
    configBtn_ = new QPushButton("Configure Clients");
    topButtons->addWidget(startServerBtn_);
    topButtons->addWidget(stopServerBtn_);
    topButtons->addWidget(startClientsBtn_);
    topButtons->addWidget(stopClientsBtn_);
    topButtons->addWidget(configBtn_);
    mainLayout->addLayout(topButtons);

    // clients table
    clientsTable_ = new QTableWidget;
    clientsTable_->setColumnCount(4);
    clientsTable_->setHorizontalHeaderLabels({"Client ID", "IP", "Port", "Status"});
    mainLayout->addWidget(clientsTable_);

    // data table
    dataTable_ = new QTableWidget;
    dataTable_->setColumnCount(4);
    dataTable_->setHorizontalHeaderLabels({"Client ID", "Type", "Content", "Time"});
    dataTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(dataTable_);

    // log view
    logView_ = new QTextEdit;
    logView_->setReadOnly(true);
    mainLayout->addWidget(logView_);

    statusLabel_ = new QLabel("Stopped");
    mainLayout->addWidget(statusLabel_);

    setCentralWidget(central);
    setWindowTitle("Server");

    connect(startServerBtn_, &QPushButton::clicked, this, &MainWindow::onStartServer);
    connect(stopServerBtn_, &QPushButton::clicked, this, &MainWindow::onStopServer);
    connect(startClientsBtn_, &QPushButton::clicked, this, &MainWindow::onSendStartClients);
    connect(stopClientsBtn_, &QPushButton::clicked, this, &MainWindow::onSendStopClients);
    connect(configBtn_, &QPushButton::clicked, this, &MainWindow::onConfigureClients);
}

void MainWindow::onStartServer() {
    if (serverManager_) {
        QMessageBox::information(this, "Info", "Server is already running.");
        return;
    }
    serverThread_ = new QThread(this);
    serverManager_ = new ServerManager(listenPort_);
    serverManager_->moveToThread(serverThread_);

    // forward signals from serverManager_ to GUI
    connect(serverThread_, &QThread::started, serverManager_, &ServerManager::startListening);
    connect(serverManager_, &ServerManager::clientConnected, this, &MainWindow::onClientConnected);
    connect(serverManager_, &ServerManager::clientDisconnected, this, &MainWindow::onClientDisconnected);
    connect(serverManager_, &ServerManager::dataReceived, this, &MainWindow::onDataReceived);
    connect(serverManager_, &ServerManager::logMessage, this, &MainWindow::onLogMessage);

    // ensure cleanup when app closes
    connect(this, &MainWindow::destroyed, [this]() {
        if (serverManager_) QMetaObject::invokeMethod(serverManager_, "stopListening", Qt::QueuedConnection);
    });

    serverThread_->start();
    statusLabel_->setText(QStringLiteral("Listening on port %1").arg(listenPort_));
    logView_->append(QStringLiteral("Server thread started"));
}

void MainWindow::onStopServer() {
    if (!serverManager_) {
        QMessageBox::information(this, "Info", "Server is not running.");
        return;
    }
    QMetaObject::invokeMethod(serverManager_, "stopListening", Qt::BlockingQueuedConnection);
    serverThread_->quit();
    serverThread_->wait();
    delete serverThread_;
    serverThread_ = nullptr;
    serverManager_ = nullptr;
    statusLabel_->setText("Stopped");
    logView_->append("Server stopped");
    clientsTable_->setRowCount(0);
}

void MainWindow::onClientConnected(const QString &clientId, const QString &ip, quint16 port) {
    int row = clientsTable_->rowCount();
    clientsTable_->insertRow(row);
    clientsTable_->setItem(row, 0, new QTableWidgetItem(clientId));
    clientsTable_->setItem(row, 1, new QTableWidgetItem(ip));
    clientsTable_->setItem(row, 2, new QTableWidgetItem(QString::number(port)));
    clientsTable_->setItem(row, 3, new QTableWidgetItem("Connected"));
    logView_->append(QStringLiteral("Connected: %1 (%2:%3)").arg(clientId).arg(ip).arg(port));
}

void MainWindow::onClientDisconnected(const QString &clientId) {
    // find in table and mark disconnected (or remove)
    for (int r = 0; r < clientsTable_->rowCount(); ++r) {
        QTableWidgetItem *item = clientsTable_->item(r, 0);
        if (item && item->text() == clientId) {
            clientsTable_->setItem(r, 3, new QTableWidgetItem("Disconnected"));
            break;
        }
    }
    logView_->append(QStringLiteral("Client disconnected: %1").arg(clientId));
}

void MainWindow::onDataReceived(const QString &clientId, const QJsonObject &obj) {
    QString type = obj.value("type").toString();
    QString content;
    if (type == "NetworkMetrics") {
        double bw = obj.value("bandwidth").toDouble();
        double lat = obj.value("latency").toDouble();
        double pl = obj.value("packet_loss").toDouble();
        content = QString("bw=%1 latency=%2 pl=%3").arg(bw).arg(lat).arg(pl);
    } else if (type == "DeviceStatus") {
        qint64 uptime = obj.value("uptime").toInt();
        int cpu = obj.value("cpu_usage").toInt();
        int mem = obj.value("memory_usage").toInt();
        content = QString("uptime=%1 cpu=%2 mem=%3").arg(uptime).arg(cpu).arg(mem);
    } else if (type == "Log") {
        content = obj.value("message").toString();
        content = QString("[%1] %2").arg(obj.value("severity").toString()).arg(content);
    } else {
        content = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }

    int row = dataTable_->rowCount();
    dataTable_->insertRow(row);
    dataTable_->setItem(row, 0, new QTableWidgetItem(clientId));
    dataTable_->setItem(row, 1, new QTableWidgetItem(type));
    dataTable_->setItem(row, 2, new QTableWidgetItem(content));
    dataTable_->setItem(row, 3, new QTableWidgetItem(QTime::currentTime().toString()));
}

void MainWindow::onLogMessage(const QString &msg) {
    logView_->append(msg);
}

void MainWindow::onSendStartClients() {
    if (!serverManager_) { QMessageBox::warning(this, "Warning", "Server not running"); return; }
    QJsonObject cmd;
    cmd["type"] = "Command";
    cmd["command"] = "START";
    QMetaObject::invokeMethod(serverManager_, "broadcast", Qt::QueuedConnection, Q_ARG(QJsonObject, cmd));
    logView_->append("Sent START to all clients");
}

void MainWindow::onSendStopClients() {
    if (!serverManager_) { QMessageBox::warning(this, "Warning", "Server not running"); return; }
    QJsonObject cmd;
    cmd["type"] = "Command";
    cmd["command"] = "STOP";
    QMetaObject::invokeMethod(serverManager_, "broadcast", Qt::QueuedConnection, Q_ARG(QJsonObject, cmd));
    logView_->append("Sent STOP to all clients");
}

void MainWindow::onConfigureClients() {
    if (!serverManager_) { QMessageBox::warning(this, "Warning", "Server not running"); return; }
    // For simplicity show a quick dialog asking critical latency/packet loss (blocking)
    bool ok = false;
    int critLat = QInputDialog::getInt(this, "Config", "Critical latency ms:", 100, 0, 100000, 1, &ok);
    if (!ok) return;
    double critPL = QInputDialog::getDouble(this, "Config", "Critical packet loss (0..1):", 0.05, 0.0, 1.0, 4, &ok);
    if (!ok) return;
    QJsonObject cfg;
    cfg["type"] = "Config";
    cfg["crit_latency_ms"] = critLat;
    cfg["crit_packet_loss"] = critPL;
    QMetaObject::invokeMethod(serverManager_, "broadcast", Qt::QueuedConnection, Q_ARG(QJsonObject, cfg));
    logView_->append(QStringLiteral("Sent config: latency=%1 ms, packet_loss=%2").arg(critLat).arg(critPL));
}
