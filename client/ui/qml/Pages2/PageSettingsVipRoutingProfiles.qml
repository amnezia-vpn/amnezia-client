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
    property bool profilesLoading: false
    property bool restoreScrollAfterRefresh: false
    property real pendingContentY: 0

    readonly property bool canManageProfiles: FBLinkController.canManageRoutingProfiles
    readonly property bool canUseAdBlock: FBLinkController.canUseAdBlock
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)
    readonly property var customProfiles: root.profiles.filter(function(profile) { return profile.kind !== "system" })

    function actionLabel(action) { return action === "proxy" ? qsTr("ЧЕРЕЗ VPN") : qsTr("БЕЗ VPN") }
    function actionTone(action) { return action === "proxy" ? "proxy" : "direct" }
    function adBlockStatusText() { return FBLinkController.vipAdBlockStatusLabel }
    function adBlockReasonText() {
        const reason = String(FBLinkController.vipAdBlockDegradeReason || "")
        if (reason === "auth_expired") return qsTr("Проверьте вход в аккаунт и обновите данные.")
        if (reason === "sync_stale") return qsTr("Профиль защиты обновляется. Повторите чуть позже.")
        if (reason === "dns_unreachable") return qsTr("Сервис фильтрации временно недоступен.")
        if (reason === "routing_rules_missing") return qsTr("Профили маршрутизации ещё не загрузились.")
        return qsTr("")
    }

    function openCreateProfileEditor() {
        if (!canManageProfiles) {
            PageController.goToPage(PageEnum.PageFBLinkSubscription)
            return
        }
        GC.vipRoutingProfileEditorDraft = {
            mode: "create",
            id: -1,
            enabled: true,
            action: "direct",
            name: "",
            domains: [],
            domain_suffixes: [],
            cidrs: []
        }
        PageController.goToPage(PageEnum.PageSettingsVipRoutingProfileEditor)
    }

    function openEditProfileEditor(profile) {
        if (!canManageProfiles) {
            PageController.goToPage(PageEnum.PageFBLinkSubscription)
            return
        }
        GC.vipRoutingProfileEditorDraft = {
            mode: "edit",
            id: Number(profile.id || -1),
            enabled: !!profile.enabled,
            action: String(profile.action || "direct"),
            name: String(profile.name || ""),
            domains: profile.domains || [],
            domain_suffixes: profile.domain_suffixes || [],
            cidrs: profile.cidrs || []
        }
        PageController.goToPage(PageEnum.PageSettingsVipRoutingProfileEditor)
    }

    function syncSharedStatus() {
        if (GC.vipRoutingProfilesStatusMessage && GC.vipRoutingProfilesStatusMessage.length > 0) {
            const message = GC.vipRoutingProfilesStatusMessage
            if (GC.vipRoutingProfilesStatusIsError) {
                PageController.showErrorMessage(message)
            } else {
                PageController.showNotificationMessage(message)
            }
            GC.vipRoutingProfilesStatusMessage = ""
            GC.vipRoutingProfilesStatusIsError = false
        }
    }

    function preserveScrollPosition() {
        pendingContentY = profilesFlick.contentY
        restoreScrollAfterRefresh = true
    }

    function toggleProfile(profile) {
        const payload = {
            id: Number(profile.id || 0),
            enabled: !profile.enabled
        }

        if (profile.kind !== "system") {
            payload.name = profile.name
            payload.action = profile.action || "direct"
            payload.domains = profile.domains || []
            payload.domain_suffixes = profile.domain_suffixes || []
            payload.cidrs = profile.cidrs || []
        }

        FBLinkController.saveRoutingProfile(payload)
    }

    function deleteProfile(profile) {
        FBLinkController.deleteRoutingProfile(Number(profile.id))
    }

    Connections {
        target: FBLinkController
        function onRoutingProfilesFetched(profiles) {
            root.profiles = profiles
            root.profilesLoading = false

            if (root.restoreScrollAfterRefresh) {
                const targetY = root.pendingContentY
                root.restoreScrollAfterRefresh = false
                Qt.callLater(function() {
                    const maxY = Math.max(0, profilesFlick.contentHeight - profilesFlick.height)
                    profilesFlick.contentY = Math.min(targetY, maxY)
                })
            }
        }
        function onRoutingProfilesError(errorMessage) {
            PageController.showErrorMessage(errorMessage)
            root.profilesLoading = false
            root.restoreScrollAfterRefresh = false
        }
        function onVipAdBlockChanged(enabled) {
            const message = enabled ? qsTr("Ad Block для VIP включён") : qsTr("Ad Block для VIP выключен")
            PageController.showNotificationMessage(message)
        }
        function onRequestError(errorMessage) {
            PageController.showErrorMessage(errorMessage)
        }
        function onBugReportSubmitted(ticketId) {
            const message = qsTr("Отчёт отправлен. Номер: %1").arg(ticketId)
            PageController.showNotificationMessage(message)
        }
        function onRoutingProfileSaved() {
            const message = qsTr("Профиль сохранён")
            PageController.showNotificationMessage(message)
            root.preserveScrollPosition()
            root.profilesLoading = true
            FBLinkController.fetchRoutingProfiles()
        }
        function onRoutingProfileDeleted() {
            const message = qsTr("Профиль удалён")
            PageController.showNotificationMessage(message)
            root.preserveScrollPosition()
            root.profilesLoading = true
            FBLinkController.fetchRoutingProfiles()
        }
        function onRoutingSystemProfileCopied(profile, created) {
            const message = created
                    ? qsTr("Пресет добавлен в мои профили")
                    : qsTr("Пресет уже был добавлен ранее")
            PageController.showNotificationMessage(message)
            root.preserveScrollPosition()
            root.profilesLoading = true
            FBLinkController.fetchRoutingProfiles()
        }
    }

    Component.onCompleted: {
        root.syncSharedStatus()
        if (FBLinkController.isLoggedIn) {
            root.profilesLoading = true
            FBLinkController.fetchRoutingProfiles()
        }
    }

    onVisibleChanged: {
        if (!visible) {
            return
        }
        root.syncSharedStatus()
        if (FBLinkController.isLoggedIn) {
            root.profilesLoading = true
            FBLinkController.fetchRoutingProfiles()
        }
    }

    Flickable {
        id: profilesFlick
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
                    accentColor: root.canManageProfiles ? "#00C8FF" : "#F59E0B"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        PremiumBadge { text: qsTr("VIP"); tone: root.canManageProfiles ? "accent" : "warning"; iconSource: "qrc:/images/controls/tag.svg" }
                        PremiumBadge { text: qsTr("Для VIP"); tone: "success"; iconSource: "qrc:/images/controls/shield-tick.svg" }
                    }

                    LabelTextType {
                        Layout.fillWidth: true
                        text: qsTr("VIP-пресеты и маршрутизация")
                        font.pixelSize: root.wideLayout ? 24 : 21
                        font.weight: 700
                        color: FBLinkStyle.color.paleGray
                        wrapMode: Text.WordWrap
                    }

                    LabelTextType {
                        Layout.fillWidth: true
                        text: root.canManageProfiles
                            ? qsTr("Выберите готовый пресет или создайте свой профиль маршрутизации.")
                            : qsTr("Маршрутизация сайтов доступна в VIP.")
                        font.pixelSize: 13
                        color: FBLinkStyle.color.mutedGray
                        wrapMode: Text.WordWrap
                    }

                    BasicButtonType {
                        Layout.fillWidth: true
                        implicitHeight: 48
                        visible: !root.canManageProfiles
                        text: qsTr("Открыть VIP-подписку")
                        defaultColor: "#00C8FF"
                        hoveredColor: "#33D4FF"
                        pressedColor: "#0099BB"
                        textColor: "#FFFFFF"
                        clickedFunc: function() { PageController.goToPage(PageEnum.PageFBLinkSubscription) }
                    }
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 12
                    visible: root.canManageProfiles && root.canUseAdBlock
                    accentVisible: true
                    accentColor: FBLinkController.vipAdBlockEnabled ? "#10B981" : "#00C8FF"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                PremiumBadge { text: qsTr("Защита"); tone: FBLinkController.vipAdBlockEnabled ? "success" : "accent"; iconSource: "qrc:/images/controls/shield-tick.svg" }
                                PremiumBadge {
                                    text: root.adBlockStatusText().toUpperCase()
                                    tone: FBLinkController.vipAdBlockStatus === "applied"
                                        ? "success"
                                        : (FBLinkController.vipAdBlockEnabled ? "warning" : "neutral")
                                }
                            }

                            LabelTextType {
                                Layout.fillWidth: true
                                text: qsTr("Ad Block только для VIP")
                                font.pixelSize: 17
                                font.weight: 700
                                color: FBLinkStyle.color.paleGray
                                wrapMode: Text.WordWrap
                            }

                            CaptionTextType {
                                Layout.fillWidth: true
                                text: FBLinkController.vipAdBlockEnabled
                                    ? (FBLinkController.vipAdBlockStatus === "applied"
                                        ? qsTr("Реклама и трекеры блокируются автоматически.")
                                        : qsTr("Фильтрация временно недоступна, но VPN продолжает работать."))
                                    : qsTr("Ad Block выключен. Сайты открываются без фильтрации рекламы.")
                                color: FBLinkStyle.color.mutedGray
                                wrapMode: Text.WordWrap
                            }

                            CaptionTextType {
                                Layout.fillWidth: true
                                visible: FBLinkController.vipAdBlockEnabled && FBLinkController.vipAdBlockStatus === "degraded"
                                text: root.adBlockReasonText()
                                color: "#F59E0B"
                                wrapMode: Text.WordWrap
                            }

                            BasicButtonType {
                                Layout.fillWidth: true
                                visible: FBLinkController.vipAdBlockEnabled && FBLinkController.vipAdBlockStatus === "degraded"
                                implicitHeight: 38
                                text: qsTr("Отправить отчёт")
                                defaultColor: Qt.rgba(245/255, 158/255, 11/255, 0.18)
                                hoveredColor: Qt.rgba(245/255, 158/255, 11/255, 0.28)
                                pressedColor: Qt.rgba(245/255, 158/255, 11/255, 0.36)
                                textColor: "#FFFFFF"
                                clickedFunc: function() {
                                    FBLinkController.submitBugReport(qsTr("Проблема с VIP Ad Block"))
                                }
                            }
                        }

                        SwitcherType {
                            Layout.alignment: Qt.AlignTop
                            enabled: root.canManageProfiles && !FBLinkController.isLoading
                            checked: FBLinkController.vipAdBlockEnabled
                            onToggled: {
                                if (checked !== FBLinkController.vipAdBlockEnabled) {
                                    FBLinkController.setVipAdBlockEnabled(checked)
                                }
                            }
                        }
                    }
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 12
                    visible: root.canManageProfiles
                    accentVisible: true
                    accentColor: "#10B981"

                    RowLayout {
                        Layout.fillWidth: true
                        LabelTextType {
                            Layout.fillWidth: true
                            text: qsTr("Системные пресеты")
                            font.pixelSize: 17
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
                        }
                        BasicButtonType {
                            implicitHeight: 38
                            implicitWidth: 170
                            text: qsTr("Добавить пресет")
                            defaultColor: Qt.rgba(16/255, 185/255, 129/255, 0.18)
                            hoveredColor: Qt.rgba(16/255, 185/255, 129/255, 0.28)
                            pressedColor: Qt.rgba(16/255, 185/255, 129/255, 0.34)
                            textColor: "#FFFFFF"
                            clickedFunc: function() { PageController.goToPage(PageEnum.PageSettingsVipPresetCatalog) }
                        }
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        text: qsTr("Откройте каталог и добавьте нужный пресет в «Мои профили».")
                        color: FBLinkStyle.color.mutedGray
                        wrapMode: Text.WordWrap
                    }
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 12
                    accentVisible: true
                    accentColor: "#00C8FF"

                    RowLayout {
                        Layout.fillWidth: true
                        LabelTextType { Layout.fillWidth: true; text: qsTr("Мои профили"); font.pixelSize: 17; font.weight: 700; color: FBLinkStyle.color.paleGray }
                        BasicButtonType {
                            visible: root.canManageProfiles
                            implicitHeight: 38
                            implicitWidth: 150
                            text: qsTr("Новый профиль")
                            defaultColor: Qt.rgba(0, 200/255, 255/255, 0.16)
                            hoveredColor: Qt.rgba(0, 200/255, 255/255, 0.26)
                            pressedColor: Qt.rgba(0, 200/255, 255/255, 0.32)
                            textColor: "#FFFFFF"
                            clickedFunc: root.openCreateProfileEditor
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: root.profilesLoading
                        spacing: 10
                        BusyIndicator {
                            Layout.preferredWidth: 22
                            Layout.preferredHeight: 22
                            running: root.profilesLoading
                        }
                        LabelTextType {
                            text: qsTr("Загружаем профили...")
                            font.pixelSize: 13
                            color: FBLinkStyle.color.mutedGray
                        }
                    }

                    LabelTextType {
                        visible: !root.profilesLoading && root.customProfiles.length === 0
                        text: qsTr("Пока нет пользовательских профилей")
                        font.pixelSize: 13
                        color: FBLinkStyle.color.mutedGray
                    }

                    BasicButtonType {
                        visible: !root.profilesLoading && root.customProfiles.length === 0 && root.canManageProfiles
                        Layout.fillWidth: true
                        implicitHeight: 42
                        text: qsTr("Открыть каталог пресетов")
                        defaultColor: Qt.rgba(1, 1, 1, 0.08)
                        hoveredColor: Qt.rgba(1, 1, 1, 0.12)
                        pressedColor: Qt.rgba(1, 1, 1, 0.18)
                        textColor: FBLinkStyle.color.paleGray
                        clickedFunc: function() { PageController.goToPage(PageEnum.PageSettingsVipPresetCatalog) }
                    }

                    Repeater {
                        model: root.profilesLoading ? [] : root.customProfiles
                        delegate: PremiumPanel {
                            Layout.fillWidth: true
                            padding: 12
                            fillColor: Qt.rgba(1, 1, 1, 0.03)
                            outlineColor: Qt.rgba(1, 1, 1, 0.06)
                            property var profileData: modelData

                            LabelTextType { Layout.fillWidth: true; text: profileData.name || qsTr("Без названия"); font.pixelSize: 16; font.weight: 700; color: FBLinkStyle.color.paleGray; wrapMode: Text.WordWrap }

                            Flow {
                                Layout.fillWidth: true
                                width: parent ? parent.width : 0
                                spacing: 8
                                PremiumBadge { text: root.actionLabel(profileData.action || "direct"); tone: root.actionTone(profileData.action || "direct") }
                                PremiumBadge { text: profileData.enabled ? qsTr("ВКЛЮЧЕН") : qsTr("ВЫКЛЮЧЕН"); tone: profileData.enabled ? "success" : "neutral" }
                            }

                                Flow {
                                Layout.fillWidth: true
                                width: parent ? parent.width : 0
                                spacing: 10
                                BasicButtonType { width: root.wideLayout ? 150 : parent.width; implicitHeight: 44; enabled: root.canManageProfiles; text: profileData.enabled ? qsTr("Выключить") : qsTr("Включить"); defaultColor: profileData.enabled ? Qt.rgba(16/255, 185/255, 129/255, 0.18) : Qt.rgba(0, 200/255, 255/255, 0.16); hoveredColor: profileData.enabled ? Qt.rgba(16/255, 185/255, 129/255, 0.28) : Qt.rgba(0, 200/255, 255/255, 0.26); pressedColor: profileData.enabled ? Qt.rgba(16/255, 185/255, 129/255, 0.34) : Qt.rgba(0, 200/255, 255/255, 0.32); textColor: "#FFFFFF"; clickedFunc: function() { root.toggleProfile(profileData) } }
                                BasicButtonType { width: root.wideLayout ? 150 : parent.width; implicitHeight: 44; text: qsTr("Редактировать"); defaultColor: Qt.rgba(1, 1, 1, 0.08); hoveredColor: Qt.rgba(1, 1, 1, 0.12); pressedColor: Qt.rgba(1, 1, 1, 0.18); textColor: FBLinkStyle.color.paleGray; clickedFunc: function() { root.openEditProfileEditor(profileData) } }
                                BasicButtonType { width: root.wideLayout ? 150 : parent.width; implicitHeight: 44; enabled: root.canManageProfiles; text: qsTr("Удалить"); defaultColor: Qt.rgba(239/255, 68/255, 68/255, 0.14); hoveredColor: Qt.rgba(239/255, 68/255, 68/255, 0.22); pressedColor: Qt.rgba(239/255, 68/255, 68/255, 0.30); textColor: "#FFFFFF"; clickedFunc: function() { root.deleteProfile(profileData) } }
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true; Layout.preferredHeight: 24 + SettingsController.safeAreaBottomMargin }
            }
        }
    }
}
