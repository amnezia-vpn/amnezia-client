#ifndef IMPORTCONTROLLER_H
#define IMPORTCONTROLLER_H

#include <QObject>

#include "core/controllers/serversController.h"
#include "core/repositories/qAppSettingsRepository.h"
#include "ui/models/containersModel.h"
#include "ui/models/serversModel.h"

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

class ImportController : public QObject
{
    Q_OBJECT
public:
    explicit ImportController(ServersController* serversController,
                              ServersModel* serversModel,
                              ContainersModel* containersModel,
                              QAppSettingsRepository* appSettingsRepository,
                              QObject *parent = nullptr);

public slots:
    void importConfig();
    void clearConfigFileName();
    bool extractConfigFromFile(const QString &fileName);
    bool extractConfigFromData(QString data);
    bool extractConfigFromQr(const QByteArray &data);
    QString getConfig();
    QString getConfigFileName();
    QString getMaliciousWarningText();

#if defined Q_OS_ANDROID || defined Q_OS_IOS
    void startDecodingQr();
    bool parseQrCodeChunk(const QString &code);

    double getQrCodeScanProgressBarValue();
    QString getQrCodeScanProgressString();
#endif

#if defined Q_OS_ANDROID
    static bool decodeQrCode(const QString &code);
#endif

    bool isNativeWireGuardConfig();
    void processNativeWireGuardConfig();

signals:
    void importFinished();
    void importErrorOccurred(ErrorCode errorCode, bool goToPageHome);

    void qrDecodingFinished();

    void restoreAppConfig(const QByteArray &data);

private:
    QJsonObject extractOpenVpnConfig(const QString &data);
    QJsonObject extractWireGuardConfig(const QString &data);
    QJsonObject extractXrayConfig(const QString &data, const QString &description = "");

    void checkForMaliciousStrings(const QJsonObject &protocolConfig);

    void processAmneziaConfig(QJsonObject &config);

#if defined Q_OS_ANDROID || defined Q_OS_IOS
    void stopDecodingQr();
#endif

    ServersController* m_serversController;
    ServersModel* m_serversModel;
    ContainersModel* m_containersModel;
    QAppSettingsRepository* m_appSettingsRepository;

    QJsonObject m_config;
    QString m_configFileName;
    ConfigTypes m_configType;
    QString m_maliciousWarningText;

#if defined Q_OS_ANDROID || defined Q_OS_IOS
    QMap<int, QByteArray> m_qrCodeChunks;
    bool m_isQrCodeProcessed;
    int m_totalQrCodeChunksCount;
    int m_receivedQrCodeChunksCount;
#endif
};

#endif // IMPORTCONTROLLER_H
