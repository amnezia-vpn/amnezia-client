#ifndef INSTALLEDAPPSMODEL_H
#define INSTALLEDAPPSMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>

class InstalledAppsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        AppNameRole = Qt::UserRole + 1,
        PackageNameRole,
        AppIconRole,
        IsAppSelectedRole
    };

    explicit InstalledAppsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public slots:
    void selectedStateChanged(const int index, const bool selected);
    QVector<QPair<QString, QString>> getSelectedAppsInfo();

    void updateModel();

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QJsonArray m_installedApps;
    QSet<int> m_selectedAppIndexes;
};

#endif // INSTALLEDAPPSMODEL_H
