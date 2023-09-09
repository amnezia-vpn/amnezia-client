#ifndef MOBILEUTILS_H
#define MOBILEUTILS_H

#include <QObject>
#include <QStringList>

class MobileUtils : public QObject
{
    Q_OBJECT
    
public:
    explicit MobileUtils(QObject *parent = nullptr);

public slots:
    bool shareText(const QStringList &filesToSend);
    QString openFile();
    
signals:
    void finished();
};

#endif // MOBILEUTILS_H
