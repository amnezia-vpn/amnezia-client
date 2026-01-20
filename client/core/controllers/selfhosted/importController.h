#ifndef IMPORTCONTROLLER_H
#define IMPORTCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QByteArray>

#include "core/repositories/qServersRepository.h"
#include "core/repositories/qAppSettingsRepository.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

namespace
{
    enum class ConfigTypes {
        Amnezia,
        OpenVpn,
        WireGuard,
        Awg,
        Xray,
        Backup,
        Invalid
    };
}

using namespace amnezia;

class ImportController : public QObject
{
    Q_OBJECT

public:
    struct ImportResult
    {
        ErrorCode errorCode = ErrorCode::NoError;
        QJsonObject config;
        QString configFileName;
        QString maliciousWarningText;
        ConfigTypes configType = ConfigTypes::Invalid;
        bool isNativeWireGuardConfig = false;
    };

    explicit ImportController(QServersRepository* serversRepository,
                              QAppSettingsRepository* appSettingsRepository,
                              QObject *parent = nullptr);

    ImportResult extractConfigFromData(const QString &data, const QString &configFileName = "");
    ImportResult extractConfigFromQr(const QByteArray &data);

    void importConfig(const QJsonObject &config);
    QJsonObject processNativeWireGuardConfig(const QJsonObject &config);

signals:
    void importFinished();
    void importErrorOccurred(ErrorCode errorCode, bool goToPageHome);
    void restoreAppConfig(const QByteArray &data);

private:
    ConfigTypes checkConfigFormat(const QString &config) const;
    QJsonObject extractOpenVpnConfig(const QString &data) const;
    QJsonObject extractWireGuardConfig(const QString &data, ConfigTypes &configType) const;
    QJsonObject extractXrayConfig(const QString &data, ConfigTypes configType, const QString &description = "") const;
    void checkForMaliciousStrings(QJsonObject &serverConfig, QString &warningText) const;
    void processAmneziaConfig(QJsonObject &config) const;

    QServersRepository* m_serversRepository;
    QAppSettingsRepository* m_appSettingsRepository;
};

#endif // IMPORTCONTROLLER_H
