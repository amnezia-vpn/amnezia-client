import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    Connections {
        target: InstallController

        function onUpdateContainerFinished() {
            PageController.showNotificationMessage(qsTr("Settings updated successfully"))
        }
    }

    ColumnLayout {
        id: backButtonLayout

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButtonLayout.bottom
        anchors.bottom: parent.bottom
        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            HeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Tor website settings")
            }

            LabelWithButtonType {
                id: websiteName
                Layout.fillWidth: true
                Layout.topMargin: 32

                text: qsTr("Website address")
                descriptionText: {
                    var containerIndex = ContainersModel.getProcessedContainerIndex()
                    var config = ContainersModel.getContainerConfig(containerIndex)
                    return config[ContainerProps.containerTypeToString(containerIndex)]["site"]
                }

                descriptionOnTop: true
                textColor: AmneziaStyle.color.goldenApricot

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

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 40
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                onLinkActivated: Qt.openUrlExternally(link)
                textFormat: Text.RichText
                text: qsTr("Use <a href=\"https://www.torproject.org/download/\" style=\"color: #FBB26A;\">Tor Browser</a> to open this URL.")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("After creating your onion site, it takes a few minutes for the Tor network to make it available for use.")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("When configuring WordPress set the this onion address as domain.")
            }
        }
    }
}
