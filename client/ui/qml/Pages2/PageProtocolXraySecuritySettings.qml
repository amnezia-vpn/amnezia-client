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

    // Temporary local state — will be replaced by model roles
    property int selectedSecurity: 2  // 0=None, 1=TLS, 2=Reality

    // Shared TLS + Reality fields
    property string fingerprint: "Mozilla/5.0"
    property string serverName: "cdn.example.com"

    // TLS-only fields
    property string alpn: "HTTP/2"

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    FlickableType {
        id: flickable
        anchors.top: backButton.bottom
        anchors.bottom: saveButton.top
        anchors.left: parent.left
        anchors.right: parent.right
        contentHeight: mainColumn.implicitHeight

        ColumnLayout {
            id: mainColumn
            width: flickable.width
            spacing: 0

            Header2TextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                text: qsTr("Security")
            }

            // ── Radio: None ───────────────────────────────────────────
            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("None")
                checked: root.selectedSecurity === 0
                onClicked: root.selectedSecurity = 0
            }

            DividerType {
            }

            // ── Radio: TLS ────────────────────────────────────────────
            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("TLS")
                checked: root.selectedSecurity === 1
                onClicked: root.selectedSecurity = 1
            }

            DividerType {
            }

            // ── Radio: Reality ────────────────────────────────────────
            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("Reality")
                checked: root.selectedSecurity === 2
                onClicked: root.selectedSecurity = 2
            }

            DividerType {
            }

            // ══════════════════════════════════════════════════════════
            // TLS fields (ALPN + Fingerprint + SNI)
            // ══════════════════════════════════════════════════════════
            ColumnLayout {
                visible: root.selectedSecurity === 1
                Layout.fillWidth: true
                spacing: 0

                DropDownType {
                    id: tlsAlpnDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: root.alpn
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
                            root.alpn = selectedText
                            tlsAlpnDropDown.text = selectedText
                            tlsAlpnDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === root.alpn) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                DropDownType {
                    id: tlsFingerprintDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: root.fingerprint
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
                                name: "Chrome"
                            }
                            ListElement {
                                name: "Firefox"
                            }
                            ListElement {
                                name: "Safari"
                            }
                            ListElement {
                                name: "iOS"
                            }
                            ListElement {
                                name: "Android"
                            }
                            ListElement {
                                name: "Edge"
                            }
                            ListElement {
                                name: "360"
                            }
                            ListElement {
                                name: "QQ"
                            }
                            ListElement {
                                name: "Random"
                            }
                        }
                        clickedFunction: function () {
                            root.fingerprint = selectedText
                            tlsFingerprintDropDown.text = selectedText
                            tlsFingerprintDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === root.fingerprint) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("Server Name (SNI)")
                    textField.text: root.serverName
                    textField.onEditingFinished: root.serverName = textField.text
                }
            }

            // ══════════════════════════════════════════════════════════
            // Reality fields (Fingerprint + SNI)
            // ══════════════════════════════════════════════════════════
            ColumnLayout {
                visible: root.selectedSecurity === 2
                Layout.fillWidth: true
                spacing: 0

                DropDownType {
                    id: realityFingerprintDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: root.fingerprint
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
                                name: "Chrome"
                            }
                            ListElement {
                                name: "Firefox"
                            }
                            ListElement {
                                name: "Safari"
                            }
                            ListElement {
                                name: "iOS"
                            }
                            ListElement {
                                name: "Android"
                            }
                            ListElement {
                                name: "Edge"
                            }
                            ListElement {
                                name: "360"
                            }
                            ListElement {
                                name: "QQ"
                            }
                            ListElement {
                                name: "Random"
                            }
                        }
                        clickedFunction: function () {
                            root.fingerprint = selectedText
                            realityFingerprintDropDown.text = selectedText
                            realityFingerprintDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === root.fingerprint) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("Server Name (SNI)")
                    textField.text: root.serverName
                    textField.onEditingFinished: root.serverName = textField.text
                }
            }

            Item {
                Layout.preferredHeight: 16
            }
        }
    }

    BasicButtonType {
        id: saveButton
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        text: qsTr("Save")
        onClicked: {
            forceActiveFocus()
            // XrayConfigModel.setSecurity(...)
        }
        Keys.onEnterPressed: clicked()
        Keys.onReturnPressed: clicked()
    }
}
