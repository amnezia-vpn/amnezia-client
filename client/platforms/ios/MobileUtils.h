#ifndef MOBILEUTILS_H
#define MOBILEUTILS_H

#include <QObject>
#include <QStringList>

class MobileUtils : public QObject {
    Q_OBJECT

public:
    MobileUtils() = delete;

public slots:
    static void shareText(const QStringList& filesToSend);

    static void writeToKeychain(const QString& tag, const QString& value);
    static QString readFromKeychain(const QString& tag);
};

#endif // MOBILEUTILS_H
