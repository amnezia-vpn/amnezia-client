#ifndef SECUREFORMAT_H
#define SECUREFORMAT_H

#include <QSettings>
#include <QIODevice>

class SecureFormat
{
public:
    SecureFormat();

    static bool readSecureFile(QIODevice &device, QSettings::SettingsMap &map);
    static bool writeSecureFile(QIODevice &device, const QSettings::SettingsMap &map);

    static void chiperSettings(const QSettings &oldSetting, QSettings &newSetting);

    const QSettings::Format& format() const;

private:
    QSettings::Format m_format;
};

#endif // SECUREFORMAT_H
