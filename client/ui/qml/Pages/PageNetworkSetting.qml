import QtQuick 2.12
import QtQuick.Controls 2.12
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"

PageBase {
    id: root
    page: PageEnum.NetworkSettings
    logic: NetworkSettingsLogic

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("DNS Servers")
    }
    LabelType {
        id: l1
        x: 40
        anchors.top: caption.bottom
        width: parent.width - 40
        height: 21
        text: qsTr("Primary DNS server")
    }
    TextFieldType {
        id: dns1
        x: 40
        anchors.top: l1.bottom
        width: parent.width - 90
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
    ImageButtonType {
        id: resetDNS1
        anchors. left: dns1.right
        anchors.leftMargin: 10
        anchors.verticalCenter: dns1.verticalCenter
        width: 24
        height: 24
        icon.source: "qrc:/images/reload.png"
        onClicked: {
            NetworkSettingsLogic.onPushButtonResetDns1Clicked()
        }
    }

    LabelType {
        id: l2
        x: 40
        anchors.top: dns1.bottom
        anchors.topMargin: 20
        width: parent.width - 40
        height: 21
        text: qsTr("Secondray DNS server")
    }
    TextFieldType {
        id: dns2
        x: 40
        anchors.top: l2.bottom
        width: parent.width - 90
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
        id: resetDNS2
        anchors. left: dns2.right
        anchors.leftMargin: 10
        anchors.verticalCenter: dns2.verticalCenter
        width: 24
        height: 24
        icon.source: "qrc:/images/reload.png"
        onClicked: {
            NetworkSettingsLogic.onPushButtonResetDns2Clicked()
        }
    }

    Logo {
        anchors.bottom: parent.bottom
    }
}
