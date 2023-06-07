import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

import "../../Controls2"
import "../../Controls2/TextTypes"
import "../../Components"

Item {
    id: root

    ColumnLayout {
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
            Layout.fillWidth: true

        }

        DropDownType {
            Layout.fillWidth: true

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
