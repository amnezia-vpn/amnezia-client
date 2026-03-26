import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"
import PageEnum 1.0

PageType {
    id: root

    property int selectedPlanIndex: 0
    property string premiumHeaderName: ""
    property string premiumHeaderDescription: ""

    readonly property var currentPlan: ApiSubscriptionPlansModel.planAt(selectedPlanIndex)

    function syncFromModel() {
        root.selectedPlanIndex = ApiSubscriptionPlansModel.recommendedRowIndex()

        root.premiumHeaderName = String(ApiServicesModel.getSelectedServiceData("name"))
        root.premiumHeaderDescription = String(ApiServicesModel.getSelectedServiceData("serviceDescription"))
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

                headerText: root.premiumHeaderName
                descriptionText: root.premiumHeaderDescription
            }

            Repeater {
                model: ApiSubscriptionPlansModel

                delegate: SubscriptionPlanCard {
                    required property int index
                    required property var model

                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: index === ApiSubscriptionPlansModel.rowCount() - 1 ? 24 : 12

                    selected: root.selectedPlanIndex === index
                    primaryLeft: String(model.primaryLeft)
                    primaryRight: String(model.primaryRight)
                    subtitle: String(model.subtitle)
                    showRecommendedBadge: !!model.recommended
                    recommendedText: qsTr("Recommended")

                    onSelectRequested: root.selectedPlanIndex = index
                }
            }

            LabelTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 12

                text: qsTr("Premium features")
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

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                visible: (Qt.platform.os === "ios" || IsMacOsNeBuild)

                horizontalAlignment: Text.AlignHCenter
                textFormat: Text.PlainText
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 12

                text: qsTr("Charged to your Apple ID at confirmation. Renews automatically unless auto-renew is turned off at least 24 hours before period end. Manage in Apple ID settings.")
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
                        .arg(termsUrl).arg(privacyUrl).arg("#FBB26A")
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
                        .arg(termsUrl).arg(privacyUrl).arg("#FBB26A")
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

        text: {
            var plan = root.currentPlan
            if (!plan) {
                return qsTr("Continue")
            }
            return qsTr("Subscribe — %1 for %2").arg(String(plan.primaryLeft)).arg(String(plan.primaryRight))
        }

        clickedFunc: function() {
            var plan = root.currentPlan
            if (!plan) {
                return
            }
            if (plan.isTrial) {
                PageController.goToPage(PageEnum.PageSetupWizardApiTrialEmail)
                return
            }
            if (plan.checkoutUrl) {
                Qt.openUrlExternally(plan.checkoutUrl)
                PageController.closePage()
                PageController.closePage()
                return
            }
            PageController.showBusyIndicator(true)
            var ok = ApiConfigsController.importService()
            PageController.showBusyIndicator(false)
            if (!ok) {
                var endpoint = ApiServicesModel.getStoreEndpoint()
                Qt.openUrlExternally(endpoint)
                PageController.closePage()
                PageController.closePage()
            }
        }
    }
}
