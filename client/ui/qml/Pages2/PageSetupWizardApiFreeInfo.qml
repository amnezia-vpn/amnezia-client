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

    function syncFromModel() {
        root.freeHeaderName = String(ApiServicesModel.getSelectedServiceData("name"))
        root.freeHeaderDescription = String(ApiServicesModel.getSelectedServiceData("serviceDescription"))
    }

    function runBusy(action) {
        PageController.showBusyIndicator(true)
        Qt.callLater(function() {
            action()
            PageController.showBusyIndicator(false)
        })
    }

    function freeAccountErrorMessage(kind, retryAfterSec) {
        switch (kind) {
        case "captchaInvalid": return qsTr("CAPTCHA was incorrect. Please try again")
        case "captchaRetry": return qsTr("Please solve the new CAPTCHA to continue")
        case "noNetwork": return qsTr("No connection. Check your internet and try again")
        case "rateLimited": return retryAfterSec > 0 ? qsTr("Too many requests. Try again in %1 s").arg(retryAfterSec)
                                                     : qsTr("Too many requests. Try again later")
        default: return qsTr("Something went wrong. Try again later")
        }
    }

    Component.onCompleted: syncFromModel()

    Component.onDestruction: SubscriptionUiController.cancelFreeAccountLogin()

    Connections {
        target: SubscriptionUiController
        enabled: root.visible

        function onFreeAccountCaptchaPassed() {
            PageController.goToPage(PageEnum.PageSetupWizardApiFreeEmail)
        }

        function onFreeAccountLoginFailed(kind, retryAfterSec) {
            if (kind === "" || kind === "config") {
                return
            }
            PageController.showErrorMessage(root.freeAccountErrorMessage(kind, retryAfterSec))
        }
    }

    FreeAccountCaptchaDialog {
        active: root.visible
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
                PageController.showBusyIndicator(true)
                var hasPurchasedKey = SubscriptionUiController.hasPurchasedKey()
                PageController.showBusyIndicator(false)

                if (!hasPurchasedKey) {
                    root.runBusy(function() {
                        SubscriptionUiController.startFreeAccountLogin()
                    })
                    return
                }

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
}
