#ifndef IMPORTCONTROLLER_H
#define IMPORTCONTROLLER_H

#include <QObject>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"

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

#if defined Q_OS_ANDROID || defined Q_OS_IOS
    void startDecodingQr();
    bool parseQrCodeChunk(const QString &code);

    double getQrCodeScanProgressBarValue();
    QString getQrCodeScanProgressString();
#endif

#if defined Q_OS_ANDROID
    static bool decodeQrCode(const QString &code);
#endif

signals:
    void importFinished();
    void importErrorOccurred(const QString &errorMessage, bool goToPageHome);

    void qrDecodingFinished();

    void restoreAppConfig(const QByteArray &data);

private:
    QJsonObject extractOpenVpnConfig(const QString &data);
    QJsonObject extractWireGuardConfig(const QString &data);

#if defined Q_OS_ANDROID || defined Q_OS_IOS
    void stopDecodingQr();
#endif

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    std::shared_ptr<Settings> m_settings;

    QJsonObject m_config;
    QString m_configFileName;

#if defined Q_OS_ANDROID || defined Q_OS_IOS
    QMap<int, QByteArray> m_qrCodeChunks;
    bool m_isQrCodeProcessed;
    int m_totalQrCodeChunksCount;
    int m_receivedQrCodeChunksCount;
#endif
};

#endif // IMPORTCONTROLLER_H
