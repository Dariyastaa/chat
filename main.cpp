#include "serverwindow.h"
#include "clientwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    ServerWindow server;
    ClientWindow client;

    server.show();
    client.show();

    return a.exec();
}