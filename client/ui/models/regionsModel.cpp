#include "regionsModel.h"

RegionsModel::RegionsModel(QSharedPointer<AuthController> authController, std::shared_ptr<Settings> settings,
                           QObject *parent) :
    m_authController(authController), m_settings(settings), QAbstractListModel{parent} {
    connect(m_authController.get(), &AuthController::regionsUpdated, this, [this]() { resetModel(); });
}

int RegionsModel::rowCount(const QModelIndex &parent) const { return m_regions.size(); }

QVariant RegionsModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_regions.size()) {
        return QVariant();
    }

    auto &region = m_regions.at(index.row());

    switch (role) {
        case NameRole:
            return region.countryName;
        case RegionImagePathRole:
            return RegionsModel::getRegionImagePath(region.countryCode);
    }

    return QVariant();
}

void RegionsModel::setSelectedRegionId(const QString id) {
    m_settings->setSelectedRegionId(id);
    emit selectedRegionChanged();
}

int RegionsModel::getSelectedRegionIndex() {
    auto selectedRegionId = getSelectedRegionId();

    for (int i = 0; i < m_regions.size(); i++) {
        if (selectedRegionId == m_regions[i].id) {
            return i;
        }
    }

    return 0;
}

void RegionsModel::setSelectedRegionIndex(const int index) {
    if (index >= m_regions.size())
        return;
    setSelectedRegionId(m_regions[index].id);
}

QString RegionsModel::getSelectedRegionId() {
    QString regionId = m_settings->getSelectedRegionId();
    if (regionId.isEmpty() && !m_regions.isEmpty()) {
        return m_regions[0].id;
    }

    return regionId;
}

QString RegionsModel::getSelectedRegionName() { return getRegionInfo(getSelectedRegionId()).countryName; }

QString RegionsModel::getSelectedRegionImagePath() {
    return RegionsModel::getRegionImagePath(getRegionInfo(getSelectedRegionId()).countryCode);
}

QString RegionsModel::getSelectedRegionDescriptionExpanded() { return getRegionInfo(getSelectedRegionId()).id; }

QString RegionsModel::getSelectedRegionDescriptionCollapsed() { return getRegionInfo(getSelectedRegionId()).id; }

QString RegionsModel::getSelectedRegionProtocolName() {
    return ContainerProps::containerHumanNames().value(DockerContainer::Xray);
}

RegionInfo RegionsModel::getRegionInfo(const QString id) {
    for (auto &region: m_regions) {
        if (region.id == id)
            return region;
    }

    return {};
}

QString RegionsModel::getRegionImagePath(const QString countryCode) {
    if (countryCode.isEmpty()) {
        return "";
    }
    return QString("qrc:/countriesFlags/images/flagKit/%1.svg").arg(countryCode);
}

void RegionsModel::resetModel() {
    beginResetModel();
    m_regions = m_authController->getRegions();
    setSelectedRegionId(getSelectedRegionId());
    endResetModel();
}

QHash<int, QByteArray> RegionsModel::roleNames() const {
    QHash<int, QByteArray> roles;

    roles[NameRole] = "regionName";
    roles[RegionImagePathRole] = "regionImagePath";

    return roles;
}
