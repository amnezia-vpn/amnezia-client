import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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

    FlickableType {
        id: fl
        anchors.top: caption.bottom
        contentHeight: content.height

        ColumnLayout {
            id: content
            enabled: logic.pageEnabled
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.rightMargin: 15

            CheckBoxType {
                Layout.preferredWidth: parent.width
                text: qsTr("Use AmneziaDNS service (recommended)")
                checked: NetworkSettingsLogic.checkBoxUseAmneziaDnsChecked
                onCheckedChanged: {
                    NetworkSettingsLogic.checkBoxUseAmneziaDnsChecked = checked
                    NetworkSettingsLogic.onCheckBoxUseAmneziaDnsToggled(checked)
                    UiLogic.onUpdateAllPages()
                }
            }

            LabelType {
                Layout.preferredWidth: parent.width
                text: qsTr("Use AmneziaDNS container on your server, when it installed.\n
Your AmneziaDNS server available only when it installed and VPN connected, it has internal IP address 172.29.172.254\n
If AmneziaDNS service is not installed on the same server, or this option is unchecked, the following DNS servers will be used:")
            }

            LabelType {
                Layout.topMargin: 15
                text: qsTr("Primary DNS server")
            }
            TextFieldType {
                height: 40
                implicitWidth: parent.width
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

            UrlButtonType {
                text: qsTr("Reset to default")
                label.horizontalAlignment: Text.AlignLeft
                label.verticalAlignment: Text.AlignTop
                label.font.pixelSize: 14
                icon.source: "qrc:/images/svg/refresh_black_24dp.svg"
                onClicked: {
                    NetworkSettingsLogic.onPushButtonResetDns1Clicked()
                    UiLogic.onUpdateAllPages()
                }
            }

            LabelType {
                text: qsTr("Secondary DNS server")
            }
            TextFieldType {
                height: 40
                implicitWidth: parent.width
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

            UrlButtonType {
                text: qsTr("Reset to default")
                label.horizontalAlignment: Text.AlignLeft
                label.verticalAlignment: Text.AlignTop
                label.font.pixelSize: 14
                icon.source: "qrc:/images/svg/refresh_black_24dp.svg"
                onClicked: {
                    NetworkSettingsLogic.onPushButtonResetDns2Clicked()
                    UiLogic.onUpdateAllPages()
                }
            }
        }
    }
}
