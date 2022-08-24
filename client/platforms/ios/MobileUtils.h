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

    static void writeToKeychain(const QString& tag, const QByteArray& value);
    static bool deleteFromKeychain(const QString& tag);
    static QByteArray readFromKeychain(const QString& tag);
};

#endif // MOBILEUTILS_H
