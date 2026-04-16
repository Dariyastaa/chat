#pragma once

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QMap>

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
    void clientDisconnected();

private:
    void broadcastMessage(const QString &message);
    void broadcastLine(const QString &line);
    QTcpSocket* findClientByNick(const QString &nick);
    bool isNickTaken(const QString &nick);

    Ui::ServerWindow *ui;
    QTcpServer *server;
    QList<QTcpSocket*> clients;
    QMap<QTcpSocket*, QString> nicknames;
    QMap<QTcpSocket*, QByteArray> buffers;
    QMap<QTcpSocket*, QString> imageSenders;
    QMap<QTcpSocket*, bool> receivingImageMap;
    bool receivingImage = false;
};