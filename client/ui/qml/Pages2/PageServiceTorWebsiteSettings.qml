import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0

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
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButton.bottom
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
                Layout.fillWidth: true
                Layout.topMargin: 32

                text: qsTr("Website address")
                descriptionText: {
                    var config = ContainersModel.getCurrentlyProcessedContainerConfig()
                    var containerIndex = ContainersModel.getCurrentlyProcessedContainerIndex()
                    return config[ContainerProps.containerTypeToString(containerIndex)]["site"]
                }

                descriptionOnTop: true
                textColor: "#FBB26A"

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: "#D7D8DB"

                clickedFunction: function() {
                    content.copyToClipBoard(descriptionText)
                }
            }

            TextEdit{
                id: clipboard
                visible: false
            }

            function copyToClipBoard(text) {
                clipboard.text = text
                clipboard.selectAll()
                clipboard.copy()
                clipboard.select(0, 0)
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 40
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                onLinkActivated: Qt.openUrlExternally(link)
                textFormat: Text.RichText
                text: qsTr("Use <a href=\"https://www.torproject.org/download/\" style=\"color: #FBB26A;\">Tor Browser</a> to open this url.")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("After installation it takes several minutes while your onion site will become available in the Tor Network.")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("When configuring WordPress set the domain as this onion address.")
            }

            BasicButtonType {
                Layout.topMargin: 24
                Layout.bottomMargin: 16
                Layout.leftMargin: 8
                implicitHeight: 32

                defaultColor: "transparent"
                hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                pressedColor: Qt.rgba(1, 1, 1, 0.12)
                textColor: "#EB5757"

                text: qsTr("Remove website")

                onClicked: {
                    questionDrawer.headerText = qsTr("Some description")
                    questionDrawer.yesButtonText = qsTr("Continue")
                    questionDrawer.noButtonText = qsTr("Cancel")

                    questionDrawer.yesButtonFunction = function() {
                        questionDrawer.visible = false
                        goToPage(PageEnum.PageDeinstalling)
                        InstallController.removeCurrentlyProcessedContainer()
                    }
                    questionDrawer.noButtonFunction = function() {
                        questionDrawer.visible = false
                    }
                    questionDrawer.visible = true
                }
            }
        }

        QuestionDrawer {
            id: questionDrawer
        }
    }
}
