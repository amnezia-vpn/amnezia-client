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
    readonly property bool storePurchaseAvailable: Qt.platform.os === "ios" || IsMacOsNeBuild
                                                   || (Qt.platform.os === "android" && IsPlayBuild)

    function proceedWithPurchase(plan) {
        var storeId = plan.storeProductId !== undefined ? String(plan.storeProductId) : ""
        if (root.storePurchaseAvailable) {
            PageController.showBusyIndicator(true)
            SubscriptionUiController.importPremiumFromStore(storeId)
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

    function syncFromModel() {
        root.selectedPlanIndex = ApiSubscriptionPlansModel.recommendedRowIndex()

        root.premiumHeaderName = String(ApiServicesModel.getSelectedServiceData("name"))
        root.premiumHeaderDescription = String(ApiServicesModel.getSelectedServiceData("serviceDescription"))
    }

    Component.onCompleted: syncFromModel()

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

                title: root.premiumHeaderName
                description: root.premiumHeaderDescription
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 24

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 12

                    StatusBadgeType {
                        Layout.alignment: Qt.AlignLeft

                        visible: !!root.currentPlan && !!root.currentPlan.hasFreeTrial

                        variant: StatusBadgeType.Success
                        dot: true
                        text: root.currentPlan ? qsTr("Try free for %n day(s)", "", Number(root.currentPlan.trialDays)) : ""
                    }

                    Repeater {
                        model: ApiSubscriptionPlansModel

                        delegate: PurchaseOptionType {
                            required property int index
                            required property var model

                            Layout.fillWidth: true

                            visible: !root.anyPlanHasFreeTrial || root.plansExpanded || index === root.selectedPlanIndex

                            checked: root.selectedPlanIndex === index
                            text: String(model.billingPeriod)
                            trailingText: String(model.priceLabel)
                            subtitle: String(model.subtitle)
                            badgeText: model.recommended ? qsTr("Recommended") : ""

                            onClicked: {
                                root.selectedPlanIndex = index
                                root.plansExpanded = false
                            }
                        }
                    }

                    LinkButtonType {
                        id: changePlanButton

                        Layout.alignment: Qt.AlignRight

                        visible: root.anyPlanHasFreeTrial && !root.plansExpanded && ApiSubscriptionPlansModel.rowCount() > 1

                        text: qsTr("Change plan")

                        onClicked: {
                            root.plansExpanded = true
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 12

                    visible: benefitsPanel.benefitsCount > 0

                    BodyTextType {
                        Layout.fillWidth: true

                        text: qsTr("Premium features")
                        color: AmneziaStyle.color.textTertiary
                    }

                    BenefitsPanel {
                        id: benefitsPanel

                        Layout.fillWidth: true

                        benefitsModel: ApiBenefitsModel
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true

                    visible: Qt.platform.os === "ios" || IsMacOsNeBuild
                    spacing: 16

                    CaptionTextType {
                        Layout.fillWidth: true

                        horizontalAlignment: Text.AlignHCenter
                        textFormat: Text.PlainText

                        text: qsTr("Charged to your Apple ID at confirmation. Renews automatically unless auto-renew is turned off at least 24 hours before period end. Manage in Apple ID settings.")
                    }

                    TermsAndPrivacyText {
                        termsUrl: "https://www.apple.com/legal/internet-services/itunes/dev/stdeula/"
                        privacyUrl: LanguageUiController.getCurrentSiteUrl("policy")
                    }
                }

                TermsAndPrivacyText {
                    Layout.fillWidth: true

                    visible: !(Qt.platform.os === "ios" || IsMacOsNeBuild)

                    termsUrl: String(ApiServicesModel.getSelectedServiceData("termsOfUseUrl"))
                    privacyUrl: String(ApiServicesModel.getSelectedServiceData("privacyPolicyUrl"))
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        spacing: 8

        ButtonType {
            id: continueButton

            Layout.fillWidth: true

            text: {
                var plan = root.currentPlan
                if (!plan) {
                    return qsTr("Continue")
                }
                if (plan.hasFreeTrial) {
                    return qsTr("Start %n-day free trial", "", Number(plan.trialDays))
                }
                return qsTr("Subscribe — %1 for %2").arg(String(plan.billingPeriod)).arg(String(plan.priceLabel))
            }

            onClicked: {
                var plan = root.currentPlan
                if (!plan) {
                    return
                }
                if (plan.isTrial) {
                    PageController.goToPage(PageEnum.PageSetupWizardApiTrialEmail)
                    return
                }

                if (root.storePurchaseAvailable) {
                    var active = SubscriptionUiController.currentActivePlanInfo()
                    if (active && active.hasActivePlan) {
                        var planPrice = Number(plan.priceAmount)
                        var activePrice = Number(active.priceAmount)
                        var pricesKnown = planPrice > 0 && activePrice > 0

                        var headerText
                        var descriptionText
                        if (pricesKnown && planPrice > activePrice) {
                            headerText = qsTr("Upgrade plan?")
                            descriptionText = qsTr("The current plan will be replaced with the %1 / %2 plan. The change will take effect immediately after confirmation")
                                    .arg(String(plan.priceLabel)).arg(String(plan.billingPeriod))
                        } else if (pricesKnown && planPrice < activePrice) {
                            headerText = qsTr("Downgrade plan?")
                            descriptionText = qsTr("The current plan will be replaced with the %1 / %2 plan. The store will apply the change based on its billing rules")
                                    .arg(String(plan.priceLabel)).arg(String(plan.billingPeriod))
                        } else {
                            headerText = qsTr("Confirm subscription change?")
                            descriptionText = qsTr("An active subscription already exists. The current plan will be replaced with the %1 / %2 plan")
                                    .arg(String(plan.priceLabel)).arg(String(plan.billingPeriod))
                        }

                        Dialogs.askQuestion(headerText, descriptionText, qsTr("Continue"), qsTr("Cancel"),
                            function() { root.proceedWithPurchase(plan) },
                            function() {})
                        return
                    }
                }

                root.proceedWithPurchase(plan)
            }
        }

        CaptionTextType {
            Layout.fillWidth: true

            horizontalAlignment: Text.AlignHCenter
            textFormat: Text.PlainText

            visible: !!root.currentPlan && root.storePurchaseAvailable

            text: {
                var plan = root.currentPlan
                if (!plan) {
                    return ""
                }
                if (plan.hasFreeTrial) {
                    return qsTr("%n day(s) free, then %1/%2. Auto-renews until canceled. Cancel anytime in Settings.", "", Number(plan.trialDays))
                            .arg(String(plan.priceLabel)).arg(String(plan.billingPeriod))
                }
                return qsTr("%1/%2, auto-renewal. Cancel anytime in the Settings.")
                        .arg(String(plan.priceLabel)).arg(String(plan.billingPeriod))
            }
        }
    }
}
