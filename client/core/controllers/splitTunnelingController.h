#ifndef SPLITTUNNELINGCONTROLLER_H
#define SPLITTUNNELINGCONTROLLER_H

#include <QObject>

class SplitTunnelingController : public QObject
{
    Q_OBJECT

public:
    explicit SplitTunnelingController(QObject *parent = nullptr);
};

#endif // SPLITTUNNELINGCONTROLLER_H 
