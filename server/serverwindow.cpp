#include "serverwindow.h"
#include "ui_serverwindow.h"
#include <QHostAddress>

ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ServerWindow)
{
    ui->setupUi(this);
    setWindowTitle("Сервер");

    server = new QTcpServer(this);

    connect(ui->startButton, &QPushButton::clicked, this, &ServerWindow::startServer);
    connect(server, &QTcpServer::newConnection, this, &ServerWindow::newConnection);
}

ServerWindow::~ServerWindow()
{
    delete ui;
}

void ServerWindow::startServer()
{
    if (server->listen(QHostAddress::Any, 12345))
    {
        ui->log->append("Сервер запущен на порту 12345");
        ui->startButton->setEnabled(false);
    }
    else
    {
        ui->log->append("Ошибка запуска сервера");
    }
}

void ServerWindow::newConnection()
{
    while (server->hasPendingConnections())
    {
        QTcpSocket *clientSocket = server->nextPendingConnection();
        clients.append(clientSocket);

        ui->log->append("Новое подключение: " + clientSocket->peerAddress().toString());

        connect(clientSocket, &QTcpSocket::readyRead, this, &ServerWindow::readMessage);
        connect(clientSocket, &QTcpSocket::disconnected, this, &ServerWindow::clientDisconnected);
    }
}

void ServerWindow::readMessage()
{
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket)
        return;

    QByteArray data = senderSocket->readAll();
    QString message = QString::fromUtf8(data).trimmed();

    if (message.isEmpty())
        return;

    if (message.startsWith("/nick "))
    {
        QString nick = message.mid(6).trimmed();
        if (nick.isEmpty())
            nick = "Гость";

        nicknames[senderSocket] = nick;

        QString info = nick + " подключился";
        ui->log->append(info);
        broadcastMessage(info);
        return;
    }

    ui->log->append(message);
    broadcastMessage(message);
}

void ServerWindow::broadcastMessage(const QString &message)
{
    QByteArray data = message.toUtf8();

    for (QTcpSocket *client : clients)
    {
        if (client && client->state() == QAbstractSocket::ConnectedState)
        {
            client->write(data);
        }
    }
}

void ServerWindow::clientDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket)
        return;

    QString nick = nicknames.value(clientSocket, "Неизвестный клиент");
    QString info = nick + " отключился";

    ui->log->append(info);
    broadcastMessage(info);

    clients.removeAll(clientSocket);
    nicknames.remove(clientSocket);
    clientSocket->deleteLater();
}