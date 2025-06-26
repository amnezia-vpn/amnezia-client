#ifndef PROTOCOLS_MODEL_H
#define PROTOCOLS_MODEL_H

#include <QAbstractListModel>
#include <QJsonObject>

#include "core/models/protocols/protocolConfig.h"
#include "ui/controllers/pageController.h"
#include "ui/models/protocols/awgConfigModel.h"
#include "ui/models/protocols/cloakConfigModel.h"
#include "ui/models/protocols/openvpnConfigModel.h"
#include "ui/models/protocols/shadowsocksConfigModel.h"
#include "ui/models/protocols/wireguardConfigModel.h"
#include "ui/models/protocols/xrayConfigModel.h"
#ifdef Q_OS_WINDOWS
    #include "ui/models/protocols/ikev2ConfigModel.h"
#endif
#include "ui/models/services/sftpConfigModel.h"
#include "ui/models/services/socks5ProxyConfigModel.h"

class ProtocolsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ProtocolNameRole = Qt::UserRole + 1,
        ServerProtocolPageRole,
        ClientProtocolPageRole,
        ProtocolIndexRole,
        RawConfigRole,
        IsClientProtocolExistsRole
    };

    ProtocolsModel(QObject *parent = nullptr);
    ProtocolsModel(const QSharedPointer<OpenVpnConfigModel> &openVpnConfigModel,
                   const QSharedPointer<ShadowSocksConfigModel> &shadowSocksConfigModel,
                   const QSharedPointer<CloakConfigModel> &cloakConfigModel, const QSharedPointer<WireGuardConfigModel> &wireGuardConfigModel,
                   const QSharedPointer<AwgConfigModel> &awgConfigModel, const QSharedPointer<XrayConfigModel> &xrayConfigModel,
#ifdef Q_OS_WINDOWS
                   const QSharedPointer<Ikev2ConfigModel> &ikev2ConfigModel,
#endif
                   const QSharedPointer<SftpConfigModel> &sftpConfigModel,
                   const QSharedPointer<Socks5ProxyConfigModel> &socks5ProxyConfigModel, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QMap<QString, QSharedPointer<ProtocolConfig>> &protocolConfigs);
    void updateProtocolModel(amnezia::Proto protocol);

    QMap<QString, QSharedPointer<ProtocolConfig>> getProtocolConfigs();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    PageLoader::PageEnum serverProtocolPage(Proto protocol) const;
    PageLoader::PageEnum clientProtocolPage(Proto protocol) const;

    QVector<QSharedPointer<ProtocolConfig>> m_protocolConfigs;

    QSharedPointer<OpenVpnConfigModel> m_openVpnConfigModel;
    QSharedPointer<ShadowSocksConfigModel> m_shadowSocksConfigModel;
    QSharedPointer<CloakConfigModel> m_cloakConfigModel;
    QSharedPointer<WireGuardConfigModel> m_wireGuardConfigModel;
    QSharedPointer<AwgConfigModel> m_awgConfigModel;
    QSharedPointer<XrayConfigModel> m_xrayConfigModel;
#ifdef Q_OS_WINDOWS
    QSharedPointer<Ikev2ConfigModel> m_ikev2ConfigModel;
#endif
    QSharedPointer<SftpConfigModel> m_sftpConfigModel;
    QSharedPointer<Socks5ProxyConfigModel> m_socks5ProxyConfigModel;
};

#endif // PROTOCOLS_MODEL_H
