#pragma once

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class ServerWindow; }
QT_END_NAMESPACE

class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow();

private slots:
    void startServer();
    void newConnection();
    void readMessage();
    void sendMessage();
    void clientDisconnected();

private:
    Ui::ServerWindow *ui;
    QTcpServer *server;
    QTcpSocket *socket;
};