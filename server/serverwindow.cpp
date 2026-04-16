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

        connect(clientSocket, &QTcpSocket::readyRead, this, &ServerWindow::readMessage);
        connect(clientSocket, &QTcpSocket::disconnected, this, &ServerWindow::clientDisconnected);
    }
}

bool ServerWindow::isNickTaken(const QString &nick)
{
    for (auto it = nicknames.begin(); it != nicknames.end(); ++it)
    {
        if (it.value().compare(nick, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

void ServerWindow::readMessage()
{
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket)
        return;

    buffers[senderSocket].append(senderSocket->readAll());

    while (true)
    {
        int nl = buffers[senderSocket].indexOf('\n');
        if (nl == -1)
            break;

        QByteArray rawLine = buffers[senderSocket].left(nl);
        buffers[senderSocket].remove(0, nl + 1);

        QString line = QString::fromUtf8(rawLine);

        if (line.startsWith("CHECK_NICK|"))
        {
            QString nick = line.mid(QString("CHECK_NICK|").length()).trimmed();

            if (nick.isEmpty())
            {
                senderSocket->write("NICK_EMPTY\n");
                continue;
            }

            if (isNickTaken(nick))
            {
                senderSocket->write("NICK_TAKEN\n");
            }
            else
            {
                senderSocket->write("NICK_OK\n");
            }

            continue;
        }

        if (line.startsWith("NICK|"))
        {
            QString nick = line.mid(5).trimmed();

            if (nick.isEmpty())
            {
                senderSocket->write("INFO|Ник не может быть пустым\n");
                senderSocket->disconnectFromHost();
                continue;
            }

            if (isNickTaken(nick))
            {
                senderSocket->write("INFO|Ник уже занят\n");
                senderSocket->disconnectFromHost();
                continue;
            }

            nicknames[senderSocket] = nick;

            QString info = nick + " подключился";
            ui->log->append(info);
            broadcastLine("INFO|" + info + "\n");
            continue;
        }

        if (line.startsWith("TEXT|"))
        {
            int sep = line.indexOf('|', 5);
            if (sep == -1)
                continue;

            QString nick = line.mid(5, sep - 5);
            QString text = line.mid(sep + 1).trimmed();

            if (text.startsWith("/private"))
            {
                QString rest = text.mid(QString("/private").length()).trimmed();

                if (rest.isEmpty())
                {
                    if (senderSocket && senderSocket->state() == QAbstractSocket::ConnectedState)
                        senderSocket->write("INFO|Неверный формат. Используйте: /private ник сообщение\n");
                    continue;
                }

                int spaceIndex = rest.indexOf(' ');
                if (spaceIndex == -1)
                {
                    if (senderSocket && senderSocket->state() == QAbstractSocket::ConnectedState)
                        senderSocket->write("INFO|Неверный формат. Используйте: /private ник сообщение\n");
                    continue;
                }

                QString targetNick = rest.left(spaceIndex).trimmed();
                QString privateText = rest.mid(spaceIndex + 1).trimmed();

                if (targetNick.isEmpty() || privateText.isEmpty())
                {
                    if (senderSocket && senderSocket->state() == QAbstractSocket::ConnectedState)
                        senderSocket->write("INFO|Неверный формат. Используйте: /private ник сообщение\n");
                    continue;
                }

                QTcpSocket *targetSocket = findClientByNick(targetNick);

                if (!targetSocket)
                {
                    if (senderSocket && senderSocket->state() == QAbstractSocket::ConnectedState)
                        senderSocket->write("INFO|Пользователь не найден\n");
                    continue;
                }

                QString msgToTarget = "TEXT|(ЛС от " + nick + ") " + privateText + "\n";
                QString msgToSender = "TEXT|(ЛС для " + targetNick + ") " + privateText + "\n";

                targetSocket->write(msgToTarget.toUtf8());

                if (senderSocket && senderSocket->state() == QAbstractSocket::ConnectedState)
                    senderSocket->write(msgToSender.toUtf8());

                ui->log->append("(ЛС) " + nick + " -> " + targetNick + ": " + privateText);
                continue;
            }

            QString out = nick + ": " + text;
            ui->log->append(out);
            broadcastLine("TEXT|" + out + "\n");
            continue;
        }

        if (line.startsWith("IMG_START|"))
        {
            QString nick = line.mid(QString("IMG_START|").length()).trimmed();
            imageSenders[senderSocket] = nick;
            receivingImageMap[senderSocket] = true;

            broadcastLine("IMG_START|" + nick + "\n");
            continue;
        }

        if (line.startsWith("IMG_CHUNK|") && receivingImageMap.value(senderSocket, false))
        {
            broadcastLine(line + "\n");
            continue;
        }

        if (line == "IMG_END" && receivingImageMap.value(senderSocket, false))
        {
            receivingImageMap[senderSocket] = false;
            broadcastLine("IMG_END\n");
            continue;
        }
    }
}

QTcpSocket* ServerWindow::findClientByNick(const QString &nick)
{
    for (auto it = nicknames.begin(); it != nicknames.end(); ++it)
    {
        if (it.value() == nick)
            return it.key();
    }
    return nullptr;
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

void ServerWindow::broadcastLine(const QString &line)
{
    QByteArray data = line.toUtf8();

    for (QTcpSocket *client : clients)
    {
        if (client && client->state() == QAbstractSocket::ConnectedState)
            client->write(data);
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
    broadcastLine("INFO|" + info + "\n");

    clients.removeAll(clientSocket);
    nicknames.remove(clientSocket);
    buffers.remove(clientSocket);
    receivingImageMap.remove(clientSocket);
    imageSenders.remove(clientSocket);

    clientSocket->deleteLater();
}