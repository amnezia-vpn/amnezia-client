#ifndef SERVER_H
#define SERVER_H

#include <QCoreApplication>
#include <QTcpServer>

#include "qtservice.h"


class HttpDaemon : public QTcpServer
{
    Q_OBJECT
public:
    HttpDaemon(quint16 port, QObject* parent = 0);
    void sendFortune();

    void pause();
    void resume();

private slots:
    void readClient();
    void discardClient();
private:
    bool disabled;
};


class HttpService : public QtService<QCoreApplication>
{
public:
    HttpService(int argc, char **argv);

protected:
    void pause();
    void resume();
    void start();

private:
    HttpDaemon *daemon;
};


#endif // SERVER_H
