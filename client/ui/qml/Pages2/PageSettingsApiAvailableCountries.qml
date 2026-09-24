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

    readonly property real listScrolled: menuContent.contentY - menuContent.originY
    readonly property bool showSubscriptionNote: root.subscriptionExpired || root.subscriptionExpiringSoon
    readonly property bool showRenewButton: root.showSubscriptionNote
                                            && root.isSubscriptionRenewalAvailable && !root.isInAppPurchase
    readonly property real collapsibleHeight: menuContent.headerItem ? menuContent.headerItem.collapsibleHeight : 0
    readonly property real collapseProgress: root.collapsibleHeight > 0
                                             ? Math.max(0, Math.min(1, root.listScrolled / root.collapsibleHeight))
                                             : 0
    readonly property real listOcclusion: pinnedBlock.y + pinnedBlock.height - menuContent.y
    readonly property bool pinnedBlockStuck: root.listScrolled >= root.collapsibleHeight

    property int stickyRevision: 0
    property real savedScroll: 0

    function clampContentY(value) {
        const maxY = menuContent.originY + Math.max(0, menuContent.contentHeight - menuContent.height)
        return Math.max(menuContent.originY, Math.min(value, maxY))
    }

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
                    menuContent.contentY = root.clampContentY(menuContent.contentY - pinnedBlock.height / 2)
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
            PageController.showNotificationMessage(qsTr("You can add up to %1 locations to favorites")
                                                   .arg(ApiCountryListModel.favoritesLimit))
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
        objectName: "proxyServersModel"

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

        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: searchField.textField.activeFocus ? 0 : PageController.imeHeight

        model: ApiCountryListModel

        interactive: menuContent.contentHeight > menuContent.height

        header: Item {
            readonly property real collapsibleHeight: collapsibleContent.implicitHeight + 4

            width: menuContent.width
            height: collapsibleHeight + pinnedBlock.height

            ColumnLayout {
                id: collapsibleContent

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 4

                spacing: 0

                opacity: 1 - root.collapseProgress

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: largeTitleMetrics.height
                    Layout.bottomMargin: root.showSubscriptionNote ? 0 : 4
                }

                ParagraphTextType {
                    visible: root.showSubscriptionNote

                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 12

                    text: root.subscriptionExpired ? qsTr("Subscription expired") : qsTr("Subscription expiring soon")
                    color: root.subscriptionExpired ? AmneziaStyle.color.vibrantRed : AmneziaStyle.color.goldenApricot
                }

                BasicButtonType {
                    visible: root.showRenewButton

                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 28

                    defaultColor: AmneziaStyle.color.paleGray
                    hoveredColor: AmneziaStyle.color.lightGray
                    pressedColor: AmneziaStyle.color.mutedGray
                    textColor: AmneziaStyle.color.midnightBlack

                    text: qsTr("Renew subscription")

                    clickedFunc: function() {
                        SubscriptionUiController.getRenewalLink(ServersUiController.processedServerId)
                    }
                }

                ParagraphTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: root.showSubscriptionNote ? 12 : 4

                    text: qsTr("Countries")
                    color: AmneziaStyle.color.mutedGray
                }
            }
        }

        footer: Item {
            width: menuContent.width
            height: 16
        }

        delegate: Item {
            id: rowItem

            required property string rowType
            required property string sectionKey
            required property bool isCurrent
            required property bool isFavorite
            required property string countryName
            required property string sourceCountryName
            required property string countryCode
            required property string countryImageCode

            width: menuContent.width

            implicitHeight: rowItem.rowType === "section" ? 60 : 72
            height: implicitHeight

            CountrySectionHeader {
                id: sectionHeader

                listModel: ApiCountryListModel
                width: rowItem.width
                visible: rowItem.rowType === "section"

                sectionKey: rowItem.rowType === "section" ? rowItem.sectionKey : ""
            }

            Item {
                id: countryRow

                width: rowItem.width
                height: rowItem.height
                visible: rowItem.rowType === "country"

                readonly property int flagX: ApiCountryListModel.isGrouped ? 44 : 28

                Item {
                    id: rowBody

                    property bool isFocusable: countryRow.visible

                    anchors.fill: parent

                    function activate() {
                        root.activateCountry(rowItem.countryCode, rowItem.sourceCountryName)
                    }

                    HoverHandler {
                        id: rowHover
                        cursorShape: Qt.PointingHandCursor
                    }

                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds

                        onTapped: function(eventPoint) {
                            if (star.contains(star.mapFromItem(rowBody, eventPoint.position))) {
                                return
                            }
                            rowBody.activate()
                        }
                    }

                    Keys.onEnterPressed: rowBody.activate()
                    Keys.onReturnPressed: rowBody.activate()
                    Keys.onSpacePressed: rowBody.activate()
                    Keys.onTabPressed: FocusController.nextKeyTabItem()
                    Keys.onBacktabPressed: FocusController.previousKeyTabItem()
                    Keys.onUpPressed: FocusController.nextKeyUpItem()
                    Keys.onDownPressed: FocusController.nextKeyDownItem()
                    Keys.onLeftPressed: FocusController.nextKeyLeftItem()
                    Keys.onRightPressed: FocusController.nextKeyRightItem()

                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        anchors.topMargin: 4
                        anchors.bottomMargin: 4
                        radius: 16

                        color: ((rowHover.hovered && !starHover.hovered) || rowBody.activeFocus)
                               ? AmneziaStyle.color.surfaceHovered
                               : AmneziaStyle.color.transparent
                        border.width: rowItem.isCurrent ? 1 : 0
                        border.color: AmneziaStyle.color.textTertiary
                    }

                    Image {
                        id: flag

                        x: countryRow.flagX
                        anchors.verticalCenter: parent.verticalCenter
                        width: 24
                        height: 16

                        source: rowItem.countryImageCode !== ""
                                ? "qrc:/countriesFlags/images/flagKit/" + rowItem.countryImageCode + ".svg"
                                : ""
                    }

                    ListItemTitleType {
                        anchors.left: flag.right
                        anchors.leftMargin: 16
                        anchors.right: parent.right
                        anchors.rightMargin: 16 + 12 + star.width + 8
                        anchors.verticalCenter: parent.verticalCenter

                        text: rowItem.countryName
                        color: AmneziaStyle.color.textPrimary
                        maximumLineCount: 2
                        elide: Text.ElideRight
                    }
                }

                Item {
                    id: star

                    property bool isFocusable: countryRow.visible

                    anchors.right: parent.right
                    anchors.rightMargin: 28
                    anchors.verticalCenter: parent.verticalCenter
                    width: 40
                    height: 40

                    function toggle() {
                        ApiCountryListModel.toggleFavorite(rowItem.countryCode)
                    }

                    Accessible.name: rowItem.isFavorite ? qsTr("Remove from favorites")
                                                        : qsTr("Add to favorites")

                    HoverHandler {
                        id: starHover
                        cursorShape: Qt.PointingHandCursor
                    }

                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds
                        onTapped: star.toggle()
                    }

                    Keys.onEnterPressed: star.toggle()
                    Keys.onReturnPressed: star.toggle()
                    Keys.onSpacePressed: star.toggle()
                    Keys.onTabPressed: FocusController.nextKeyTabItem()
                    Keys.onBacktabPressed: FocusController.previousKeyTabItem()
                    Keys.onUpPressed: FocusController.nextKeyUpItem()
                    Keys.onDownPressed: FocusController.nextKeyDownItem()
                    Keys.onLeftPressed: FocusController.nextKeyLeftItem()
                    Keys.onRightPressed: FocusController.nextKeyRightItem()

                    Rectangle {
                        anchors.fill: parent
                        radius: 12

                        color: (starHover.hovered || star.activeFocus) ? AmneziaStyle.color.surfaceHovered
                                                                       : AmneziaStyle.color.transparent
                        border.width: star.activeFocus ? 1 : 0
                        border.color: AmneziaStyle.color.borderSoft
                    }

                    Image {
                        anchors.centerIn: parent
                        width: 24
                        height: 24

                        source: rowItem.isFavorite ? "qrc:/images/controls/star-filled.svg"
                                                   : "qrc:/images/controls/star.svg"
                    }
                }

                DividerType {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    anchors.bottom: parent.bottom
                }
            }
        }
    }

    CountrySectionHeader {
        id: stickyHeader

        listModel: ApiCountryListModel

        anchors.top: pinnedBlock.bottom
        anchors.topMargin: stickyHeader.pushOffset
        anchors.left: parent.left
        anchors.right: parent.right

        readonly property real visibleTop: menuContent.contentY + root.listOcclusion

        readonly property int topRow: {
            root.stickyRevision
            return menuContent.indexAt(menuContent.width / 2, stickyHeader.visibleTop + 1)
        }

        readonly property real pushOffset: {
            root.stickyRevision
            const probe = menuContent.indexAt(menuContent.width / 2, stickyHeader.visibleTop + 60)
            if (probe < 0 || probe === stickyHeader.topRow || !ApiCountryListModel.isSectionHeaderRow(probe)) {
                return 0
            }
            const next = menuContent.itemAtIndex(probe)
            if (!next) {
                return 0
            }
            return Math.min(0, next.y - stickyHeader.visibleTop - 60)
        }

        onToggled: function(sectionKey) {
            Qt.callLater(function() {
                const headerRow = ApiCountryListModel.rowForSectionHeader(sectionKey)
                if (headerRow >= 0) {
                    menuContent.positionViewAtIndex(headerRow, ListView.Beginning)
                    menuContent.contentY = root.clampContentY(menuContent.contentY - pinnedBlock.height)
                }
            })
        }

        isPinnedOverlay: true
        sectionKey: stickyHeader.topRow >= 0 ? ApiCountryListModel.sectionKeyAtRow(stickyHeader.topRow) : ""

        visible: ApiCountryListModel.isGrouped
                 && ApiCountryListModel.hasResults
                 && root.pinnedBlockStuck
                 && stickyHeader.sectionKey !== ""
    }

    CountriesEmptyState {
        anchors.top: pinnedBlock.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: searchField.textField.activeFocus ? 0 : PageController.imeHeight

        visible: !ApiCountryListModel.hasResults

        isSearchResult: ApiCountryListModel.isSearchActive
        categoryName: ApiCountryListModel.activeUseCaseId !== "all"
                      ? (CountryRegionNames.useCaseNames[ApiCountryListModel.activeUseCaseId]
                         || ApiCountryListModel.activeUseCaseId)
                      : ""

        onShowAllRequested: {
            searchField.clear()
            ApiCountryListModel.activeUseCaseId = "all"
        }
    }

    Rectangle {
        id: topBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        color: AmneziaStyle.color.midnightBlack

        implicitHeight: navigationRow.y + navigationRow.height + 4

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        Item {
            id: navigationRow

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 20 + PageController.safeAreaTopMargin
            height: backButton.implicitHeight

            BackButtonType {
                id: backButton
                objectName: "backButton"

                anchors.left: parent.left
                anchors.right: settingsButton.left
                anchors.verticalCenter: parent.verticalCenter
            }

            ImageButtonType {
                id: settingsButton
                objectName: "settingsButton"

                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter

                implicitWidth: 40
                implicitHeight: 40

                hoverEnabled: true
                image: "qrc:/images/controls/settings.svg"
                imageColor: AmneziaStyle.color.paleGray

                onClicked: root.openServerInfo()
            }
        }
    }

    Rectangle {
        id: pinnedBlock

        anchors.left: parent.left
        anchors.right: parent.right
        y: topBar.height + Math.max(0, root.collapsibleHeight - root.listScrolled)
        height: pinnedContent.implicitHeight

        color: AmneziaStyle.color.midnightBlack

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        ColumnLayout {
            id: pinnedContent

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12

                spacing: 8

                CountrySearchField {
                    id: searchField

                    Layout.fillWidth: true

                    onTextChanged: ApiCountryListModel.searchText = searchField.text
                }

                ImageButtonType {
                    objectName: "sortButton"

                    implicitWidth: 64
                    implicitHeight: 64

                    hoverEnabled: true
                    image: "qrc:/images/controls/sort-desc.svg"
                    imageColor: AmneziaStyle.color.paleGray

                    onClicked: sortDrawer.openTriggered()
                }
            }

            CountryUseCaseChips {
                id: useCaseChips

                listModel: ApiCountryListModel

                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.preferredHeight: useCaseChips.implicitHeight
            }

            WarningType {
                readonly property string bannerText:
                    CountryRegionNames.useCaseBanners[ApiCountryListModel.activeUseCaseId] || ""

                visible: bannerText !== ""

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12

                backGroundColor: AmneziaStyle.color.surfaceBase
                iconPath: "qrc:/images/controls/info.svg"
                textString: bannerText
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: ApiCountryListModel.isGrouped ? 0 : 12
            }
        }
    }

    Header1TextType {
        id: largeTitleMetrics

        visible: false
        text: movingTitle.text
    }

    Header1TextType {
        id: movingTitle

        readonly property real compactScale: 18 / 32
        readonly property real titleScale: 1 - (1 - movingTitle.compactScale) * root.collapseProgress
        readonly property real expandedX: 16
        readonly property real compactX: (root.width - movingTitle.implicitWidth * movingTitle.compactScale) / 2
        readonly property real expandedY: topBar.height + 4 - Math.min(0, root.listScrolled)
        readonly property real compactY: navigationRow.y
                                         + (navigationRow.height - movingTitle.implicitHeight * movingTitle.compactScale) / 2

        x: movingTitle.expandedX + (movingTitle.compactX - movingTitle.expandedX) * root.collapseProgress
        y: Math.max(movingTitle.compactY, movingTitle.expandedY - Math.max(0, root.listScrolled))
        width: movingTitle.implicitWidth

        transformOrigin: Item.TopLeft
        scale: movingTitle.titleScale

        wrapMode: Text.NoWrap
        maximumLineCount: 1
        text: qsTr("Amnezia Premium")
    }

    SortCountriesDrawer {
        id: sortDrawer

        listModel: ApiCountryListModel

        anchors.fill: parent
    }
}
