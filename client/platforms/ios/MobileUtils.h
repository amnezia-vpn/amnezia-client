#ifndef MOBILEUTILS_H
#define MOBILEUTILS_H

#include <QObject>
#include <QDebug>
#include <QEventLoop>
#include <QString>
#include <QStringList>

class MobileUtils : public QObject
{
    Q_OBJECT
    
public:
    explicit MobileUtils(QObject *parent = nullptr);

public slots:
    bool shareText(const QStringList &filesToSend);
    QString openFile();
    void requestInetAccess();
    
signals:
    void finished();
};

#endif // MOBILEUTILS_H
