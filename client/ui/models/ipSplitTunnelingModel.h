#ifndef IPSPLITTUNNELINGMODEL_H
#define IPSPLITTUNNELINGMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QPair>

class IpSplitTunnelingModel : public QAbstractListModel
{
    Q_OBJECT

public:
    Q_PROPERTY(int count READ count NOTIFY countChanged)

    enum Roles {
        UrlRole = Qt::UserRole + 1,
        IpRole
    };

    explicit IpSplitTunnelingModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int count() const;

public slots:
    void updateModel(const QVector<QPair<QString, QString>> &sites);

signals:
    void countChanged();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<QPair<QString, QString>> m_sites;
};

#endif // IPSPLITTUNNELINGMODEL_H
