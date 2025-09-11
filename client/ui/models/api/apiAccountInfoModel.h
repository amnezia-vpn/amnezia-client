#ifndef APIACCOUNTINFOMODEL_H
#define APIACCOUNTINFOMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>

#include "core/api/apiDefs.h"

class ApiAccountInfoModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        SubscriptionStatusRole = Qt::UserRole + 1,
        ConnectedDevicesRole,
        ServiceDescriptionRole,
        EndDateRole,
        IsComponentVisibleRole,
        HasExpiredWorkerRole,
        IsProtocolSelectionSupportedRole
    };

    explicit ApiAccountInfoModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QJsonObject &accountInfoObject, const QJsonObject &serverConfig);
    QVariant data(const QString &roleString);

    QJsonArray getAvailableCountries();
    QJsonArray getIssuedConfigsInfo();

    QString getTelegramBotLink();
    QString getEmailLink();
    QString getBillingEmailLink();
    QString getSiteLink();
    QString getFullSiteLink();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    struct AccountInfoData
    {
        QString subscriptionEndDate;
        int activeDeviceCount;
        int maxDeviceCount;

        apiDefs::ConfigType configType;

        QStringList supportedProtocols;
    };

    AccountInfoData m_accountInfoData;
    QJsonArray m_availableCountries;
    QJsonArray m_issuedConfigsInfo;
    QJsonObject m_supportInfo;
};

#endif // APIACCOUNTINFOMODEL_H
