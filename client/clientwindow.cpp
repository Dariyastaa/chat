#include "clientwindow.h"
#include "ui_clientwindow.h"
#include <QInputDialog>
#include <QLineEdit>

ClientWindow::ClientWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ClientWindow)
{
    ui->setupUi(this);

    bool ok;
    username = QInputDialog::getText(
        this,
        "Ввод ника",
        "Введите ваш ник:",
        QLineEdit::Normal,
        "",
        &ok
        );

    username = username.trimmed();

    if (!ok || username.isEmpty())
    {
        username = "Гость";
    }

    setWindowTitle("Клиент: " + username);

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
    ui->log->append("Подключено к серверу");
    ui->sendButton->setEnabled(true);

    QString nickMessage = "/nick " + username;
    socket->write(nickMessage.toUtf8());
}

void ClientWindow::readMessage()
{
    QByteArray data = socket->readAll();
    QString message = QString::fromUtf8(data);
    ui->log->append(message);
}

void ClientWindow::sendMessage()
{
    QString text = ui->input->text().trimmed();

    if (text.isEmpty())
        return;

    QString message = username + ": " + text;
    socket->write(message.toUtf8());

    ui->input->clear();
}

void ClientWindow::onDisconnected()
{
    ui->log->append("Отключено от сервера");
    ui->sendButton->setEnabled(false);
}