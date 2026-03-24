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
    function updateSubscriptionState() {
        root.subscriptionExpiringSoon = ApiAccountInfoModel.data("isSubscriptionExpiringSoon")
    }

    Component.onCompleted: {
        root.updateSubscriptionState()
    }

    Connections {
        target: ApiAccountInfoModel
        function onModelReset() {
            root.updateSubscriptionState()
        }
    }

    Connections {
        target: ServersModel

        function onProcessedServerChanged() {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    Connections {
        target: ApiConfigsController

        function onSubscriptionExpiredOnServer() {
            root.subscriptionExpired = true
            root.subscriptionExpiringSoon = false
        }
    }

    SortFilterProxyModel {
        id: proxyServersModel
        objectName: "proxyServersModel"

        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]

        Component.onCompleted: {
            root.processedServer = proxyServersModel.get(0)
        }
    }

    ListViewType {
        id: menuContent

        anchors.fill: parent

        model: ApiCountryModel

        currentIndex: 0

        ButtonGroup {
            id: containersRadioButtonGroup
        }

        header: ColumnLayout {
            width: menuContent.width

            spacing: 4

            BackButtonType {
                id: backButton
                objectName: "backButton"

                Layout.topMargin: 20 + SettingsController.safeAreaTopMargin
            }

            HeaderTypeWithButton {
                id: headerContent
                objectName: "headerContent"

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 4

                actionButtonImage: "qrc:/images/controls/settings.svg"

                headerText: root.processedServer.name

                actionButtonFunction: function() {
                    PageController.showBusyIndicator(true)
                    let result = ApiSettingsController.getAccountInfo(false)
                    PageController.showBusyIndicator(false)
                    if (!result) {
                        return
                    }

                    PageController.goToPage(PageEnum.PageSettingsApiServerInfo)
                }
            }

            CaptionTextType {
                visible: root.subscriptionExpired || root.subscriptionExpiringSoon

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 4

                text: root.subscriptionExpired ? qsTr("Subscription expired") : qsTr("Subscription expiring soon")
                color: root.subscriptionExpired ? AmneziaStyle.color.vibrantRed : AmneziaStyle.color.goldenApricot
            }

            BasicButtonType {
                visible: root.subscriptionExpired || root.subscriptionExpiringSoon

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 4

                defaultColor: AmneziaStyle.color.paleGray
                hoveredColor: AmneziaStyle.color.lightGray
                pressedColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.midnightBlack

                text: qsTr("Renew subscription")

                clickedFunc: function() {
                    ApiSettingsController.getRenewalLink()
                }
            }

            CaptionTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: (root.subscriptionExpired || root.subscriptionExpiringSoon) ? 8 : 4
                Layout.bottomMargin: 8

                text: qsTr("Location for connection")
                color: AmneziaStyle.color.mutedGray
            }
        }

        delegate: ColumnLayout {
            id: content

            width: menuContent.width
            height: content.implicitHeight

            RowLayout {
                VerticalRadioButton {
                    id: containerRadioButton

                    Layout.fillWidth: true
                    Layout.leftMargin: 16

                    text: countryName

                    ButtonGroup.group: containersRadioButtonGroup

                    imageSource: "qrc:/images/controls/download.svg"

                    checked: index === ApiCountryModel.currentIndex
                    checkable: !ConnectionController.isConnected

                    onClicked: {
                        if (ConnectionController.isConnectionInProgress) {
                            PageController.showNotificationMessage(qsTr("Unable change server location while trying to make an active connection"))
                            return
                        }
                        if (ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Unable change server location while there is an active connection"))
                            return
                        }

                        if (index !== ApiCountryModel.currentIndex) {
                            PageController.showBusyIndicator(true)
                            var prevIndex = ApiCountryModel.currentIndex
                            ApiCountryModel.currentIndex = index
                            if (!ApiConfigsController.updateServiceFromGateway(ServersModel.defaultIndex, countryCode, countryName)) {
                                ApiCountryModel.currentIndex = prevIndex
                            }
                            PageController.showBusyIndicator(false)
                        }
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
                    Layout.rightMargin: 32
                    Layout.alignment: Qt.AlignRight

                    source: "qrc:/countriesFlags/images/flagKit/" + countryImageCode + ".svg"
                }
            }

            DividerType {
                Layout.fillWidth: true
            }
        }
    }
}
