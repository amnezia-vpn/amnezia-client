import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    defaultActiveFocusItem: focusItem

    property var serviceInfo: ServersModel.getProcessedServerData("apiServiceInfo")
    property var supportedSitesDrawerRoot

    FlickableType {
        id: fl
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            Item {
                id: focusItem
//                KeyNavigation.tab: backButton
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/history.svg"
                leftText: qsTr("Work period")
                rightText: serviceInfo["timelimit"]
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/gauge.svg"
                leftText: qsTr("Speed")
                rightText: serviceInfo["speed"]
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/info.svg"
                leftText: qsTr("Features")
                rightText: ""
            }

            ParagraphTextType {
                property var features: serviceInfo["features"]

                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                text: features === undefined ? "" : features
            }

            ShowSupportedSitesButton {
                id: supportedSites
                drawerParent: root.supportedSitesDrawerRoot
                sitesList: serviceInfo["sites_list"]
            }

            LabelWithButtonType {
                id: supportUuid
                Layout.fillWidth: true

                text: qsTr("Support uuid")
                descriptionText: SettingsController.getInstallationUuid()

                descriptionOnTop: true

//                parentFlickable: fl
//                KeyNavigation.tab: passwordLabel.eyeButton

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                    if (!GC.isMobile()) {
                        this.rightButton.forceActiveFocus()
                    }
                }
            }

            BasicButtonType {
                id: removeButton
                Layout.topMargin: 24
                Layout.bottomMargin: 16
                Layout.leftMargin: 8
                implicitHeight: 32

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                textColor: AmneziaStyle.color.vibrantRed

                text: qsTr("Remove from application")

//                Keys.onTabPressed: lastItemTabClicked(focusItem)

                clickedFunc: function() {
                    var headerText = qsTr("The site with all data will be removed from the tor network.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot remove server during active connection"))
                        } else {
                            PageController.showBusyIndicator(true)
                            InstallController.removeProcessedServer()
                            PageController.showBusyIndicator(false)
                        }
                    }
                    var noButtonFunction = function() {
                        if (!GC.isMobile()) {
                            removeButton.forceActiveFocus()
                        }
                    }

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }
    }
}
