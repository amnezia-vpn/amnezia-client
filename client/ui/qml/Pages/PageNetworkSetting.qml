import QtQuick
import QtQuick.Controls
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

    CheckBoxType {
        id: cb_amnezia_dns
        anchors.top: caption.bottom
        x: 30
        width: parent.width - 60
        text: qsTr("Use AmneziaDNS service (recommended)")
        checked: NetworkSettingsLogic.checkBoxUseAmneziaDnsChecked
        onCheckedChanged: {
            NetworkSettingsLogic.checkBoxUseAmneziaDnsChecked = checked
            NetworkSettingsLogic.onCheckBoxUseAmneziaDnsToggled(checked)
            UiLogic.onUpdateAllPages()
        }
    }

    LabelType {
        id: lb_amnezia_dns
        x: 30
        anchors.top: cb_amnezia_dns.bottom
        width: parent.width - 60
        text: qsTr("Use AmneziaDNS container on your server, when it installed.\n
Your AmneziaDNS server available only when it installed and VPN connected, it has internal IP address 172.29.172.254\n
If AmneziaDNS service is not installed on the same server, or this option is unchecked, the following DNS servers will be used:")
    }

    LabelType {
        id: l1
        x: 30
        anchors.top: lb_amnezia_dns.bottom
        width: parent.width - 30
        height: 21
        text: qsTr("Primary DNS server")
    }
    TextFieldType {
        id: dns1
        x: 30
        anchors.top: l1.bottom
        width: parent.width - 90
        height: 40
        text: NetworkSettingsLogic.lineEditDns1Text
        onEditingFinished: {
            NetworkSettingsLogic.lineEditDns1Text = text
            NetworkSettingsLogic.onLineEditDns1EditFinished(text)
            UiLogic.onUpdateAllPages()
        }
        validator: RegularExpressionValidator {
            regularExpression: NetworkSettingsLogic.ipAddressRegex
        }
    }
    SvgButtonType {
        id: resetDNS1
        anchors. left: dns1.right
        anchors.leftMargin: 10
        anchors.verticalCenter: dns1.verticalCenter
        width: 24
        height: 24
        icon.source: "qrc:/images/svg/refresh_black_24dp.svg"
        onClicked: {
            NetworkSettingsLogic.onPushButtonResetDns1Clicked()
            UiLogic.onUpdateAllPages()
        }
    }

    LabelType {
        id: l2
        x: 30
        anchors.top: dns1.bottom
        anchors.topMargin: 20
        width: parent.width - 60
        height: 21
        text: qsTr("Secondary DNS server")
    }
    TextFieldType {
        id: dns2
        x: 30
        anchors.top: l2.bottom
        width: parent.width - 90
        height: 40
        text: NetworkSettingsLogic.lineEditDns2Text
        onEditingFinished: {
            NetworkSettingsLogic.lineEditDns2Text = text
            NetworkSettingsLogic.onLineEditDns2EditFinished(text)
            UiLogic.onUpdateAllPages()
        }
        validator: RegularExpressionValidator {
            regularExpression: NetworkSettingsLogic.ipAddressRegex
        }
    }
    SvgButtonType {
        id: resetDNS2
        anchors. left: dns2.right
        anchors.leftMargin: 10
        anchors.verticalCenter: dns2.verticalCenter
        width: 24
        height: 24
        icon.source: "qrc:/images/svg/refresh_black_24dp.svg"
        onClicked: {
            NetworkSettingsLogic.onPushButtonResetDns2Clicked()
            UiLogic.onUpdateAllPages()
        }
    }

    Logo {
        anchors.bottom: parent.bottom
    }
}
