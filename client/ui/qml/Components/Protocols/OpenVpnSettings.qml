import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

import "../../Controls2"
import "../../Controls2/TextTypes"
import "../../Components"

Item {
    id: root
    implicitHeight: col.implicitHeight
    implicitWidth: col.implicitWidth

    ColumnLayout {
        id: col

        anchors.fill: parent

        anchors.leftMargin: 16
        anchors.rightMargin: 16

        spacing: 16

        Header2TextType {
            Layout.fillWidth: true

            text: "OpenVpn"
        }

        TextFieldWithHeaderType {
            Layout.fillWidth: true

            headerText: qsTr("VPN Addresses Subnet")
        }

        ParagraphTextType {
            Layout.fillWidth: true

            text: qsTr("Network protocol")
        }

        TransportProtoSelector {
            Layout.fillWidth: true
        }

        TextFieldWithHeaderType {
            Layout.fillWidth: true

            headerText: qsTr("Port")
        }

        SwitcherType {
            Layout.fillWidth: true
            text: qsTr("Auto-negotiate encryption")
        }

        DropDownType {
            id: hash
            Layout.fillWidth: true
            implicitHeight: 74

            rootButtonBorderWidth: 0

            descriptionText: qsTr("Hash")
            headerText: qsTr("Hash")

            listView: ListViewType {
                rootWidth: root.width

                model: ListModel {
                    ListElement { name : qsTr("SHA512") }
                    ListElement { name : qsTr("SHA384") }
                    ListElement { name : qsTr("SHA256") }
                    ListElement { name : qsTr("SHA3-512") }
                    ListElement { name : qsTr("SHA3-384") }
                    ListElement { name : qsTr("SHA3-256") }
                    ListElement { name : qsTr("whirlpool") }
                    ListElement { name : qsTr("BLAKE2b512") }
                    ListElement { name : qsTr("BLAKE2s256") }
                    ListElement { name : qsTr("SHA1") }
                }
                currentIndex: 0

                clickedFunction: {
                    hash.text = selectedText
                    hash.menuVisible = false
                }

                Component.onCompleted: {
                    hash.text = selectedText
                }
            }
        }

        DropDownType {
            id: cipher
            Layout.fillWidth: true
            implicitHeight: 74

            rootButtonBorderWidth: 0

            descriptionText: qsTr("Cipher")
            headerText: qsTr("Cipher")

            listView: ListViewType {
                rootWidth: root.width

                model: ListModel {
                    ListElement { name : qsTr("AES-256-GCM") }
                    ListElement { name : qsTr("AES-192-GCM") }
                    ListElement { name : qsTr("AES-128-GCM") }
                    ListElement { name : qsTr("AES-256-CBC") }
                    ListElement { name : qsTr("AES-192-CBC") }
                    ListElement { name : qsTr("AES-128-CBC") }
                    ListElement { name : qsTr("ChaCha20-Poly1305") }
                    ListElement { name : qsTr("ARIA-256-CBC") }
                    ListElement { name : qsTr("CAMELLIA-256-CBC") }
                    ListElement { name : qsTr("none") }
                }
                currentIndex: 0

                clickedFunction: {
                    cipher.text = selectedText
                    cipher.menuVisible = false
                }

                Component.onCompleted: {
                    cipher.text = selectedText
                }
            }
        }

        CheckBoxType {
            Layout.fillWidth: true

            text: qsTr("TLS auth")
        }

        CheckBoxType {
            Layout.fillWidth: true

            text: qsTr("Block DNS requests outside of VPN")
        }

        SwitcherType {
            Layout.fillWidth: true

            text: qsTr("Additional configuration commands")
        }
    }
}
