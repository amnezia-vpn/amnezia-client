#ifndef EXPORTCONTROLLER_H
#define EXPORTCONTROLLER_H

#include <QObject>

#include "configurators/vpn_configurator.h"
#include "ui/models/containers_model.h"
#include "ui/models/servers_model.h"

class ExportController : public QObject
{
    Q_OBJECT
public:
    explicit ExportController(const QSharedPointer<ServersModel> &serversModel,
                              const QSharedPointer<ContainersModel> &containersModel,
                              const std::shared_ptr<Settings> &settings,
                              const std::shared_ptr<VpnConfigurator> &configurator,
                              QObject *parent = nullptr);

public slots:
    void generateFullAccessConfig();
    void generateConnectionConfig();
    QString getAmneziaCode();
    QList<QString> getQrCodes();

    void saveFile();

signals:
    void generateConfig(bool isFullAccess);

private:
    QList<QString> generateQrCodeImageSeries(const QByteArray &data);
    QString svgToBase64(const QString &image);

    QSharedPointer<ServersModel> m_serversModel;
    QSharedPointer<ContainersModel> m_containersModel;
    std::shared_ptr<Settings> m_settings;
    std::shared_ptr<VpnConfigurator> m_configurator;

    QString m_amneziaCode;
    QList<QString> m_qrCodes;
};

#endif // EXPORTCONTROLLER_H
