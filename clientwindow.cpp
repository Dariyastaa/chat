#include "clientwindow.h"
#include "ui_clientwindow.h"

ClientWindow::ClientWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ClientWindow)
{
    ui->setupUi(this);

    socket = new QTcpSocket(this);

    connect(ui->connectButton, &QPushButton::clicked, this, &ClientWindow::connectToServer);
    connect(ui->sendButton, &QPushButton::clicked, this, &ClientWindow::sendMessage);
    connect(ui->input, &QLineEdit::returnPressed, this, &ClientWindow::sendMessage);

    connect(socket, &QTcpSocket::connected, this, &ClientWindow::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &ClientWindow::readMessage);
    connect(socket, &QTcpSocket::disconnected, this, &ClientWindow::onDisconnected);

    ui->sendButton->setEnabled(false);
}

ClientWindow::~ClientWindow()
{
    delete ui;
}

void ClientWindow::connectToServer()
{
    socket->connectToHost(ui->ipEdit->text(), 12345);
}

void ClientWindow::onConnected()
{
    ui->log->append("Подключено");
    ui->sendButton->setEnabled(true);
}

void ClientWindow::readMessage()
{
    QByteArray data = socket->readAll();
    ui->log->append("Сервер: " + QString::fromUtf8(data));
}

void ClientWindow::sendMessage()
{
    QString text = ui->input->text().trimmed();
    if (text.isEmpty()) return;

    socket->write(text.toUtf8());
    ui->log->append("Клиент: " + text);
    ui->input->clear();
}

void ClientWindow::onDisconnected()
{
    ui->log->append("Отключено");
    ui->sendButton->setEnabled(false);
}