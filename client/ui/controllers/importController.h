#ifndef IMPORTCONTROLLER_H
#define IMPORTCONTROLLER_H

#include <QObject>

#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"

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
    explicit ImportController(const QSharedPointer<ServersModel> &serversModel,
                              const QSharedPointer<ContainersModel> &containersModel,
                              const std::shared_ptr<Settings> &settings, QObject *parent = nullptr);

public slots:
    void importConfig();
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

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    std::shared_ptr<Settings> m_settings;

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
