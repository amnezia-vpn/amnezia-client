import QtQuick
import QtQuick.Layouts

import MtProxyConfig 1.0

import "../Controls2"
import "../Controls2/TextTypes"

ColumnLayout {
    id: root

    property string endpoint
    property string port
    property string endpointPlaceholder
    property string portPlaceholder
    property bool warningVisible: false
    property var scroller: null

    signal endpointEdited(string text)
    signal portEdited(string text)

    spacing: 12

    H2TextType {
        Layout.fillWidth: true
        Layout.topMargin: 28
        Layout.leftMargin: 16
        Layout.rightMargin: 16

        text: qsTr("Cascade connection")
    }

    NotificationType {
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16

        visible: root.warningVisible

        variant: NotificationType.Warning
        text: qsTr("Cascade endpoint and port are the address and port of the entry server that forwards traffic to this server. New client configs will connect to them instead of this server's address and port. Existing configs are not changed")
    }

    TextFieldType {
        id: endpointTextField

        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16

        headerText: qsTr("Cascade endpoint")
        textField.placeholderText: root.endpointPlaceholder
        textField.text: root.endpoint
        textField.maximumLength: 253
        textField.validator: PublicHostInputValidator {}

        textField.onTextEdited: root.endpointEdited(textField.text.trim())

        textField.onActiveFocusChanged: {
            if (textField.activeFocus && root.scroller) {
                root.scroller.scrollToItem(endpointTextField)
            }
        }
    }

    TextFieldType {
        id: portTextField

        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16

        headerText: qsTr("Cascade port")
        textField.placeholderText: root.portPlaceholder
        textField.text: root.port
        textField.maximumLength: 5
        textField.validator: IntValidator { bottom: 1; top: 65535 }

        textField.onTextEdited: root.portEdited(textField.text)

        textField.onActiveFocusChanged: {
            if (textField.activeFocus && root.scroller) {
                root.scroller.scrollToItem(portTextField)
            }
        }
    }
}
