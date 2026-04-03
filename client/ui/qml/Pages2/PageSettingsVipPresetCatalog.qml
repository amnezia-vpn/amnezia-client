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

    property var profiles: []
    readonly property bool canManageProfiles: FBLinkController.canManageRoutingProfiles
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)
    readonly property var systemProfiles: root.profiles.filter(function(profile) { return profile.kind === "system" })
    readonly property var copiedTemplateCodes: root.profiles
        .filter(function(profile) { return profile.kind !== "system" && profile.template_code && profile.template_code.length > 0 })
        .map(function(profile) { return String(profile.template_code) })

    function actionLabel(action) { return action === "proxy" ? qsTr("ЧЕРЕЗ VPN") : qsTr("БЕЗ VPN") }
    function actionTone(action) { return action === "proxy" ? "proxy" : "direct" }
    function isAdded(profile) {
        const code = String(profile.code || "")
        return code.length > 0 && root.copiedTemplateCodes.indexOf(code) !== -1
    }

    function openMyProfiles() {
        PageController.goToPage(PageEnum.PageSettingsVipRoutingProfiles)
    }

    function addPreset(profile) {
        if (!root.canManageProfiles) {
            PageController.goToPage(PageEnum.PageFBLinkSubscription)
            return
        }
        FBLinkController.copySystemRoutingProfile(String(profile.code || ""))
    }

    Connections {
        target: FBLinkController

        function onRoutingProfilesFetched(profiles) {
            root.profiles = profiles
        }

        function onRoutingProfilesError(errorMessage) {
            PageController.showErrorMessage(errorMessage)
        }

        function onRoutingSystemProfileCopied(profile, created) {
            const profileName = profile && profile.name ? String(profile.name) : qsTr("пресет")
            const message = created
                ? qsTr("Пресет «%1» добавлен в мои профили").arg(profileName)
                : qsTr("Пресет уже добавлен в мои профили")
            PageController.showNotificationMessage(message)
            FBLinkController.fetchRoutingProfiles()
        }
    }

    Component.onCompleted: {
        if (FBLinkController.isLoggedIn) {
            FBLinkController.fetchRoutingProfiles()
        }
    }

    Flickable {
        anchors.fill: parent
        clip: true
        contentHeight: content.implicitHeight + 28

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Item {
            width: parent.width
            height: content.implicitHeight + 28

            ColumnLayout {
                id: content
                width: Math.min(root.maxContentWidth, parent.width - root.sideMargin * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                spacing: 10

                BackButtonType {
                    Layout.topMargin: 16 + SettingsController.safeAreaTopMargin
                    Layout.leftMargin: 4
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 12
                    accentVisible: true
                    accentColor: "#10B981"

                    LabelTextType {
                        Layout.fillWidth: true
                        text: qsTr("Каталог системных пресетов")
                        font.pixelSize: root.wideLayout ? 24 : 21
                        font.weight: 700
                        color: FBLinkStyle.color.paleGray
                        wrapMode: Text.WordWrap
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        text: qsTr("Здесь готовые пресеты. Добавьте нужный и он появится в «Моих профилях».")
                        color: FBLinkStyle.color.mutedGray
                        wrapMode: Text.WordWrap
                    }
                }

                Repeater {
                    model: root.systemProfiles
                    delegate: PremiumPanel {
                        Layout.fillWidth: true
                        padding: 12
                        fillColor: Qt.rgba(1, 1, 1, 0.03)
                        outlineColor: Qt.rgba(1, 1, 1, 0.06)
                        property var profileData: modelData

                        LabelTextType {
                            Layout.fillWidth: true
                            text: profileData.name || qsTr("Без названия")
                            font.pixelSize: 16
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
                            wrapMode: Text.WordWrap
                        }

                        Flow {
                            Layout.fillWidth: true
                            width: parent ? parent.width : 0
                            spacing: 8
                            PremiumBadge { text: root.actionLabel(profileData.action || "direct"); tone: root.actionTone(profileData.action || "direct") }
                            PremiumBadge { text: root.isAdded(profileData) ? qsTr("УЖЕ ДОБАВЛЕН") : qsTr("НЕ ДОБАВЛЕН"); tone: root.isAdded(profileData) ? "success" : "neutral" }
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            text: profileData.description || ""
                            color: FBLinkStyle.color.mutedGray
                            wrapMode: Text.WordWrap
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 44
                            enabled: root.canManageProfiles
                            text: root.isAdded(profileData) ? qsTr("Открыть мои профили") : qsTr("Добавить в мои профили")
                            defaultColor: root.isAdded(profileData) ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(234/255, 179/255, 8/255, 0.16)
                            hoveredColor: root.isAdded(profileData) ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(234/255, 179/255, 8/255, 0.26)
                            pressedColor: root.isAdded(profileData) ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(234/255, 179/255, 8/255, 0.32)
                            textColor: "#FFFFFF"
                            clickedFunc: function() {
                                if (root.isAdded(profileData)) {
                                    root.openMyProfiles()
                                } else {
                                    root.addPreset(profileData)
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24 + SettingsController.safeAreaBottomMargin
                }
            }
        }
    }
}
