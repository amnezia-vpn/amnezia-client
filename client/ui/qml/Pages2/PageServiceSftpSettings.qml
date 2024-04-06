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

    defaultActiveFocusItem: focusItem

    Connections {
        target: InstallController

        function onUpdateContainerFinished() {
            PageController.showNotificationMessage(qsTr("Settings updated successfully"))
        }
    }

    Item {
        id: focusItem
        KeyNavigation.tab: backButton
    }

    ColumnLayout {
        id: backButtonLayout

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
            KeyNavigation.tab: listview
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButtonLayout.bottom
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

                onFocusChanged: {
                    if (focus) {
                        listview.currentItem.focusItem.forceActiveFocus()
                    }
                }

                delegate: Item {
                    implicitWidth: listview.width
                    implicitHeight: col.implicitHeight

                    property alias focusItem: hostLabel.rightButton

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
                            id: hostLabel
                            Layout.fillWidth: true
                            Layout.topMargin: 32

                            parentFlickable: fl
                            KeyNavigation.tab: portLabel.rightButton

                            text: qsTr("Host")
                            descriptionText: ServersModel.getProcessedServerData("hostName")

                            descriptionOnTop: true

                            rightImageSource: "qrc:/images/controls/copy.svg"
                            rightImageColor: "#D7D8DB"

                            clickedFunction: function() {
                                GC.copyToClipBoard(descriptionText)
                                PageController.showNotificationMessage(qsTr("Copied"))
                                if (!GC.isMobile()) {
                                    this.rightButton.forceActiveFocus()
                                }
                            }
                        }

                        LabelWithButtonType {
                            id: portLabel
                            Layout.fillWidth: true

                            text: qsTr("Port")
                            descriptionText: port

                            descriptionOnTop: true

                            parentFlickable: fl
                            KeyNavigation.tab: usernameLabel.rightButton

                            rightImageSource: "qrc:/images/controls/copy.svg"
                            rightImageColor: "#D7D8DB"

                            clickedFunction: function() {
                                GC.copyToClipBoard(descriptionText)
                                PageController.showNotificationMessage(qsTr("Copied"))
                                if (!GC.isMobile()) {
                                    this.rightButton.forceActiveFocus()
                                }
                            }
                        }

                        LabelWithButtonType {
                            id: usernameLabel
                            Layout.fillWidth: true

                            text: qsTr("User name")
                            descriptionText: username

                            descriptionOnTop: true

                            parentFlickable: fl
                            KeyNavigation.tab: passwordLabel.rightButton

                            rightImageSource: "qrc:/images/controls/copy.svg"
                            rightImageColor: "#D7D8DB"

                            clickedFunction: function() {
                                GC.copyToClipBoard(descriptionText)
                                PageController.showNotificationMessage(qsTr("Copied"))
                                if (!GC.isMobile()) {
                                    this.rightButton.forceActiveFocus()
                                }
                            }
                        }

                        LabelWithButtonType {
                            id: passwordLabel
                            Layout.fillWidth: true

                            text: qsTr("Password")
                            descriptionText: password

                            descriptionOnTop: true

                            parentFlickable: fl
                            Keys.onTabPressed: {
                                if (mountButton.visible) {
                                    mountButton.forceActiveFocus()
                                } else {
                                    detailedInstructionsButton.forceActiveFocus()
                                }
                            }

                            rightImageSource: "qrc:/images/controls/copy.svg"
                            rightImageColor: "#D7D8DB"

                            clickedFunction: function() {
                                GC.copyToClipBoard(descriptionText)
                                PageController.showNotificationMessage(qsTr("Copied"))
                                if (!GC.isMobile()) {
                                    this.rightButton.forceActiveFocus()
                                }
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

                            defaultColor: "transparent"
                            hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                            pressedColor: Qt.rgba(1, 1, 1, 0.12)
                            disabledColor: "#878B91"
                            textColor: "#D7D8DB"
                            borderWidth: 1

                            parentFlickable: fl
                            KeyNavigation.tab: detailedInstructionsButton

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

                            defaultColor: "transparent"
                            hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                            pressedColor: Qt.rgba(1, 1, 1, 0.12)
                            disabledColor: "#878B91"
                            textColor: "#FBB26A"

                            text: qsTr("Detailed instructions")

                            parentFlickable: fl
                            KeyNavigation.tab: removeButton

                            clickedFunc: function() {
//                                Qt.openUrlExternally("https://github.com/amnezia-vpn/desktop-client/releases/latest")
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
                            textColor: "#EB5757"

                            parentFlickable: fl
                            Keys.onTabPressed: lastItemTabClicked(focusItem)

                            text: qsTr("Remove SFTP and all data stored there")

                            clickedFunc: function() {
                                var headerText = qsTr("Remove SFTP and all data stored there?")
                                var yesButtonText = qsTr("Continue")
                                var noButtonText = qsTr("Cancel")

                                var yesButtonFunction = function() {
                                    PageController.goToPage(PageEnum.PageDeinstalling)
                                    InstallController.removeProcessedContainer()
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
        }
    }
}
