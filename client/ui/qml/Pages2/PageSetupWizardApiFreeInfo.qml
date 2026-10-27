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

    property string freeHeaderName: ""
    property string freeHeaderDescription: ""
    property bool emailSignInRequested: false

    function syncFromModel() {
        root.freeHeaderName = String(ApiServicesModel.getSelectedServiceData("name"))
        root.freeHeaderDescription = String(ApiServicesModel.getSelectedServiceData("serviceDescription"))
    }

    Component.onCompleted: syncFromModel()

    Component.onDestruction: SubscriptionUiController.cancelFreeAccountLogin()

    function startEmailSignIn() {
        if (SubscriptionUiController.startFreeAccountLogin()) {
            PageController.goToPage(PageEnum.PageSetupWizardApiFreeEmail)
        }
    }

    Connections {
        target: SubscriptionUiController
        enabled: root.visible

        function onFreeEmailSignInRequired() {
            root.emailSignInRequested = true
            root.startEmailSignIn()
        }
    }

    FlickableType {
        id: flick

        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        contentHeight: scrollColumn.implicitHeight + 16

        ColumnLayout {
            id: scrollColumn

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: root.freeHeaderName
                description: root.freeHeaderDescription
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 24

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 12

                    visible: benefitsPanel.visible

                    BodyTextType {
                        Layout.fillWidth: true

                        text: qsTr("Free features")
                        color: AmneziaStyle.color.textTertiary
                    }

                    BenefitsPanel {
                        id: benefitsPanel

                        Layout.fillWidth: true

                        benefitsModel: ApiBenefitsModel
                    }
                }

                TermsAndPrivacyText {
                    Layout.fillWidth: true

                    visible: !(Qt.platform.os === "ios" || IsMacOsNeBuild)

                    termsUrl: String(ApiServicesModel.getSelectedServiceData("termsOfUseUrl"))
                    privacyUrl: String(ApiServicesModel.getSelectedServiceData("privacyPolicyUrl"))
                }

                TermsAndPrivacyText {
                    Layout.fillWidth: true

                    visible: (Qt.platform.os === "ios" || IsMacOsNeBuild)

                    termsUrl: "https://www.apple.com/legal/internet-services/itunes/dev/stdeula/"
                    privacyUrl: LanguageUiController.getCurrentSiteUrl("policy")
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        ButtonType {
            id: continueButton

            Layout.fillWidth: true

            text: qsTr("Continue")

            onClicked: {
                if (ApiServicesModel.getSelectedServiceData("emailRegistrationRequired") === true) {
                    root.startEmailSignIn()
                    return
                }

                root.emailSignInRequested = false
                PageController.showBusyIndicator(true)
                var result = SubscriptionUiController.importFreeFromGateway()
                PageController.showBusyIndicator(false)

                if (!result) {
                    if (SubscriptionUiController.isCaptchaAwaitingUser() || root.emailSignInRequested) {
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
}
