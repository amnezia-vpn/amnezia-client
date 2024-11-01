#ifndef REGIONSMODEL_H
#define REGIONSMODEL_H

#include <QAbstractListModel>
#include "settings.h"
#include "ui/controllers/authController.h"

class RegionsModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { NameRole = Qt::UserRole + 1, RegionImagePathRole };

    explicit RegionsModel(QSharedPointer<AuthController> authController, std::shared_ptr<Settings> settings,
                          QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void resetModel();

    Q_PROPERTY(int selectedRegionIndex READ getSelectedRegionIndex WRITE setSelectedRegionIndex NOTIFY
                       selectedRegionChanged)
    Q_PROPERTY(QString selectedRegionName READ getSelectedRegionName NOTIFY selectedRegionChanged)
    Q_PROPERTY(QString selectedRegionImagePath READ getSelectedRegionImagePath NOTIFY selectedRegionChanged)
    Q_PROPERTY(QString selectedRegionDescriptionExpanded READ getSelectedRegionDescriptionExpanded NOTIFY
                       selectedRegionChanged)
    Q_PROPERTY(QString selectedRegionDescriptionCollapsed READ getSelectedRegionDescriptionCollapsed NOTIFY
                       selectedRegionChanged)
    Q_PROPERTY(QString selectedRegionProtocolName READ getSelectedRegionProtocolName NOTIFY selectedRegionChanged)

public slots:
    void setSelectedRegionId(const QString id);

    int getSelectedRegionIndex();
    void setSelectedRegionIndex(const int index);

    QString getSelectedRegionId();
    QString getSelectedRegionName();
    QString getSelectedRegionImagePath();

    QString getSelectedRegionDescriptionExpanded();
    QString getSelectedRegionDescriptionCollapsed();

    QString getSelectedRegionProtocolName();

    RegionInfo getRegionInfo(const QString id);

    static QString getRegionImagePath(const QString countryCode);

protected:
    QHash<int, QByteArray> roleNames() const override;

signals:
    void selectedRegionChanged();

private:
    QSharedPointer<AuthController> m_authController;
    std::shared_ptr<Settings> m_settings;
    QList<RegionInfo> m_regions;
};

#endif // REGIONSMODEL_H
