#include "secure_qsettings.h"
#include "secureformat.h"

#include <QDataStream>

SecureQSettings::SecureQSettings(const QString &organization, const QString &application, QObject *parent)
    : QObject{parent},
      m_setting(organization, application, parent)
{
    encrypted = m_setting.value("encrypted").toBool();

    // convert settings to encrypted
    if (! encrypted) {
        // TODO: convert
        // m_setting.sync();
    }
}

QVariant SecureQSettings::value(const QString &key, const QVariant &defaultValue) const
{
    if (encrypted) {
        QByteArray encryptedValue = m_setting.value(key, defaultValue).toByteArray();
        QByteArray decryptedValue = decryptText(encryptedValue);

        QDataStream ds(&decryptedValue, QIODevice::ReadOnly);
        QVariant v;
        ds >> v;
        return v;
    }
    else {
        return m_setting.value(key, defaultValue);
    }
}

void SecureQSettings::setValue(const QString &key, const QVariant &value)
{
    QByteArray decryptedValue;
    {
        QDataStream ds(&decryptedValue, QIODevice::WriteOnly);
        ds << value;
    }

    QByteArray encryptedValue = encryptText(decryptedValue);
    m_setting.setValue(key, encryptedValue);
}


