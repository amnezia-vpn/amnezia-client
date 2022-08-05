#ifndef SECUREFORMAT_H
#define SECUREFORMAT_H

#include <QSettings>
#include <QIODevice>

QByteArray encryptText(const QByteArray &value);
QByteArray decryptText(const QByteArray& qEncryptArray);

class SecureFormat
{
public:
    SecureFormat();




};

#endif // SECUREFORMAT_H
