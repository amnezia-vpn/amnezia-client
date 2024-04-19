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

    defaultActiveFocusItem: listview

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
        contentHeight: listview.implicitHeight

        ListView {
            id: listview

            width: parent.width
            height: listview.contentItem.height

            clip: true
            interactive: false

            model: Socks5ProxyConfigModel

            onFocusChanged: {
                if (focus) {
                    listview.currentItem.focusItemId.forceActiveFocus()
                }
            }

            delegate: Item {
                implicitWidth: listview.width
                implicitHeight: content.implicitHeight

                property alias focusItemId: portTextField.textField

                ColumnLayout {
                    id: content

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16


                    spacing: 0

                    HeaderType {
                        Layout.fillWidth: true

                        headerText: qsTr("SOCKS5 settings")
                    }

                    TextFieldWithHeaderType {
                        id: portTextField

                        Layout.fillWidth: true
                        Layout.topMargin: 40
                        parentFlickable: fl

                        headerText: qsTr("Port")
                        textFieldText: port
                        textField.maximumLength: 5
                        textField.validator: IntValidator { bottom: 1; top: 65535 }

                        textField.onEditingFinished: {
                            if (textFieldText !== port) {
                                port = textFieldText
                            }
                        }

                        KeyNavigation.tab: usernameTextField.textField
                    }

                    TextFieldWithHeaderType {
                        id: usernameTextField

                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        parentFlickable: fl

                        headerText: qsTr("Username")
                        textFieldText: username

                        textField.onEditingFinished: {
                            if (textFieldText !== port) {
                                port = textFieldText
                            }
                        }

                        KeyNavigation.tab: passwordTextField.textField
                    }

                    TextFieldWithHeaderType {
                        id: passwordTextField

                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        parentFlickable: fl

                        headerText: qsTr("Password")
                        textFieldText: password

                        textField.onEditingFinished: {
                            if (textFieldText !== port) {
                                port = textFieldText
                            }
                        }

                        KeyNavigation.tab: removeButton
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

                        text: qsTr("Remove SOCKS5 proxy server")

                        Keys.onTabPressed: lastItemTabClicked(focusItem)

                        clickedFunc: function() {
                            var headerText = qsTr("The site with all data will be removed from the tor network.")
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
