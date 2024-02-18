import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0

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

        Column {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            enabled: ServersModel.isProcessedServerHasWriteAccess()

            ListView {
                id: listview

                width: parent.width
                height: listview.contentItem.height

                clip: true
                interactive: false

                model: SftpConfigModel

                delegate: Item {
                    implicitWidth: listview.width
                    implicitHeight: col.implicitHeight

                    ColumnLayout {
                        id: col

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right

                        spacing: 0

                        HeaderType {
                            Layout.fillWidth: true
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            headerText: qsTr("SFTP settings")
                        }

                        LabelWithButtonType {
                            Layout.fillWidth: true
                            Layout.topMargin: 32

                            text: qsTr("Host")
                            descriptionText: ServersModel.getProcessedServerData("HostName")

                            descriptionOnTop: true

                            rightImageSource: "qrc:/images/controls/copy.svg"
                            rightImageColor: "#D7D8DB"

                            clickedFunction: function() {
                                GC.copyToClipBoard(descriptionText)
                                PageController.showNotificationMessage(qsTr("Copied"))
                            }
                        }

                        LabelWithButtonType {
                            Layout.fillWidth: true

                            text: qsTr("Port")
                            descriptionText: port

                            descriptionOnTop: true

                            rightImageSource: "qrc:/images/controls/copy.svg"
                            rightImageColor: "#D7D8DB"

                            clickedFunction: function() {
                                GC.copyToClipBoard(descriptionText)
                                PageController.showNotificationMessage(qsTr("Copied"))
                            }
                        }

                        LabelWithButtonType {
                            Layout.fillWidth: true

                            text: qsTr("Login")
                            descriptionText: username

                            descriptionOnTop: true

                            rightImageSource: "qrc:/images/controls/copy.svg"
                            rightImageColor: "#D7D8DB"

                            clickedFunction: function() {
                                GC.copyToClipBoard(descriptionText)
                                PageController.showNotificationMessage(qsTr("Copied"))
                            }
                        }

                        LabelWithButtonType {
                            Layout.fillWidth: true

                            text: qsTr("Password")
                            descriptionText: password

                            descriptionOnTop: true

                            rightImageSource: "qrc:/images/controls/copy.svg"
                            rightImageColor: "#D7D8DB"

                            clickedFunction: function() {
                                GC.copyToClipBoard(descriptionText)
                                PageController.showNotificationMessage(qsTr("Copied"))
                            }
                        }

                        BasicButtonType {
                            visible: !GC.isMobile()

                            Layout.fillWidth: true
                            Layout.topMargin: 24
                            Layout.bottomMargin: 24
                            Layout.leftMargin: 16
                            Layout.rightMargin: 16

                            defaultColor: "transparent"
                            hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                            pressedColor: Qt.rgba(1, 1, 1, 0.12)
                            disabledColor: "#878B91"
                            textColor: "#D7D8DB"
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
                            Layout.topMargin: 16
                            Layout.bottomMargin: 16
                            Layout.leftMargin: 8
                            implicitHeight: 32

                            defaultColor: "transparent"
                            hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                            pressedColor: Qt.rgba(1, 1, 1, 0.12)
                            disabledColor: "#878B91"
                            textColor: "#FBB26A"

                            text: qsTr("Detailed instructions")

                            clickedFunc: function() {
//                                Qt.openUrlExternally("https://github.com/amnezia-vpn/desktop-client/releases/latest")
                            }
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

                            text: qsTr("Remove SFTP and all data stored there")

                            clickedFunc: function() {
                                var headerText = qsTr("Remove SFTP and all data stored there?")
                                var yesButtonText = qsTr("Continue")
                                var noButtonText = qsTr("Cancel")

                                var yesButtonFunction = function() {
                                    PageController.goToPage(PageEnum.PageDeinstalling)
                                    InstallController.removeCurrentlyProcessedContainer()
                                }
                                var noButtonFunction = function() {
                                }

                                showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                            }
                        }
                    }
                }
            }
        }
    }
}
