#ifndef IMPORTCONTROLLER_H
#define IMPORTCONTROLLER_H

#include <QObject>
#include <QJsonObject>
#include <QByteArray>
#include <QMap>

#include "core/repositories/secureServersRepository.h"
#include "core/repositories/secureAppSettingsRepository.h"
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
        ShadowSocks,
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

    explicit ImportController(SecureServersRepository* serversRepository,
                              SecureAppSettingsRepository* appSettingsRepository,
                              QObject *parent = nullptr);

    struct QrParseResult {
        bool success = false;
        ImportResult importResult;
        int chunksReceived = 0;
        int chunksTotal = 0;
    };

    ImportResult extractConfigFromData(const QString &data, const QString &configFileName = "");
    ImportResult extractConfigFromQr(const QByteArray &data);

    void startDecodingQr();
    QrParseResult parseQrCodeChunk(const QString &code);
    bool isQrDecodingActive() const;
    int qrChunksReceived() const;
    int qrChunksTotal() const;

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
    void checkForMaliciousStrings(const QJsonObject &serverConfig, QString &warningText) const;
    void processAmneziaConfig(QJsonObject &config) const;

    SecureServersRepository* m_serversRepository;
    SecureAppSettingsRepository* m_appSettingsRepository;

    QMap<int, QByteArray> m_qrCodeChunks;
    bool m_isQrCodeProcessed = false;
    int m_totalQrCodeChunksCount = 0;
    int m_receivedQrCodeChunksCount = 0;
};

#endif // IMPORTCONTROLLER_H
