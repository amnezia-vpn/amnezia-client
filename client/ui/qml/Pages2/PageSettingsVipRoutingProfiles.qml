import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

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
    readonly property bool isRoutingLocked: ConnectionController.isConnected || ConnectionController.isConnectionInProgress
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)
    readonly property var systemProfiles: root.profiles.filter(function(profile) { return profile.kind === "system" })
    readonly property var customProfiles: root.profiles.filter(function(profile) { return profile.kind !== "system" })
    readonly property int enabledProfilesCount: root.customProfiles.filter(function(profile) { return profile.enabled }).length

    function actionLabel(action) { return action === "proxy" ? qsTr("ЧЕРЕЗ VPN") : qsTr("БЕЗ VPN") }
    function actionTone(action) { return action === "proxy" ? "proxy" : "direct" }
    function showRoutingLockedNotification() {
        PageController.showNotificationMessage(qsTr("Нельзя менять профили маршрутизации во время активного подключения"))
    }
    function adBlockStatusText() { return FBLinkController.vipAdBlockStatusLabel }
    function adBlockReasonText() {
        const reason = String(FBLinkController.vipAdBlockDegradeReason || "")
        if (reason === "auth_expired") return qsTr("Проверьте вход в аккаунт и обновите данные.")
        if (reason === "sync_stale") return qsTr("Профиль защиты обновляется. Повторите чуть позже.")
        if (reason === "dns_unreachable") return qsTr("Сервис фильтрации временно недоступен.")
        if (reason === "routing_rules_missing") return qsTr("Профили маршрутизации ещё не загрузились.")
        return qsTr("")
    }
    function profileRulesSummary(profile) {
        const domainsCount = (profile.domains || []).length
        const suffixesCount = (profile.domain_suffixes || []).length
        const cidrsCount = (profile.cidrs || []).length
        const total = domainsCount + suffixesCount + cidrsCount
        return total > 0 ? qsTr("Правил: %1").arg(total) : qsTr("Правил нет")
    }

    function openCreateProfileEditor() {
        if (root.isRoutingLocked) {
            root.showRoutingLockedNotification()
            return
        }
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
        if (root.isRoutingLocked) {
            root.showRoutingLockedNotification()
            return
        }
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
        if (root.isRoutingLocked) {
            root.showRoutingLockedNotification()
            return
        }
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
        if (root.isRoutingLocked) {
            root.showRoutingLockedNotification()
            return
        }
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

                LabelTextType {
                    Layout.fillWidth: true
                    text: qsTr("Профили маршрутизации")
                    font.pixelSize: root.wideLayout ? 34 : 30
                    font.weight: 700
                    color: FBLinkStyle.color.paleGray
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 12
                    visible: true
                    radius: 16
                    fillColor: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    accentVisible: false

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        PremiumBadge {
                            text: root.canManageProfiles ? qsTr("VIP АКТИВЕН") : qsTr("VIP ТРЕБУЕТСЯ")
                            tone: root.canManageProfiles ? "success" : "warning"
                            compact: true
                        }
                        PremiumBadge {
                            text: qsTr("МАРШРУТИЗАЦИЯ")
                            tone: "neutral"
                            compact: true
                        }
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
                        defaultColor: "#EAB308"
                        hoveredColor: "#FACC15"
                        pressedColor: "#CA8A04"
                        textColor: "#FFFFFF"
                        clickedFunc: function() { PageController.goToPage(PageEnum.PageFBLinkSubscription) }
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    visible: root.canManageProfiles
                    columns: 1
                    columnSpacing: 10
                    rowSpacing: 10

                    PremiumPanel {
                        Layout.fillWidth: true
                        radius: 16
                        padding: 14
                        fillColor: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                        outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                        accentVisible: false
                        clickable: true
                        onClicked: PageController.goToPage(PageEnum.PageSettingsVipPresetCatalog)

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            LabelTextType {
                                Layout.fillWidth: true
                                text: qsTr("Системные пресеты")
                                font.pixelSize: 17
                                font.weight: 700
                                color: FBLinkStyle.color.paleGray
                            }
                            Image {
                                source: "qrc:/images/controls/chevron-right.svg"
                                sourceSize: Qt.size(18, 18)
                                Layout.alignment: Qt.AlignVCenter
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: FBLinkStyle.color.mutedGray }
                            }
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            text: qsTr("Откройте каталог и добавьте нужный пресет в «Мои профили».")
                            color: FBLinkStyle.color.mutedGray
                            wrapMode: Text.WordWrap
                        }

                    }
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    radius: 16
                    padding: 14
                    fillColor: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    accentVisible: false

                    RowLayout {
                        Layout.fillWidth: true
                        LabelTextType {
                            Layout.fillWidth: true
                            text: qsTr("Мои профили")
                            font.pixelSize: 17
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
                        }
                        PremiumBadge {
                            text: qsTr("%1 активных").arg(root.enabledProfilesCount)
                            tone: root.enabledProfilesCount > 0 ? "success" : "neutral"
                            compact: true
                        }
                        Item {
                            visible: root.canManageProfiles
                            Layout.preferredHeight: 36
                            Layout.preferredWidth: 36
                            enabled: !root.isRoutingLocked
                            opacity: enabled ? 1.0 : 0.45

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/images/controls/plus.svg"
                                sourceSize: Qt.size(22, 22)
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: "#FFFFFF" }
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: !root.isRoutingLocked
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.openCreateProfileEditor()
                            }
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

                    WarningType {
                        Layout.fillWidth: true
                        visible: root.canManageProfiles && root.isRoutingLocked
                        textString: qsTr("Редактирование профилей временно заблокировано до отключения VPN.")
                        iconPath: "qrc:/images/controls/alert-circle.svg"
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
                            property var profileData: modelData
                            fillColor: Qt.rgba(14/255, 14/255, 14/255, 1.0)
                            outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                            accentVisible: false

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                LabelTextType {
                                    Layout.fillWidth: true
                                    text: profileData.name || qsTr("Без названия")
                                    font.pixelSize: 16
                                    font.weight: 700
                                    color: FBLinkStyle.color.paleGray
                                    wrapMode: Text.WordWrap
                                }

                                SwitcherType {
                                    text: ""
                                    visible: root.canManageProfiles
                                    enabled: root.canManageProfiles && !FBLinkController.isLoading && !root.isRoutingLocked
                                    checked: !!profileData.enabled
                                    onToggled: {
                                        if (checked !== !!profileData.enabled) {
                                            root.toggleProfile(profileData)
                                        }
                                    }
                                }

                                RowLayout {
                                    visible: root.canManageProfiles
                                    spacing: 8

                                    Rectangle {
                                        Layout.preferredWidth: 34
                                        Layout.preferredHeight: 34
                                        radius: 9
                                        color: editMouse.pressed
                                            ? Qt.rgba(255/255, 255/255, 255/255, 0.14)
                                            : (editMouse.containsMouse ? Qt.rgba(255/255, 255/255, 255/255, 0.10) : Qt.rgba(255/255, 255/255, 255/255, 0.06))
                                        border.width: 1
                                        border.color: Qt.rgba(255/255, 255/255, 255/255, 0.12)
                                        enabled: !root.isRoutingLocked
                                        opacity: enabled ? 1.0 : 0.45

                                        Image {
                                            anchors.centerIn: parent
                                            source: "qrc:/images/controls/edit-3.svg"
                                            sourceSize: Qt.size(16, 16)
                                            layer.enabled: true
                                            layer.effect: ColorOverlay { color: "#E5E7EB" }
                                        }

                                        MouseArea {
                                            id: editMouse
                                            anchors.fill: parent
                                            enabled: !root.isRoutingLocked
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.openEditProfileEditor(profileData)
                                        }
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 34
                                        Layout.preferredHeight: 34
                                        radius: 9
                                        color: deleteMouse.pressed
                                            ? Qt.rgba(239/255, 68/255, 68/255, 0.28)
                                            : (deleteMouse.containsMouse ? Qt.rgba(239/255, 68/255, 68/255, 0.22) : Qt.rgba(239/255, 68/255, 68/255, 0.14))
                                        border.width: 1
                                        border.color: Qt.rgba(239/255, 68/255, 68/255, 0.45)
                                        enabled: !root.isRoutingLocked
                                        opacity: enabled ? 1.0 : 0.45

                                        Image {
                                            anchors.centerIn: parent
                                            source: "qrc:/images/controls/trash.svg"
                                            sourceSize: Qt.size(16, 16)
                                            layer.enabled: true
                                            layer.effect: ColorOverlay { color: "#F87171" }
                                        }

                                        MouseArea {
                                            id: deleteMouse
                                            anchors.fill: parent
                                            enabled: !root.isRoutingLocked
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.deleteProfile(profileData)
                                        }
                                    }
                                }
                            }

                            Flow {
                                Layout.fillWidth: true
                                width: parent ? parent.width : 0
                                spacing: 8
                                PremiumBadge { text: root.actionLabel(profileData.action || "direct"); tone: root.actionTone(profileData.action || "direct") }
                                PremiumBadge { text: profileData.enabled ? qsTr("ВКЛЮЧЕН") : qsTr("ВЫКЛЮЧЕН"); tone: profileData.enabled ? "success" : "neutral" }
                                PremiumBadge { text: root.profileRulesSummary(profileData); tone: "neutral" }
                            }

                            CaptionTextType {
                                Layout.fillWidth: true
                                text: profileData.action === "proxy"
                                    ? qsTr("Трафик правил направляется через VPN.")
                                    : qsTr("Трафик правил идёт в обход VPN.")
                                color: FBLinkStyle.color.mutedGray
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true; Layout.preferredHeight: 24 + SettingsController.safeAreaBottomMargin }
            }
        }
    }
}
