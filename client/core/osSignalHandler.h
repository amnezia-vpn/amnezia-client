#ifndef OSSIGNALHANDLER_H
#define OSSIGNALHANDLER_H

#include <QObject>

class OsSignalHandler : public QObject
{
    Q_OBJECT
public:
    static void setup();

private:
    explicit OsSignalHandler(QObject *parent = nullptr);
    static void handleSignal(int signal);
};

#endif // OSSIGNALHANDLER_H
