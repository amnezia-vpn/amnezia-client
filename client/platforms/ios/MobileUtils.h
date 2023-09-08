#ifndef MOBILEUTILS_H
#define MOBILEUTILS_H

#include <QObject>
#include <QStringList>

class MobileUtils : public QObject
{
    Q_OBJECT

//public:
//    MobileUtils() = delete;

public slots:
    static void shareText(const QStringList &filesToSend);
    QString openFile();
    
signals:
    void finished();
};

#endif // MOBILEUTILS_H
