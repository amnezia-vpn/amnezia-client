import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

    FlickableType {
        id: flickable

        anchors.top: parent.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Tor website settings")
            }

            ColumnLayout {
                Layout.fillWidth: true

                spacing: 12

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    text: qsTr("Website address")
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    spacing: 8

                    BodyTextType {
                        id: websiteName

                        Layout.fillWidth: true

                        text: TorConfigModel.site || ""
                        color: AmneziaStyle.color.textAccent
                        wrapMode: Text.WrapAnywhere
                    }

                    IconButtonType {
                        Layout.alignment: Qt.AlignVCenter

                        icon.source: "qrc:/images/controls/copy.svg"
                        Accessible.name: qsTr("Website address")

                        onClicked: {
                            GC.copyToClipBoard(websiteName.text)
                            PageController.showNotificationMessage(qsTr("Copied"))
                        }
                    }
                }
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                textFormat: Text.StyledText
                linkColor: AmneziaStyle.color.textAccent
                text: qsTr("Use <a href=\"https://www.torproject.org/download/\" style=\"color: #FBB26A;\">Tor Browser</a> to open this URL.").replace(/ style="[^"]*"/, "")

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }

                HoverHandler {
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("After creating your onion site, it takes a few minutes for the Tor network to make it available for use.")
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                text: qsTr("When configuring WordPress set the this onion address as domain.")
            }
        }
    }
}
