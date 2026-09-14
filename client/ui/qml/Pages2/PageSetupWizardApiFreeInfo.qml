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

    function syncFromModel() {
        root.freeHeaderName = String(ApiServicesModel.getSelectedServiceData("name"))
        root.freeHeaderDescription = String(ApiServicesModel.getSelectedServiceData("serviceDescription"))
    }

    Component.onCompleted: syncFromModel()

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

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

        contentHeight: scrollColumn.childrenRect.height + 24

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

                text: qsTr("Free features")
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 13
            }

            BenefitsPanel {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                benefitsModel: ApiBenefitsModel
            }

            TermsAndPrivacyText {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                visible: !(Qt.platform.os === "ios" || IsMacOsNeBuild)

                termsUrl: String(ApiServicesModel.getSelectedServiceData("termsOfUseUrl"))
                privacyUrl: String(ApiServicesModel.getSelectedServiceData("privacyPolicyUrl"))
            }

            TermsAndPrivacyText {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                visible: (Qt.platform.os === "ios" || IsMacOsNeBuild)

                termsUrl: "https://www.apple.com/legal/internet-services/itunes/dev/stdeula/"
                privacyUrl: LanguageUiController.getCurrentSiteUrl("policy")
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
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin

        text: qsTr("Continue")

        clickedFunc: function() {
            PageController.showBusyIndicator(true)
            var result = SubscriptionUiController.importFreeFromGateway()
            PageController.showBusyIndicator(false)

            if (!result) {
                if (SubscriptionUiController.isCaptchaAwaitingUser()) {
                    return
                }
                var endpoint = ApiServicesModel.getStoreEndpoint()
                Qt.openUrlExternally(endpoint)
                PageController.closePage()
                PageController.closePage()
            }
        }
    }
}
