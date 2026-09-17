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

    function selectConnectionCountry(countryIndex, countryCode, countryName) {
        if (countryIndex === ApiCountryModel.currentIndex) {
            return
        }

        PageController.showBusyIndicator(true)
        SubscriptionUiController.updateServiceFromGateway(ServersUiController.processedServerId, countryCode, countryName)
        PageController.showBusyIndicator(false)
    }

    Component.onCompleted: {
        root.updateSubscriptionState()

        ApiCountryListModel.clearSearch()
        ApiCountryListModel.expandCurrentSection()
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

        visible: ApiCountryListModel.hasResults

        interactive: menuContent.contentHeight > menuContent.height

        ButtonGroup {
            id: containersRadioButtonGroup
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
            required property int sourceIndex
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

            ColumnLayout {
                id: countryRow

                width: rowItem.width
                height: rowItem.height
                visible: rowItem.rowType === "country"
                spacing: 0

                RowLayout {
                    VerticalRadioButton {
                        id: containerRadioButton

                        Layout.fillWidth: true
                        Layout.leftMargin: 16

                        text: rowItem.countryName

                        ButtonGroup.group: containersRadioButtonGroup

                        imageSource: "qrc:/images/controls/download.svg"

                        checked: rowItem.isCurrent
                        checkable: !ConnectionController.isConnected
                                   && !ConnectionController.isConnectionInProgress

                        onClicked: {
                            if (ConnectionController.isConnectionInProgress) {
                                PageController.showNotificationMessage(qsTr("Unable change server location while trying to make an active connection"))
                                return
                            }
                            if (ConnectionController.isConnected) {
                                PageController.showNotificationMessage(qsTr("Unable change server location while there is an active connection"))
                                return
                            }

                            root.selectConnectionCountry(rowItem.sourceIndex, rowItem.countryCode, rowItem.sourceCountryName)
                        }

                        Keys.onEnterPressed: {
                            if (checkable) {
                                checked = true
                            }
                            containerRadioButton.clicked()
                        }
                        Keys.onReturnPressed: {
                            if (checkable) {
                                checked = true
                            }
                            containerRadioButton.clicked()
                        }
                    }

                    Image {
                        Layout.rightMargin: 16
                        Layout.alignment: Qt.AlignRight

                        source: rowItem.countryImageCode !== ""
                                ? "qrc:/countriesFlags/images/flagKit/" + rowItem.countryImageCode + ".svg"
                                : ""
                    }
                }

                DividerType {
                    Layout.fillWidth: true
                }
            }
        }
    }

    CountrySectionHeader {
        id: stickyHeader

        listModel: ApiCountryListModel

        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        readonly property int topRow: menuContent.indexAt(menuContent.width / 2, menuContent.contentY + 1)

        isPinnedOverlay: true
        sectionKey: stickyHeader.topRow >= 0 ? ApiCountryListModel.sectionKeyAtRow(stickyHeader.topRow) : ""

        visible: ApiCountryListModel.isGrouped
                 && menuContent.visible
                 && menuContent.contentHeight > menuContent.height
                 && menuContent.contentY > menuContent.originY
                 && stickyHeader.sectionKey !== ""
    }

    CountriesEmptyState {
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: searchField.textField.activeFocus ? 0 : PageController.imeHeight

        visible: !ApiCountryListModel.hasResults

        isSearchResult: ApiCountryListModel.isSearchActive

        onShowAllRequested: searchField.clear()
    }

    Rectangle {
        id: topBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        color: AmneziaStyle.color.midnightBlack

        implicitHeight: topBarContent.implicitHeight

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        ColumnLayout {
            id: topBarContent

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            spacing: 4

            Item {
                id: navigationRow

                Layout.fillWidth: true
                Layout.topMargin: 20 + PageController.safeAreaTopMargin
                Layout.preferredHeight: backButton.implicitHeight

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
                    Keys.onEnterPressed: root.openServerInfo()
                    Keys.onReturnPressed: root.openServerInfo()
                }
            }

            BaseHeaderType {
                id: headerContent
                objectName: "headerContent"

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: root.subscriptionExpired || root.subscriptionExpiringSoon ? 0 : 4

                headerText: root.processedServer ? root.processedServer.name : ""
            }

            ParagraphTextType {
                visible: root.subscriptionExpired || root.subscriptionExpiringSoon

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12

                text: root.subscriptionExpired ? qsTr("Subscription expired") : qsTr("Subscription expiring soon")
                color: root.subscriptionExpired ? AmneziaStyle.color.vibrantRed : AmneziaStyle.color.goldenApricot
            }

            BasicButtonType {
                visible: (root.subscriptionExpired || root.subscriptionExpiringSoon)
                    && root.isSubscriptionRenewalAvailable && !root.isInAppPurchase

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 28
                Layout.bottomMargin: 0

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
                Layout.topMargin: (root.subscriptionExpired || root.subscriptionExpiringSoon) ? 12 : 4

                text: qsTr("Countries")
                color: AmneziaStyle.color.mutedGray
            }

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
                    Keys.onEnterPressed: sortDrawer.openTriggered()
                    Keys.onReturnPressed: sortDrawer.openTriggered()
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: ApiCountryListModel.isGrouped ? 0 : 12
            }
        }
    }

    SortCountriesDrawer {
        id: sortDrawer

        listModel: ApiCountryListModel

        anchors.fill: parent
    }
}
