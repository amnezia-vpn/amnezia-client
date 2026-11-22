import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    isTabBarHidden: true

    property var processedServer
    property bool subscriptionExpired: false
    property bool subscriptionExpiringSoon: false
    property bool isSubscriptionRenewalAvailable: false
    property bool isInAppPurchase: false

    function updateSubscriptionState() {
        root.subscriptionExpired = ServersUiController.isServerSubscriptionExpired(ServersUiController.processedServerId)
        root.subscriptionExpiringSoon = ServersUiController.isServerSubscriptionExpiringSoon(ServersUiController.processedServerId)
        root.isSubscriptionRenewalAvailable = ApiAccountInfoModel.data("isSubscriptionRenewalAvailable")
        root.isInAppPurchase = ApiAccountInfoModel.data("isInAppPurchase")
    }

    function openServerInfo() {
        PageController.showBusyIndicator(true)
        let result = SubscriptionUiController.getAccountInfo(ServersUiController.processedServerId, false)
        PageController.showBusyIndicator(false)
        if (!result) {
            return
        }

        PageController.goToPage(PageEnum.PageSettingsApiServerInfo)
    }

    function selectConnectionCountry(countryCode, countryName) {
        if (countryCode === ApiCountryModel.currentCountryCode) {
            return
        }

        PageController.showBusyIndicator(true)
        SubscriptionUiController.updateServiceFromGateway(ServersUiController.processedServerId, countryCode, countryName)
        PageController.showBusyIndicator(false)
    }

    readonly property bool showSubscriptionNote: root.subscriptionExpired || root.subscriptionExpiringSoon
    readonly property bool showRenewButton: root.showSubscriptionNote
                                            && root.isSubscriptionRenewalAvailable && !root.isInAppPurchase

    property int stickyRevision: 0
    property real savedScroll: 0

    function activateCountry(countryCode, countryName) {
        if (ConnectionController.isConnectionInProgress) {
            PageController.showNotificationMessage(qsTr("Unable change server location while trying to make an active connection"))
            return
        }
        if (ConnectionController.isConnected) {
            PageController.showNotificationMessage(qsTr("Unable change server location while there is an active connection"))
            return
        }
        root.selectConnectionCountry(countryCode, countryName)
    }

    Component.onCompleted: {
        root.updateSubscriptionState()

        ApiCountryListModel.applyDefaultState(true)
    }

    Connections {
        target: ApiCountryListModel

        function onPositionRequested(row) {
            Qt.callLater(function() {
                const current = ApiCountryListModel.rowForCountryCode(ApiCountryModel.currentCountryCode)
                if (current >= 0) {
                    menuContent.positionViewAtIndex(current, ListView.Center)
                    menuContent.contentY = pageHeader.clampContentY(menuContent.contentY
                                                                          - pageHeader.pinnedHeight / 2)
                }
            })
        }

        function onLayoutRebuilt() {
            Qt.callLater(function() {
                menuContent.forceLayout()
                root.stickyRevision += 1
            })
        }

        function onSourceAboutToRefresh() {
            root.savedScroll = menuContent.contentY - menuContent.originY
        }

        function onSourceRefreshed() {
            Qt.callLater(function() {
                menuContent.forceLayout()
                const maxScroll = Math.max(0, menuContent.contentHeight - menuContent.height)
                menuContent.contentY = menuContent.originY + Math.min(root.savedScroll, maxScroll)
            })
        }

        function onFavoritesLimitExceeded() {
            favoritesLimitToast.show(qsTr("You already have %1 locations in favorites. Remove one")
                                     .arg(ApiCountryListModel.favoritesLimit),
                                     qsTr("Show"))
        }
    }

    Connections {
        target: ServersUiController

        function onProcessedServerIdChanged() {
            root.processedServer = proxyServersModel.get(0)
            root.updateSubscriptionState()
        }
    }

    Connections {
        target: ServersModel

        function onModelReset() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    Connections {
        target: ApiAccountInfoModel

        function onModelReset() {
            root.updateSubscriptionState()
        }
    }

    SortFilterProxyModel {
        id: proxyServersModel

        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "serverId"
                value: ServersUiController.processedServerId
            }
        ]

        Component.onCompleted: {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    ListViewType {
        id: menuContent

        reuseItems: true

        anchors.top: parent.top
        anchors.topMargin: pageHeader.topBarHeight
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: searchRow.searchField.activeFocus ? 0 : PageController.imeHeight

        model: ApiCountryListModel


        ScrollBar.vertical: pageHeader.scrollBar

        header: pageHeader.listHeader

        footer: Item {
            width: menuContent.width
            height: 16 + PageController.safeAreaBottomMargin
        }

        delegate: Item {
            id: rowItem

            required property int index
            required property string rowType
            required property string sectionKey
            required property bool isCurrent
            required property bool isFavorite
            required property bool isIssued
            required property bool isWorkerExpired
            required property var availableProtocols
            required property int lastConnectionState
            required property string lastConnectionAt
            required property string lastSuccessAt
            required property int lastConnectionSetupMs
            required property string lastConnectionError
            required property string countryName
            required property string sourceCountryName
            required property string countryCode
            required property string countryImageCode

            width: menuContent.width

            implicitHeight: rowItem.rowType === "section" ? sectionHeader.implicitHeight : countryItem.implicitHeight
            height: implicitHeight

            readonly property bool isLiveConnection: rowItem.isCurrent && ConnectionController.isConnected

            readonly property string protocolsLabel: {
                if (!rowItem.availableProtocols || rowItem.availableProtocols.length === 0) {
                    return ""
                }

                let names = []
                for (let i = 0; i < rowItem.availableProtocols.length; ++i) {
                    const protocol = rowItem.availableProtocols[i]
                    if (protocol === "awg") {
                        names.push("AmneziaWG")
                    } else if (protocol === "vless") {
                        names.push("VLESS")
                    } else {
                        names.push(String(protocol).toUpperCase())
                    }
                }
                return names.join(" · ")
            }

            function formatBytes(bytes) {
                if (!bytes || bytes <= 0) {
                    return "0 B"
                }

                const units = ["B", "KB", "MB", "GB", "TB"]
                let value = Number(bytes)
                let unit = 0
                while (value >= 1024 && unit < units.length - 1) {
                    value /= 1024
                    ++unit
                }
                return (unit === 0 ? value.toFixed(0) : value.toFixed(1)) + " " + units[unit]
            }

            function formatDuration(seconds) {
                if (!seconds || seconds <= 0) {
                    return "0 s"
                }
                if (seconds < 60) {
                    return seconds + " s"
                }

                const minutes = Math.floor(seconds / 60)
                if (minutes < 60) {
                    return qsTr("%1 min").arg(minutes)
                }

                const hours = Math.floor(minutes / 60)
                const remainder = minutes % 60
                return remainder > 0 ? qsTr("%1 h %2 min").arg(hours).arg(remainder)
                                     : qsTr("%1 h").arg(hours)
            }

            function timeAgo(isoDate) {
                if (!isoDate) {
                    return ""
                }

                const timestamp = Date.parse(isoDate)
                if (isNaN(timestamp)) {
                    return ""
                }

                const diff = Math.max(0, Date.now() - timestamp)
                const minutes = Math.floor(diff / 60000)
                if (minutes < 1) {
                    return qsTr("just now")
                }
                if (minutes < 60) {
                    return qsTr("%1 min ago").arg(minutes)
                }

                const hours = Math.floor(minutes / 60)
                if (hours < 24) {
                    return qsTr("%1 h ago").arg(hours)
                }
                return Qt.formatDateTime(new Date(timestamp), "dd.MM.yyyy")
            }

            readonly property string connectionSubtitle: {
                if (rowItem.rowType !== "country") {
                    return ""
                }

                if (rowItem.isLiveConnection) {
                    return qsTr("Connected · %1 · ↓ %2/s ↑ %3/s")
                            .arg(formatDuration(ConnectionController.connectionDurationSecs))
                            .arg(formatBytes(ConnectionController.downloadSpeed))
                            .arg(formatBytes(ConnectionController.uploadSpeed))
                }

                if (rowItem.isCurrent) {
                    return rowItem.protocolsLabel ? qsTr("Selected · %1").arg(rowItem.protocolsLabel)
                                                  : qsTr("Selected")
                }

                if (rowItem.lastConnectionState === 2) {
                    const error = rowItem.lastConnectionError ? ": " + rowItem.lastConnectionError : ""
                    const when = rowItem.lastConnectionAt ? " · " + timeAgo(rowItem.lastConnectionAt) : ""
                    return qsTr("Last attempt failed") + error + when
                }

                if (rowItem.lastConnectionState === 1) {
                    const setup = rowItem.lastConnectionSetupMs > 0
                            ? " · " + qsTr("setup %1 s").arg((rowItem.lastConnectionSetupMs / 1000).toFixed(1))
                            : ""
                    const when = rowItem.lastSuccessAt ? timeAgo(rowItem.lastSuccessAt) : ""
                    return when ? qsTr("Last connected %1").arg(when) + setup
                                : qsTr("Last connection successful") + setup
                }

                if (rowItem.isWorkerExpired) {
                    return rowItem.protocolsLabel ? qsTr("Update available · %1").arg(rowItem.protocolsLabel)
                                                  : qsTr("Update available")
                }

                if (rowItem.isIssued) {
                    return rowItem.protocolsLabel ? qsTr("Config ready · %1").arg(rowItem.protocolsLabel)
                                                  : qsTr("Config ready")
                }

                return rowItem.protocolsLabel
            }

            readonly property color connectionStatusColor: {
                if (rowItem.isLiveConnection) {
                    return AmneziaStyle.color.statusSuccess
                }
                if (rowItem.isCurrent) {
                    return AmneziaStyle.color.accentDefault
                }
                if (rowItem.lastConnectionState === 2) {
                    return AmneziaStyle.color.statusDestructive
                }
                if (rowItem.isWorkerExpired) {
                    return AmneziaStyle.color.statusWarning
                }
                if (rowItem.lastConnectionState === 1) {
                    return AmneziaStyle.color.statusSuccess
                }
                return AmneziaStyle.color.transparent
            }

            CountrySectionHeader {
                id: sectionHeader

                listModel: ApiCountryListModel
                width: rowItem.width
                visible: rowItem.rowType === "section"

                sectionKey: rowItem.rowType === "section" ? rowItem.sectionKey : ""
                row: rowItem.rowType === "section" ? rowItem.index : -1
            }

            CountryListItem {
                id: countryItem

                x: 16
                width: rowItem.width - 32
                height: rowItem.height

                visible: rowItem.rowType === "country"

                text: rowItem.countryName
                subtitle: rowItem.connectionSubtitle
                statusColor: rowItem.connectionStatusColor
                countryFlagCode: rowItem.countryImageCode
                highlighted: rowItem.isCurrent
                favorite: rowItem.isFavorite

                onClicked: root.activateCountry(rowItem.countryCode, rowItem.sourceCountryName)
                onFavoriteToggled: ApiCountryListModel.toggleFavorite(rowItem.countryCode)
            }
        }
    }

    ListPageHeaderType {
        id: pageHeader

        anchors.fill: parent

        listView: menuContent

        collapsibleContent: ColumnLayout {
            spacing: 0

            BodyTextType {
                visible: root.showSubscriptionNote

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                text: root.subscriptionExpired ? qsTr("Subscription expired") : qsTr("Subscription expiring soon")
                color: root.subscriptionExpired ? AmneziaStyle.color.statusDestructive : AmneziaStyle.color.textAccent
            }

            ButtonType {
                visible: root.showRenewButton

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 24

                text: qsTr("Renew subscription")

                onClicked: {
                    SubscriptionUiController.getRenewalLink(ServersUiController.processedServerId)
                }
            }
        }

        navigationButtons: [
            IconButtonType {
                id: settingsButton

                icon.source: "qrc:/images/controls/settings.svg"

                onClicked: root.openServerInfo()
            }
        ]

        pinnedContent: [
            CountrySearchRow {
                id: searchRow

                listModel: ApiCountryListModel

                onSortRequested: sortDrawer.open()
            },

            CountryUseCaseChips {
                id: useCaseChips

                listModel: ApiCountryListModel

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.preferredHeight: useCaseChips.implicitHeight
            },

            NotificationType {
                readonly property string bannerText:
                    CountryRegionNames.useCaseBanners[ApiCountryListModel.activeUseCaseId] || ""

                visible: bannerText !== ""

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 24

                text: bannerText
            },

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: ApiCountryListModel.isGrouped ? 0 : 24
            }
        ]

        CountryListStickyHeader {
            listModel: ApiCountryListModel
            listView: menuContent
            header: pageHeader
            revision: root.stickyRevision
        }

        CountriesEmptyState {
            anchors.top: parent.top
            anchors.topMargin: pageHeader.pinnedBottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: searchRow.searchField.activeFocus ? 0 : PageController.imeHeight

            visible: !ApiCountryListModel.hasResults

            isSearchResult: ApiCountryListModel.isSearchActive
            categoryName: ApiCountryListModel.activeUseCaseId !== "all"
                          ? CountryRegionNames.useCaseName(ApiCountryListModel, ApiCountryListModel.activeUseCaseId)
                          : ""

            onShowAllRequested: {
                searchRow.searchField.clear()
                ApiCountryListModel.activeUseCaseId = "all"
            }
        }
    }

    SortCountriesDrawer {
        id: sortDrawer

        listModel: ApiCountryListModel
    }

    SnackbarType {
        id: favoritesLimitToast

        onActionTriggered: ApiCountryListModel.activeUseCaseId = "favorites"
    }
}
