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

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    ListViewType {
        id: listView
        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        model: XrayConfigModel

        delegate: ColumnLayout {
            width: listView.width
            spacing: 0

            Header2TextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                text: qsTr("Security")
            }

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("None")
                checked: security === "none"
                onClicked: security = "none"
            }

            DividerType {
            }

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("TLS")
                checked: security === "tls"
                onClicked: security = "tls"
            }

            DividerType {
            }

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Reality")
                checked: security === "reality"
                onClicked: security = "reality"
            }

            DividerType {
            }

            // ── TLS fields ────────────────────────────────────────────
            ColumnLayout {
                visible: security === "tls"
                Layout.fillWidth: true
                spacing: 0

                DropDownType {
                    id: tlsAlpnDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: alpn
                    descriptionText: qsTr("ALPN")
                    headerText: qsTr("ALPN")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            ListElement {
                                name: "HTTP/2"
                            }
                            ListElement {
                                name: "HTTP/1.1"
                            }
                            ListElement {
                                name: "HTTP/2,HTTP/1.1"
                            }
                        }
                        clickedFunction: function () {
                            alpn = selectedText
                            tlsAlpnDropDown.text = selectedText
                            tlsAlpnDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === alpn) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            tlsAlpnDropDown.text = alpn
                        }
                    }
                }

                DropDownType {
                    id: tlsFingerprintDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: fingerprint
                    descriptionText: qsTr("Fingerprint")
                    headerText: qsTr("Fingerprint")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            ListElement {
                                name: "Mozilla/5.0"
                            }
                            ListElement {
                                name: "chrome"
                            }
                            ListElement {
                                name: "firefox"
                            }
                            ListElement {
                                name: "safari"
                            }
                            ListElement {
                                name: "ios"
                            }
                            ListElement {
                                name: "android"
                            }
                            ListElement {
                                name: "edge"
                            }
                            ListElement {
                                name: "360"
                            }
                            ListElement {
                                name: "qq"
                            }
                            ListElement {
                                name: "random"
                            }
                        }
                        clickedFunction: function () {
                            fingerprint = selectedText
                            tlsFingerprintDropDown.text = selectedText
                            tlsFingerprintDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === fingerprint) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            tlsFingerprintDropDown.text = fingerprint
                        }
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("Server Name (SNI)")
                    textField.text: sni
                    textField.onEditingFinished: {
                        if (textField.text !== sni) sni = textField.text
                    }
                }
            }

            // ── Reality fields ────────────────────────────────────────
            ColumnLayout {
                visible: security === "reality"
                Layout.fillWidth: true
                spacing: 0

                DropDownType {
                    id: realityFingerprintDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: fingerprint
                    descriptionText: qsTr("Fingerprint")
                    headerText: qsTr("Fingerprint")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            ListElement {
                                name: "Mozilla/5.0"
                            }
                            ListElement {
                                name: "chrome"
                            }
                            ListElement {
                                name: "firefox"
                            }
                            ListElement {
                                name: "safari"
                            }
                            ListElement {
                                name: "ios"
                            }
                            ListElement {
                                name: "android"
                            }
                            ListElement {
                                name: "edge"
                            }
                            ListElement {
                                name: "360"
                            }
                            ListElement {
                                name: "qq"
                            }
                            ListElement {
                                name: "random"
                            }
                        }
                        clickedFunction: function () {
                            fingerprint = selectedText
                            realityFingerprintDropDown.text = selectedText
                            realityFingerprintDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === fingerprint) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            realityFingerprintDropDown.text = fingerprint
                        }
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("Server Name (SNI)")
                    textField.text: sni
                    textField.onEditingFinished: {
                        if (textField.text !== sni) sni = textField.text
                    }
                }
            }

            Item {
                Layout.preferredHeight: 16
            }
        }
    }

    //     BasicButtonType {
    //         id: saveButton
    //         anchors.bottom: parent.bottom
    //         anchors.left: parent.left
    //         anchors.right: parent.right
    //         anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin
    //         anchors.leftMargin: 16
    //         anchors.rightMargin: 16
    //         text: qsTr("Save")
    //         onClicked: {
    //             forceActiveFocus()
    //             PageController.closePage()
    //         }
    //         Keys.onEnterPressed: clicked()
    //         Keys.onReturnPressed: clicked()
    //     }
}

