#ifndef PROTECTOR_H
#define PROTECTOR_H

#include <QByteArray>
#include <QObject>
#include <QString>

class Protector : public QObject
{
    Q_OBJECT
public:
    explicit Protector(QObject *parent = nullptr);

    Q_INVOKABLE static void encryptFile(const QString &filePath, const QString &password);
    Q_INVOKABLE static void decryptFile(const QString &filePath, const QString &password);
    Q_INVOKABLE static void resetPassword(const QString &filePath, const QString &password);

private:
    static QByteArray deriveKey(const QString &password, const QByteArray &salt);
    static QByteArray generateSalt();
};

#endif // PROTECTOR_H
