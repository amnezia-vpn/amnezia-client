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
    property bool plansExpanded: false

    readonly property var currentPlan: ApiSubscriptionPlansModel.planAt(selectedPlanIndex)
    readonly property bool anyPlanHasFreeTrial: ApiSubscriptionPlansModel.hasAnyFreeTrial()

    function syncFromModel() {
        root.selectedPlanIndex = ApiSubscriptionPlansModel.recommendedRowIndex()

        var rawHeaderName = String(ApiServicesModel.getSelectedServiceData("name"))
        root.premiumHeaderName = rawHeaderName.replace("Premium", "<span style=\"color: #950051;\">Premium</span>")
        root.premiumHeaderDescription = String(ApiServicesModel.getSelectedServiceData("serviceDescription"))
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
        anchors.bottom: bottomBar.top
        anchors.left: parent.left
        anchors.right: parent.right

        contentHeight: scrollColumn.childrenRect.height + 24

        ColumnLayout {
            id: scrollColumn

            width: flick.width
            spacing: 0

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                Layout.bottomMargin: 12
                visible: !!root.currentPlan && !!root.currentPlan.hasFreeTrial
                radius: 10
                color: AmneziaStyle.color.vibrantGreen
                implicitHeight: trialBadgeLabel.implicitHeight + 8
                implicitWidth: trialBadgeLabel.implicitWidth + 16

                LabelTextType {
                    id: trialBadgeLabel
                    anchors.centerIn: parent
                    text: root.currentPlan ? qsTr("Try free for %1 days").arg(root.currentPlan.trialDays) : ""
                    color: AmneziaStyle.color.midnightBlack
                    font.pixelSize: 11
                    font.weight: Font.Medium
                }
            }

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                headerText: root.premiumHeaderName
                headerTextFormat: Text.RichText
                headerHorizontalAlignment: Text.AlignHCenter
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

                    visible: !root.anyPlanHasFreeTrial || root.plansExpanded || index === root.selectedPlanIndex

                    selected: root.selectedPlanIndex === index
                    billingPeriod: String(model.billingPeriod)
                    priceLabel: String(model.priceLabel)
                    subtitle: String(model.subtitle)
                    showRecommendedBadge: !!model.recommended
                    recommendedText: qsTr("Recommended")

                    onSelectRequested: {
                        root.selectedPlanIndex = index
                        root.plansExpanded = false
                    }
                }
            }

            BasicButtonType {
                id: changePlanButton

                implicitHeight: 25

                Layout.alignment: Qt.AlignRight
                Layout.topMargin: -(root.selectedPlanIndex === ApiSubscriptionPlansModel.rowCount() - 1 ? 24 : 12) + 6
                Layout.rightMargin: 16
                Layout.bottomMargin: 24
                visible: root.anyPlanHasFreeTrial && !root.plansExpanded && ApiSubscriptionPlansModel.rowCount() > 1

                defaultColor: AmneziaStyle.color.transparent
                hoveredColor: AmneziaStyle.color.translucentWhite
                pressedColor: AmneziaStyle.color.sheerWhite
                disabledColor: AmneziaStyle.color.mutedGray
                textColor: AmneziaStyle.color.goldenApricot
                borderFocusedWidth: 0

                text: qsTr("Change plan")

                clickedFunc: function() {
                    root.plansExpanded = true
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

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24
                visible: (Qt.platform.os === "ios" || IsMacOsNeBuild) && !(root.currentPlan && root.currentPlan.hasFreeTrial)
                spacing: 16

                ParagraphTextType {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    textFormat: Text.PlainText
                    color: AmneziaStyle.color.mutedGray
                    font.pixelSize: 12

                    text: qsTr("Charged to your Apple ID at confirmation. Renews automatically unless auto-renew is turned off at least 24 hours before period end. Manage in Apple ID settings.")
                }

                TermsAndPrivacyText {
                    termsUrl: "https://www.apple.com/legal/internet-services/itunes/dev/stdeula/"
                    privacyUrl: LanguageUiController.getCurrentSiteUrl("policy")
                }
            }

            TermsAndPrivacyText {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 24

                visible: !(Qt.platform.os === "ios" || IsMacOsNeBuild)

                termsUrl: String(ApiServicesModel.getSelectedServiceData("termsOfUseUrl"))
                privacyUrl: String(ApiServicesModel.getSelectedServiceData("privacyPolicyUrl"))
            }
        }
    }

    Rectangle {
        id: bottomBar

        z: 2
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        radius: 16
        color: AmneziaStyle.color.onyxBlack
        implicitHeight: bottomBarColumn.implicitHeight + 24

        Rectangle {
            // Squares off the bottom corners so only the top-left/top-right stay rounded,
            // keeping the bar flush with the tab bar directly below it.
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.radius
            color: parent.color
        }

        ColumnLayout {
            id: bottomBarColumn

            anchors.fill: parent
            anchors.margins: 16
            anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin
            spacing: 8

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true

                text: {
                    var plan = root.currentPlan
                    if (!plan) {
                        return qsTr("Continue")
                    }
                    if (plan.hasFreeTrial) {
                        return qsTr("Try free for %1 days").arg(plan.trialDays)
                    }
                    return qsTr("Subscribe — %1 for %2").arg(String(plan.billingPeriod)).arg(String(plan.priceLabel))
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
                    if (Qt.platform.os === "ios" || IsMacOsNeBuild) {
                        PageController.showBusyIndicator(true)
                        var storeId = plan.storeProductId !== undefined ? String(plan.storeProductId) : ""
                        SubscriptionUiController.importPremiumFromAppStore(storeId)
                        PageController.showBusyIndicator(false)
                        return
                    }
                    if (Qt.platform.os === "android") {
                        PageController.showBusyIndicator(true)
                        var androidStoreId = plan.storeProductId !== undefined ? String(plan.storeProductId) : ""
                        SubscriptionUiController.importPremiumFromPlayMarket(androidStoreId)
                        PageController.showBusyIndicator(false)
                        return
                    }
                    if (plan.checkoutUrl) {
                        Qt.openUrlExternally(plan.checkoutUrl)
                        PageController.closePage()
                        PageController.closePage()
                        return
                    }
                }
            }

            ParagraphTextType {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                textFormat: Text.PlainText
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 12

                visible: !!root.currentPlan

                text: {
                    var plan = root.currentPlan
                    if (!plan) {
                        return ""
                    }
                    if (plan.hasFreeTrial) {
                        return qsTr("Free for %1 days. Next, %2/%3, auto-renewal. Cancel at any time in the settings.")
                                .arg(plan.trialDays).arg(String(plan.priceLabel)).arg(String(plan.billingPeriod))
                    }
                    return qsTr("%1/%2, auto-renewal. Cancel at any time in the settings.")
                            .arg(String(plan.priceLabel)).arg(String(plan.billingPeriod))
                }
            }
        }
    }
}
