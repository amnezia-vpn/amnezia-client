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

    property string email: ""

    function isValidEmail(value) {
        return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(value)
    }

    function runBusy(action) {
        PageController.showBusyIndicator(true)
        Qt.callLater(function() {
            action()
            PageController.showBusyIndicator(false)
        })
    }

    function submit() {
        var value = emailField.textField.text.trim()
        if (!root.isValidEmail(value)) {
            emailField.errorText = qsTr("Invalid email")
            return
        }
        emailField.errorText = ""
        root.email = value
        root.runBusy(function() {
            SubscriptionUiController.startFreeAccountLogin()
        })
    }

    function openSupport() {
        if (ApiAccountInfoModel.getEmailLink() !== "") {
            PageController.goToPage(PageEnum.PageSettingsApiSupport)
            return
        }
        Qt.openUrlExternally("mailto:support@amnezia.org")
    }

    function errorMessage(kind, retryAfterSec) {
        switch (kind) {
        case "captchaInvalid": return qsTr("CAPTCHA was incorrect. Please try again")
        case "captchaRetry": return qsTr("Please solve the new CAPTCHA to continue")
        case "noNetwork": return qsTr("No connection. Check your internet and try again")
        case "rateLimited": return retryAfterSec > 0 ? qsTr("Too many requests. Try again in %1 s").arg(retryAfterSec)
                                                     : qsTr("Too many requests. Try again later")
        default: return qsTr("Something went wrong. Try again later")
        }
    }

    Component.onDestruction: SubscriptionUiController.cancelFreeAccountLogin()

    Connections {
        target: SubscriptionUiController
        enabled: root.visible

        function onFreeAccountCaptchaPassed() {
            SubscriptionUiController.requestFreeEmailCode(root.email)
        }

        function onFreeEmailCodeSent(retryAfterSec) {
            PageController.goToPage(PageEnum.PageSetupWizardApiFreeCode)
        }

        function onFreeAccountLoginFailed(kind, retryAfterSec) {
            if (kind === "" || kind === "config") {
                return
            }
            PageController.showErrorMessage(root.errorMessage(kind, retryAfterSec))
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

                title: qsTr("Sign in or sign up")
                description: qsTr("Enter your email to sign in or create an account")
            }

            TextFieldType {
                id: emailField

                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Email")
                textField.placeholderText: qsTr("Enter email")
                textField.inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText

                textField.onTextChanged: {
                    if (emailField.errorText !== "") {
                        emailField.errorText = ""
                    }
                }

                textField.onAccepted: root.submit()
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

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

        ButtonType {
            Layout.fillWidth: true

            text: qsTr("Continue")

            onClicked: root.submit()
        }

        LinkButtonType {
            Layout.alignment: Qt.AlignHCenter

            text: qsTr("Can't sign in?")

            onClicked: root.openSupport()
        }
    }
}
