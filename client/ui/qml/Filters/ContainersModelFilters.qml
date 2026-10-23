pragma Singleton

import QtQuick 2.15

import SortFilterProxyModel 0.2

Item {
    ValueFilter {
        id: vpnTypeFilter
        roleName: "isVpnContainer"
        value: true
    }

    ValueFilter {
        id: serviceTypeFilter
        roleName: "isServiceContainer"
        value: true
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

    ValueFilter {
        id: installationAllowedFilter
        roleName: "isInstallationAllowed"
        value: true
    }

    AnyOf {
        id: showProtocolFilter
        filters: [ installedFilter, installationAllowedFilter ]
    }

    function getWriteAccessProtocolsListFilters() {
        return [ vpnTypeFilter, showProtocolFilter ]
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
