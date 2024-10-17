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

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/map-pin.svg"
                leftText: qsTr("For the region")
                rightText: ApiServicesModel.getSelectedServiceData("region")
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/tag.svg"
                leftText: qsTr("Price")
                rightText: ApiServicesModel.getSelectedServiceData("price")
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/history.svg"
                leftText: qsTr("Work period")
                rightText: ApiServicesModel.getSelectedServiceData("workPeriod")

                visible: rightText !== ""
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/gauge.svg"
                leftText: qsTr("Speed")
                rightText: ApiServicesModel.getSelectedServiceData("speed")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }
                textFormat: Text.RichText
                text: {
                    var text = ApiServicesModel.getSelectedServiceData("features")
                    if (text === undefined) {
                        return ""
                    }
                    return text.replace("%1", LanguageModel.getCurrentSiteUrl())
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            LabelWithButtonType {
                id: supportUuid
                Layout.fillWidth: true

                text: qsTr("Support tag")
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
                id: resetButton
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 24
                Layout.bottomMargin: 16
                Layout.leftMargin: 8
                implicitHeight: 32

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                textColor: AmneziaStyle.color.vibrantRed

                text: qsTr("Reload API config")

//                Keys.onTabPressed: lastItemTabClicked(focusItem)

                clickedFunc: function() {
                    var headerText = qsTr("Reload API config?")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ServersModel.isDefaultServerCurrentlyProcessed() && ConnectionController.isConnected) {
                            PageController.showNotificationMessage(qsTr("Cannot reload API config during active connection"))
                        } else {
                            PageController.showBusyIndicator(true)
                            InstallController.updateServiceFromApi(ServersModel.processedIndex, "", "", true)
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

            BasicButtonType {
                id: removeButton
                Layout.alignment: Qt.AlignHCenter
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
                    var headerText = qsTr("Remove from application?")
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
