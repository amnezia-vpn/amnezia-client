import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

    FlickableType {
        id: flickable

        anchors.top: parent.top

        enabled: ServersUiController.isProcessedServerHasWriteAccess()

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("SFTP settings")
            }

            ListItemType {
                id: hostLabel

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                dense: true
                overline: qsTr("Host")
                text: ServersUiController.serverHostName(ServersUiController.processedServerId)
                Accessible.name: overline + " " + text

                trailingIconSource: "qrc:/images/controls/copy.svg"

                onClicked: {
                    GC.copyToClipBoard(text)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            ListItemType {
                id: portLabel

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                dense: true
                overline: qsTr("Port")
                text: SftpConfigModel.port
                Accessible.name: overline + " " + text

                trailingIconSource: "qrc:/images/controls/copy.svg"

                onClicked: {
                    GC.copyToClipBoard(text)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            ListItemType {
                id: usernameLabel

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                dense: true
                overline: qsTr("User name")
                text: SftpConfigModel.username
                Accessible.name: overline + " " + text

                trailingIconSource: "qrc:/images/controls/copy.svg"

                onClicked: {
                    GC.copyToClipBoard(text)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            ListItemType {
                id: passwordLabel

                property bool masked: true

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                showSeparator: false
                dense: true
                overline: qsTr("Password")
                text: masked ? "*".repeat(SftpConfigModel.password.length) : SftpConfigModel.password
                Accessible.name: overline

                trailingIconSource: "qrc:/images/controls/copy.svg"

                onClicked: {
                    GC.copyToClipBoard(SftpConfigModel.password)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }

                IconButtonType {
                    icon.source: passwordLabel.masked ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"
                    Accessible.name: passwordLabel.masked ? qsTr("Show password") : qsTr("Hide password")

                    onClicked: passwordLabel.masked = !passwordLabel.masked
                }
            }

            ButtonType {
                id: mountButton

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: !GC.isMobile()
                variant: ButtonType.Secondary

                text: qsTr("Mount folder on device")

                onClicked: {
                    PageController.showBusyIndicator(true)
                    InstallController.mountSftpDrive(ServersUiController.processedServerId, SftpConfigModel.port, SftpConfigModel.password, SftpConfigModel.username)
                    PageController.showBusyIndicator(false)
                }
            }

            BodyTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                readonly property string windowsFirstLink: "<a href=\"https://github.com/billziss-gh/winfsp/releases/latest\">WinFsp</a>"
                readonly property string windowsSecondLink: "<a href=\"https://github.com/billziss-gh/sshfs-win/releases\">SSHFS-Win</a>"

                readonly property string macosFirstLink: "<a href=\"https://osxfuse.github.io/\">macFUSE</a>"
                readonly property string macosSecondLink: "<a href=\"https://osxfuse.github.io/\">SSHFS</a>"

                visible: text !== ""
                color: AmneziaStyle.color.textPrimary
                linkColor: AmneziaStyle.color.textAccent
                textFormat: Text.StyledText
                text: {
                    var str = qsTr("In order to mount remote SFTP folder as local drive, perform following steps: <br>")
                    if (Qt.platform.os === "windows") {
                        str += qsTr("<br>1. Install the latest version of ") + windowsFirstLink + "\n"
                        str += qsTr("<br>2. Install the latest version of ") + windowsSecondLink + "\n"
                    } else if (Qt.platform.os === "osx") {
                        str += qsTr("<br>1. Install the latest version of ") + macosFirstLink + "\n"
                        str += qsTr("<br>2. Install the latest version of ") + macosSecondLink + "\n"
                    } else {
                        return ""
                    }

                    return str
                }

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }

                HoverHandler {
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            LinkButtonType {
                id: detailedInstructionsButton

                Layout.topMargin: 12
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                size: LinkButtonType.Medium
                horizontalPadding: 14
                verticalPadding: 8

                text: qsTr("Detailed instructions")
            }

            Item {
                Layout.preferredHeight: 16 + PageController.safeAreaBottomMargin
            }
        }
    }
}
