#ifndef ALLOWEDDNSMODEL_H
#define ALLOWEDDNSMODEL_H

#include <QAbstractListModel>
#include <QStringList>

class AllowedDnsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IpRole = Qt::UserRole + 1
    };

    explicit AllowedDnsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QStringList &dnsServers);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QStringList m_dnsServers;
};

#endif // ALLOWEDDNSMODEL_H
