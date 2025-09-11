import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

    Connections {
        target: InstallController

        function onUpdateContainerFinished() {
            PageController.showNotificationMessage(qsTr("Settings updated successfully"))
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        enabled: ServersModel.isProcessedServerHasWriteAccess()

        model: SftpConfigModel

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("SFTP settings")
            }

            LabelWithButtonType {
                id: hostLabel

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Host")
                descriptionText: ServersModel.getProcessedServerData("hostName")

                descriptionOnTop: true

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            LabelWithButtonType {
                id: portLabel

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Port")
                descriptionText: port

                descriptionOnTop: true

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            LabelWithButtonType {
                id: usernameLabel

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("User name")
                descriptionText: username

                descriptionOnTop: true

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            LabelWithButtonType {
                id: passwordLabel

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Password")
                descriptionText: password

                descriptionOnTop: true

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                buttonImageSource: hideDescription ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"

                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            BasicButtonType {
                id: mountButton

                visible: !GC.isMobile()

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.paleGray
                borderWidth: 1

                text: qsTr("Mount folder on device")

                clickedFunc: function() {
                    PageController.showBusyIndicator(true)
                    InstallController.mountSftpDrive(port, password, username)
                    PageController.showBusyIndicator(false)
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                readonly property string windowsFirstLink: "<a href=\"https://github.com/billziss-gh/winfsp/releases/latest\" style=\"color: #FBB26A;\">WinFsp</a>"
                readonly property string windowsSecondLink: "<a href=\"https://github.com/billziss-gh/sshfs-win/releases\" style=\"color: #FBB26A;\">SSHFS-Win</a>"

                readonly property string macosFirstLink: "<a href=\"https://osxfuse.github.io/\" style=\"color: #FBB26A;\">macFUSE</a>"
                readonly property string macosSecondLink: "<a href=\"https://osxfuse.github.io/\" style=\"color: #FBB26A;\">SSHFS</a>"

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }
                textFormat: Text.RichText
                text: {
                    var str = qsTr("In order to mount remote SFTP folder as local drive, perform following steps: <br>")
                    if (Qt.platform.os === "windows") {
                        str += qsTr("<br>1. Install the latest version of ") + windowsFirstLink + "\n"
                        str += qsTr("<br>2. Install the latest version of ") + windowsSecondLink + "\n"
                    } else if (Qt.platform.os === "osx") {
                        str += qsTr("<br>1. Install the latest version of ") + macosFirstLink + "\n"
                        str += qsTr("<br>2. Install the latest version of ") + macosSecondLink + "\n"
                    } else if (Qt.platform.os === "linux") {
                        return ""
                    } else return ""

                    return str
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            BasicButtonType {
                id: detailedInstructionsButton

                Layout.topMargin: 16
                Layout.bottomMargin: 16
                Layout.leftMargin: 8
                implicitHeight: 32

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.goldenApricot

                text: qsTr("Detailed instructions")

                clickedFunc: function() {
                    // Qt.openUrlExternally("https://github.com/amnezia-vpn/desktop-client/releases/latest")
                }
            }
        }
    }
}
