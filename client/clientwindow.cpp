#include "clientwindow.h"
#include "ui_clientwindow.h"
#include <QInputDialog>
#include <QLineEdit>
#include <QFileDialog>
#include <QFile>
#include <QPixmap>
#include <QTextCursor>
#include <QTextDocument>
#include <QDateTime>
#include <QUrl>
#include <QPixmap>
#include <QMessageBox>
#include <QEventLoop>
#include <QTimer>

ClientWindow::ClientWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::ClientWindow)
{
    ui->setupUi(this);
    setWindowTitle("Клиент");

    socket = new QTcpSocket(this);

    connect(ui->connectButton, &QPushButton::clicked, this, &ClientWindow::connectToServer);
    connect(ui->sendButton, &QPushButton::clicked, this, &ClientWindow::sendMessage);
    connect(ui->input, &QLineEdit::returnPressed, this, &ClientWindow::sendMessage);
    connect(ui->sendImageButton, &QPushButton::clicked, this, &ClientWindow::sendImage);

    connect(socket, &QTcpSocket::connected, this, &ClientWindow::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &ClientWindow::readMessage);
    connect(socket, &QTcpSocket::disconnected, this, &ClientWindow::onDisconnected);

    ui->sendButton->setEnabled(false);
    ui->sendImageButton->setEnabled(false);
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
    askNickname();
}

void ClientWindow::askNickname()
{
    while (true)
    {
        bool ok;
        QString nick = QInputDialog::getText(
                           this,
                           "Ввод ника",
                           "Введите уникальный ник:",
                           QLineEdit::Normal,
                           "",
                           &ok
                           ).trimmed();

        if (!ok)
        {
            socket->disconnectFromHost();
            close();
            return;
        }

        if (nick.isEmpty())
        {
            QMessageBox::warning(this, "Ошибка", "Ник не может быть пустым");
            continue;
        }

        username = nick;
        nicknameAccepted = false;
        waitingNickReply = true;

        QString packet = "CHECK_NICK|" + username + "\n";
        socket->write(packet.toUtf8());

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);

        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(socket, &QTcpSocket::readyRead, &loop, &QEventLoop::quit);

        timer.start(3000);
        loop.exec();

        if (!waitingNickReply && nicknameAccepted)
        {
            QString regPacket = "NICK|" + username + "\n";
            socket->write(regPacket.toUtf8());

            setWindowTitle("Клиент: " + username);
            ui->sendButton->setEnabled(true);
            ui->sendImageButton->setEnabled(true);
            break;
        }

        if (!waitingNickReply && !nicknameAccepted)
        {
            QMessageBox::warning(this, "Ошибка", "Ник уже занят");
            continue;
        }

        QMessageBox::warning(this, "Ошибка", "Сервер не ответил");
    }
}

void ClientWindow::readMessage()
{
    buffer.append(socket->readAll());

    while (true)
    {
        int nl = buffer.indexOf('\n');
        if (nl == -1)
            break;

        QByteArray rawLine = buffer.left(nl);
        buffer.remove(0, nl + 1);

        QString line = QString::fromUtf8(rawLine);

        if (line == "NICK_OK")
        {
            waitingNickReply = false;
            nicknameAccepted = true;
            continue;
        }

        if (line == "NICK_TAKEN")
        {
            waitingNickReply = false;
            nicknameAccepted = false;
            continue;
        }

        if (line == "NICK_EMPTY")
        {
            waitingNickReply = false;
            nicknameAccepted = false;
            continue;
        }

        if (line.startsWith("TEXT|"))
        {
            QString payload = line.mid(5);
            ui->log->append(payload);
            continue;
        }

        if (line.startsWith("INFO|"))
        {
            QString payload = line.mid(5);
            ui->log->append(payload);
            continue;
        }

        if (line.startsWith("IMG_START|"))
        {
            incomingSender = line.mid(QString("IMG_START|").length());
            incomingImageData.clear();
            receivingImage = true;
            continue;
        }

        if (line.startsWith("IMG_CHUNK|") && receivingImage)
        {
            QString base64 = line.mid(QString("IMG_CHUNK|").length());
            incomingImageData.append(QByteArray::fromBase64(base64.toUtf8()));
            continue;
        }

        if (line == "IMG_END" && receivingImage)
        {
            receivingImage = false;

            ui->log->append(incomingSender + " отправил картинку:");

            QString html =
                "<img src='data:image/png;base64,"
                + incomingImageData.toBase64()
                + "' width='250'/>";

            ui->log->append(html);
            continue;
        }
    }
}

void ClientWindow::sendMessage()
{
    QString text = ui->input->text().trimmed();
    if (text.isEmpty())
        return;

    QString packet = "TEXT|" + username + "|" + text + "\n";
    socket->write(packet.toUtf8());

    ui->input->clear();
}

void ClientWindow::sendImage()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Выбрать изображение",
        "",
        "Images (*.png *.jpg *.jpeg *.bmp)"
        );

    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QByteArray imageData = file.readAll();

    socket->write(("IMG_START|" + username + "\n").toUtf8());

    const int chunkSize = 3000;
    for (int i = 0; i < imageData.size(); i += chunkSize)
    {
        QByteArray chunk = imageData.mid(i, chunkSize).toBase64();
        socket->write("IMG_CHUNK|");
        socket->write(chunk);
        socket->write("\n");
    }

    socket->write("IMG_END\n");
    ui->log->append("Вы отправили картинку");
}

void ClientWindow::onDisconnected()
{
    ui->log->append("Отключено от сервера");
    ui->sendButton->setEnabled(false);
}