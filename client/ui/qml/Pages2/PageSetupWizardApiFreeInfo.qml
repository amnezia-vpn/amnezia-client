import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property string freeHeaderName: ""
    property string freeHeaderDescription: ""
    property string freeFeaturesHtml: ""

    function syncFromModel() {
        root.freeHeaderName = String(ApiServicesModel.getSelectedServiceData("name"))
        root.freeHeaderDescription = String(ApiServicesModel.getSelectedServiceData("serviceDescription"))
        var text = ApiServicesModel.getSelectedServiceData("features")
        root.freeFeaturesHtml = String(text).replace("%1", LanguageModel.getCurrentSiteUrl("free")).replace("/free", "")
    }

    Component.onCompleted: syncFromModel()

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        onFocusChanged: {
            if (activeFocus) {
                flick.contentY = 0
            }
        }
    }

    FlickableType {
        id: flick

        anchors.top: backButton.bottom
        anchors.bottom: continueButton.top
        anchors.left: parent.left
        anchors.right: parent.right

        contentHeight: scrollColumn.implicitHeight + 24

        ColumnLayout {
            id: scrollColumn

            width: flick.width
            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                headerText: root.freeHeaderName
                descriptionText: root.freeHeaderDescription
            }

            LabelTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 12

                text: qsTr("Available with Free")
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 13
            }

            BenefitsPanel {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                benefitsModel: ApiConfigsController.benefitsModel
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                visible: root.freeFeaturesHtml.length > 0 && ApiConfigsController.benefitsModel.rowCount() === 0

                textFormat: Text.RichText
                text: root.freeFeaturesHtml

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                visible: !(Qt.platform.os === "ios" || IsMacOsNeBuild)

                horizontalAlignment: Text.AlignHCenter
                textFormat: Text.RichText
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 12

                text: {
                    var termsUrl = LanguageModel.getCurrentSiteUrl()
                    var privacyUrl = LanguageModel.getCurrentSiteUrl("policy")
                    return qsTr("By continuing, you agree to the <a href=\"%1\" style=\"color: %3;\">Terms of Use</a> and <a href=\"%2\" style=\"color: %3;\">Privacy Policy</a>")
                        .arg(termsUrl).arg(privacyUrl).arg(Qt.colorToString(AmneziaStyle.color.goldenApricot))
                }

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                visible: (Qt.platform.os === "ios" || IsMacOsNeBuild)

                horizontalAlignment: Text.AlignHCenter
                textFormat: Text.RichText
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 12

                text: {
                    var termsUrl = "https://www.apple.com/legal/internet-services/itunes/dev/stdeula/"
                    var privacyUrl = LanguageModel.getCurrentSiteUrl("policy")
                    return qsTr("By continuing, you agree to the <a href=\"%1\" style=\"color: %3;\">Terms of Use</a> and <a href=\"%2\" style=\"color: %3;\">Privacy Policy</a>")
                        .arg(termsUrl).arg(privacyUrl).arg(Qt.colorToString(AmneziaStyle.color.goldenApricot))
                }

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }
        }
    }

    BasicButtonType {
        id: continueButton

        z: 2
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16 + SettingsController.safeAreaBottomMargin

        text: ApiServicesModel.getSelectedServiceType() === "amnezia-trial" ? qsTr("Try Trial") : qsTr("Continue")

        clickedFunc: function() {
            PageController.showBusyIndicator(true)
            var result = ApiConfigsController.importService()
            PageController.showBusyIndicator(false)

            if (!result) {
                var endpoint = ApiServicesModel.getStoreEndpoint()
                Qt.openUrlExternally(endpoint)
                PageController.closePage()
                PageController.closePage()
            }
        }
    }
}
