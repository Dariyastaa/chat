#pragma once

#include <QMainWindow>
#include <QTcpSocket>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class ClientWindow; }
QT_END_NAMESPACE

class ClientWindow : public QMainWindow
{
    Q_OBJECT

public:
    ClientWindow(QWidget *parent = nullptr);
    ~ClientWindow();

private slots:
    void connectToServer();
    void onConnected();
    void readMessage();
    void sendMessage();
    void onDisconnected();
    void sendImage();
    void askNickname();

private:
    Ui::ClientWindow *ui;
    QTcpSocket *socket;
    QString username;
    QByteArray incomingImageData;
    QString incomingSender;
    bool receivingImage = false;
    QByteArray buffer;
    bool nicknameAccepted = false;
    bool waitingNickReply = false;
};