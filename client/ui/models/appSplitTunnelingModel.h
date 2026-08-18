#ifndef APPSPLITTUNNELINGMODEL_H
#define APPSPLITTUNNELINGMODEL_H

#include <QAbstractListModel>
#include <QVector>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"

class AppSplitTunnelingModel: public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        AppPathRole = Qt::UserRole + 1,
        PackageAppNameRole,
        PackageAppIconRole
    };

    explicit AppSplitTunnelingModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void updateModel(const QVector<amnezia::InstalledAppInfo> &apps);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<amnezia::InstalledAppInfo> m_apps;
};

#endif // APPSPLITTUNNELINGMODEL_H
