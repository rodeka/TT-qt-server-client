#pragma once
#include <QMainWindow>
#include <QThread>

QT_BEGIN_NAMESPACE
class QTableWidget;
class QPushButton;
class QTextEdit;
class QLabel;
QT_END_NAMESPACE

class ServerManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStartServer();
    void onStopServer();
    void onClientConnected(const QString &clientId, const QString &ip, quint16 port);
    void onClientDisconnected(const QString &clientId);
    void onDataReceived(const QString &clientId, const QJsonObject &obj);
    void onLogMessage(const QString &msg);
    void onSendStartClients();
    void onSendStopClients();
    void onConfigureClients();

private:
    void setupUi();

    QTableWidget *clientsTable_;
    QTableWidget *dataTable_;
    QPushButton *startServerBtn_;
    QPushButton *stopServerBtn_;
    QPushButton *startClientsBtn_;
    QPushButton *stopClientsBtn_;
    QPushButton *configBtn_;
    QTextEdit *logView_;
    QLabel *statusLabel_;

    ServerManager *serverManager_;
    QThread *serverThread_;
    quint16 listenPort_;
};


