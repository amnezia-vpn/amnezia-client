#ifndef SITESMODEL_H
#define SITESMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QPair>

class SitesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        UrlRole = Qt::UserRole + 1,
        IpRole
    };

    explicit SitesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QVector<QPair<QString, QString>> &sites);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<QPair<QString, QString>> m_sites;
};

#endif // SITESMODEL_H
