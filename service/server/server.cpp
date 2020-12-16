#include <QDateTime>
#include <QDebug>
#include <QTcpSocket>

#include "server.h"

HttpDaemon::HttpDaemon(quint16 port, QObject* parent)
    : QTcpServer(parent), disabled(false)
{
    listen(QHostAddress::Any, port);
    qDebug() << "Listen on port: " << port;

    connect(this, &QTcpServer::newConnection, this, &HttpDaemon::sendFortune);
}

void HttpDaemon::sendFortune()
{
    qDebug() << "New connection: ";

    QTcpSocket *clientConnection = this->nextPendingConnection();
    connect(clientConnection, &QAbstractSocket::disconnected,
            clientConnection, &QObject::deleteLater);


    connect(clientConnection, SIGNAL(readyRead()), this, SLOT(readClient()));
    connect(clientConnection, SIGNAL(disconnected()), this, SLOT(discardClient()));
    //->setSocketDescriptor(socket);
}

void HttpDaemon::pause()
{
    disabled = true;
}

void HttpDaemon::resume()
{
    disabled = false;
}

void HttpDaemon::readClient()
{
    qDebug() << "readClient";

    // if (disabled)
    //     return;
    //
    // This slot is called when the client sent data to the server. The
    // server looks if it was a get request and sends a very simple HTML
    // document back.
    QTcpSocket* socket = (QTcpSocket*)sender();
    if (socket->canReadLine()) {
        QStringList tokens = QString(socket->readLine()).split(QRegExp("[ \r\n][ \r\n]*"));
        if (tokens[0] == "GET") {
            QTextStream os(socket);
            os.setAutoDetectUnicode(true);
            os << "HTTP/1.0 200 Ok\r\n"
                  "Content-Type: text/html; charset=\"utf-8\"\r\n"
                  "\r\n"
                  "<h1>Nothing to see here</h1>\n"
               << QDateTime::currentDateTime().toString() << "\n";
            socket->close();

            QtServiceBase::instance()->logMessage("Wrote to client");

            if (socket->state() == QTcpSocket::UnconnectedState) {
                delete socket;
                QtServiceBase::instance()->logMessage("Connection closed");
            }
        }
    }
}

void HttpDaemon::discardClient()
{
    QTcpSocket* socket = (QTcpSocket*)sender();
    socket->deleteLater();

    QtServiceBase::instance()->logMessage("Connection closed");
}






HttpService::HttpService(int argc, char **argv)
    : QtService<QCoreApplication>(argc, argv, "Qt HTTP Daemon")
{
    setServiceDescription("A dummy HTTP service implemented with Qt");
    setServiceFlags(QtServiceBase::CanBeSuspended);
}

void HttpService::start()
{
    QCoreApplication *app = application();
    daemon = new HttpDaemon(8989, app);

    if (!daemon->isListening()) {
        logMessage(QString("Failed to bind to port %1").arg(daemon->serverPort()), QtServiceBase::Error);
        app->quit();
    }
}

void HttpService::pause()
{
    daemon->pause();
}

void HttpService::resume()
{
    daemon->resume();
}
