import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import "../Controls"
import "../Config"

Item {
    id: root
    BackButton {
        id: back
    }
    Text {
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 24
        color: "#100A44"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: qsTr("DNS Servers")
        x: 10
        y: 35
        width: 361
        height: 31
    }
    Image {
        anchors.horizontalCenter: root.horizontalCenter
        width: GC.trW(150)
        height: GC.trH(22)
        y: GC.trY(590)
        source: "qrc:/images/AmneziaVPN.png"
    }
    LabelType {
        x: 40
        y: 95
        width: 291
        height: 21
        text: qsTr("Primary DNS server")
    }
    LabelType {
        x: 40
        y: 175
        width: 291
        height: 21
        text: qsTr("Secondray DNS server")
    }
    TextFieldType {
        id: dns1
        x: 40
        y: 120
        width: 271
        height: 40
        text: NetworkSettingsLogic.lineEditDns1Text
        onEditingFinished: {
            NetworkSettingsLogic.lineEditDns1Text = text
            NetworkSettingsLogic.onLineEditDns1EditFinished(text)
        }
        validator: RegExpValidator {
            regExp: NetworkSettingsLogic.ipAddressValidatorRegex
        }
    }
    TextFieldType {
        id: dns2
        x: 40
        y: 200
        width: 271
        height: 40
        text: NetworkSettingsLogic.lineEditDns2Text
        onEditingFinished: {
            NetworkSettingsLogic.lineEditDns2Text = text
            NetworkSettingsLogic.onLineEditDns2EditFinished(text)
        }
        validator: RegExpValidator {
            regExp: NetworkSettingsLogic.ipAddressValidatorRegex
        }
    }
    ImageButtonType {
        id: resetDNS1
        x: 320
        y: 127
        width: 24
        height: 24
        icon.source: "qrc:/images/reload.png"
        onClicked: {
            NetworkSettingsLogic.onPushButtonResetDns1Clicked()
        }
    }
    ImageButtonType {
        id: resetDNS2
        x: 320
        y: 207
        width: 24
        height: 24
        icon.source: "qrc:/images/reload.png"
        onClicked: {
            NetworkSettingsLogic.onPushButtonResetDns2Clicked()
        }
    }
}
