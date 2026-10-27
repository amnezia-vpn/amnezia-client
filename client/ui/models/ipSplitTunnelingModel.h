#ifndef IPSPLITTUNNELINGMODEL_H
#define IPSPLITTUNNELINGMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QPair>
#include <QStringList>

class IpSplitTunnelingModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        UrlRole = Qt::UserRole + 1,
        IpRole
    };

    explicit IpSplitTunnelingModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QVector<QPair<QString, QStringList>> &sites);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<QPair<QString, QStringList>> m_sites;
};

#endif // IPSPLITTUNNELINGMODEL_H
