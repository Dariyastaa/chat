#include "serverwindow.h"
#include "ui_serverwindow.h"
#include <QHostAddress>

ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ServerWindow), socket(nullptr)
{
    ui->setupUi(this);

    server = new QTcpServer(this);

    connect(ui->startButton, &QPushButton::clicked, this, &ServerWindow::startServer);
    connect(ui->sendButton, &QPushButton::clicked, this, &ServerWindow::sendMessage);
    connect(ui->input, &QLineEdit::returnPressed, this, &ServerWindow::sendMessage);
    connect(server, &QTcpServer::newConnection, this, &ServerWindow::newConnection);

    ui->sendButton->setEnabled(false);
}

ServerWindow::~ServerWindow()
{
    delete ui;
}

void ServerWindow::startServer()
{
    if (server->listen(QHostAddress::Any, 12345))
    {
        ui->log->append("Сервер запущен");
        ui->startButton->setEnabled(false);
    }
    else
    {
        ui->log->append("Ошибка запуска");
    }
}

void ServerWindow::newConnection()
{
    socket = server->nextPendingConnection();
    ui->log->append("Клиент подключился");
    ui->sendButton->setEnabled(true);

    connect(socket, &QTcpSocket::readyRead, this, &ServerWindow::readMessage);
    connect(socket, &QTcpSocket::disconnected, this, &ServerWindow::clientDisconnected);
}

void ServerWindow::readMessage()
{
    QByteArray data = socket->readAll();
    ui->log->append("Клиент: " + QString::fromUtf8(data));
}

void ServerWindow::sendMessage()
{
    if (!socket) return;

    QString text = ui->input->text().trimmed();
    if (text.isEmpty()) return;

    socket->write(text.toUtf8());
    ui->log->append("Сервер: " + text);
    ui->input->clear();
}

void ServerWindow::clientDisconnected()
{
    ui->log->append("Клиент отключился");
    ui->sendButton->setEnabled(false);
    socket->deleteLater();
    socket = nullptr;
}