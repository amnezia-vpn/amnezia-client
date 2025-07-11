pragma Singleton

import QtQuick 2.15

import SortFilterProxyModel 0.2

import ProtocolEnum 1.0

Item {
    ValueFilter {
        id: vpnTypeFilter
        roleName: "serviceType"
        value: ProtocolEnum.Vpn
    }

    ValueFilter {
        id: serviceTypeFilter
        roleName: "serviceType"
        value: ProtocolEnum.Other
    }

    ValueFilter {
        id: supportedFilter
        roleName: "isSupported"
        value: true
    }

    ValueFilter {
        id: installedFilter
        roleName: "isInstalled"
        value: true
    }

    ExpressionFilter {
        id: awgLegacyFilter
        // 2 - DockerContainer.AwgLegacy
        expression: model.dockerContainer !== 2 || model.isInstalled
    }

    function getWriteAccessProtocolsListFilters() {
        return [vpnTypeFilter, awgLegacyFilter]
    }
    function getReadAccessProtocolsListFilters() {
        return [vpnTypeFilter, installedFilter]
    }

    function getWriteAccessServicesListFilters() {
        return [serviceTypeFilter]
    }
    function getReadAccessServicesListFilters() {
        return [serviceTypeFilter, installedFilter]
    }
}
