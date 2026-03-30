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
    property string statusMessage: ""
    property bool statusIsError: false
    property int editingId: -1
    property bool editingEnabled: true
    property string editingAction: "direct"
    property bool showEditor: false

    readonly property bool canManageProfiles: FBLinkController.canManageRoutingProfiles
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)
    readonly property var systemProfiles: root.profiles.filter(function(profile) { return profile.kind === "system" })
    readonly property var customProfiles: root.profiles.filter(function(profile) { return profile.kind !== "system" })

    function normalizeEntries(rawText) {
        return rawText.split(/[\n,;]+/).map(function(entry) { return entry.trim() }).filter(function(entry) { return entry.length > 0 })
    }

    function listToText(list) { return list && list.length ? list.join("\n") : "" }
    function actionLabel(action) { return action === "proxy" ? qsTr("ЧЕРЕЗ VPN") : qsTr("БЕЗ VPN") }
    function actionTone(action) { return action === "proxy" ? "proxy" : "direct" }
    function profileIcon(profile) { return "qrc:/images/controls/" + (profile.icon || (profile.action === "proxy" ? "scan-line.svg" : "tag.svg")) }
    function rulesSummary(profile) {
        return qsTr("Домены: %1 • Суффиксы: %2 • CIDR: %3")
            .arg(profile.domains ? profile.domains.length : 0)
            .arg(profile.domain_suffixes ? profile.domain_suffixes.length : 0)
            .arg(profile.cidrs ? profile.cidrs.length : 0)
    }

    function resetEditor() {
        editingId = -1
        editingEnabled = true
        editingAction = "direct"
        showEditor = false
        statusMessage = ""
        statusIsError = false
        nameField.textField.text = ""
        domainsArea.textArea.text = ""
        suffixesArea.textArea.text = ""
        cidrsArea.textArea.text = ""
    }

    function startNewProfile() {
        resetEditor()
        showEditor = true
    }

    function startEdit(profile) {
        editingId = Number(profile.id || -1)
        editingEnabled = !!profile.enabled
        editingAction = String(profile.action || "direct")
        showEditor = true
        nameField.textField.text = profile.name || ""
        domainsArea.textArea.text = listToText(profile.domains || [])
        suffixesArea.textArea.text = listToText(profile.domain_suffixes || [])
        cidrsArea.textArea.text = listToText(profile.cidrs || [])
    }

    function applyQuickTemplate(code) {
        startNewProfile()
        if (code === "ai_proxy") {
            nameField.textField.text = qsTr("AI через VPN")
            editingAction = "proxy"
            domainsArea.textArea.text = "chatgpt.com\nclaude.ai\nperplexity.ai\npoe.com\ngemini.google.com"
            suffixesArea.textArea.text = ".openai.com\n.anthropic.com\n.perplexity.ai\n.openrouter.ai"
        } else if (code === "media_proxy") {
            nameField.textField.text = qsTr("Медиа через VPN")
            editingAction = "proxy"
            domainsArea.textArea.text = "youtube.com\nyoutu.be\ntwitch.tv\ntiktok.com"
            suffixesArea.textArea.text = ".youtube.com\n.googlevideo.com\n.twitch.tv\n.tiktokcdn.com"
        } else if (code === "ru_direct") {
            nameField.textField.text = qsTr("RU без VPN")
            editingAction = "direct"
            suffixesArea.textArea.text = ".ru\n.xn--p1ai"
        } else if (code === "banks_gosuslugi_direct") {
            nameField.textField.text = qsTr("Банки и госуслуги")
            editingAction = "direct"
            domainsArea.textArea.text = "gosuslugi.ru\nesia.gosuslugi.ru\ngosuslugi.com"
            suffixesArea.textArea.text = ".gosuslugi.ru\n.nalog.gov.ru\n.sber.ru\n.vtb.ru\n.tbank.ru"
        } else if (code === "games_launchers_direct") {
            nameField.textField.text = qsTr("Игры и лаунчеры без VPN")
            editingAction = "direct"
            suffixesArea.textArea.text = ".vkplay.ru\n.4game.ru\n.lesta.ru\n.tanki.su\n.mail.ru"
        } else if (code === "local_media_direct") {
            nameField.textField.text = qsTr("Локальные медиа без VPN")
            editingAction = "direct"
            suffixesArea.textArea.text = ".kinopoisk.ru\n.rutube.ru\n.ivi.ru\n.okko.tv\n.wink.ru\n.smotrim.ru\n.premier.one"
        }
    }

    function submitProfile() {
        if (!canManageProfiles) {
            PageController.goToPage(PageEnum.PageFBLinkSubscription)
            return
        }
        const name = nameField.textField.text.trim()
        if (!name) {
            statusMessage = qsTr("Введите название профиля")
            statusIsError = true
            return
        }
        const payload = {
            name: name,
            action: editingAction,
            enabled: editingEnabled,
            domains: normalizeEntries(domainsArea.textArea.text),
            domain_suffixes: normalizeEntries(suffixesArea.textArea.text),
            cidrs: normalizeEntries(cidrsArea.textArea.text)
        }
        if (editingId > 0) payload.id = editingId
        FBLinkController.saveRoutingProfile(payload)
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
        function onRoutingProfilesFetched(profiles) { root.profiles = profiles }
        function onRoutingProfilesError(errorMessage) { root.statusMessage = errorMessage; root.statusIsError = true }
        function onRoutingProfileSaved() {
            root.statusMessage = qsTr("Профиль сохранён")
            root.statusIsError = false
            FBLinkController.fetchRoutingProfiles()
            root.resetEditor()
        }
        function onRoutingProfileDeleted() {
            root.statusMessage = qsTr("Профиль удалён")
            root.statusIsError = false
            FBLinkController.fetchRoutingProfiles()
            root.resetEditor()
        }
    }

    Component.onCompleted: {
        if (FBLinkController.isLoggedIn) FBLinkController.fetchRoutingProfiles()
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
                    accentColor: root.canManageProfiles ? "#00C8FF" : "#F59E0B"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        PremiumBadge { text: qsTr("VIP"); tone: root.canManageProfiles ? "accent" : "warning"; iconSource: "qrc:/images/controls/tag.svg" }
                        PremiumBadge { text: qsTr("Новые пакеты"); tone: "success"; iconSource: "qrc:/images/controls/shield-tick.svg" }
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
                            ? qsTr("Готовые пакеты и свои правила без длинной перегруженной формы.")
                            : qsTr("Редактирование доступно только в VIP.")
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
                    visible: root.canManageProfiles
                    accentVisible: true
                    accentColor: "#10B981"

                    LabelTextType {
                        text: qsTr("Быстрые пакеты")
                        font.pixelSize: 17
                        font.weight: 700
                        color: FBLinkStyle.color.paleGray
                    }

                    Flow {
                        Layout.fillWidth: true
                        width: parent ? parent.width : 0
                        spacing: 10

                        BasicButtonType { width: root.wideLayout ? 220 : parent.width; implicitHeight: 48; text: qsTr("AI через VPN"); defaultColor: Qt.rgba(0, 200/255, 255/255, 0.16); hoveredColor: Qt.rgba(0, 200/255, 255/255, 0.26); pressedColor: Qt.rgba(0, 200/255, 255/255, 0.32); textColor: "#FFFFFF"; clickedFunc: function() { root.applyQuickTemplate("ai_proxy") } }
                        BasicButtonType { width: root.wideLayout ? 220 : parent.width; implicitHeight: 48; text: qsTr("Медиа через VPN"); defaultColor: Qt.rgba(0, 200/255, 255/255, 0.16); hoveredColor: Qt.rgba(0, 200/255, 255/255, 0.26); pressedColor: Qt.rgba(0, 200/255, 255/255, 0.32); textColor: "#FFFFFF"; clickedFunc: function() { root.applyQuickTemplate("media_proxy") } }
                        BasicButtonType { width: root.wideLayout ? 220 : parent.width; implicitHeight: 48; text: qsTr("RU без VPN"); defaultColor: Qt.rgba(16/255, 185/255, 129/255, 0.18); hoveredColor: Qt.rgba(16/255, 185/255, 129/255, 0.28); pressedColor: Qt.rgba(16/255, 185/255, 129/255, 0.34); textColor: "#FFFFFF"; clickedFunc: function() { root.applyQuickTemplate("ru_direct") } }
                        BasicButtonType { width: root.wideLayout ? 220 : parent.width; implicitHeight: 48; text: qsTr("Банки и госуслуги"); defaultColor: Qt.rgba(16/255, 185/255, 129/255, 0.18); hoveredColor: Qt.rgba(16/255, 185/255, 129/255, 0.28); pressedColor: Qt.rgba(16/255, 185/255, 129/255, 0.34); textColor: "#FFFFFF"; clickedFunc: function() { root.applyQuickTemplate("banks_gosuslugi_direct") } }
                        BasicButtonType { width: root.wideLayout ? 220 : parent.width; implicitHeight: 48; text: qsTr("Игры без VPN"); defaultColor: Qt.rgba(16/255, 185/255, 129/255, 0.18); hoveredColor: Qt.rgba(16/255, 185/255, 129/255, 0.28); pressedColor: Qt.rgba(16/255, 185/255, 129/255, 0.34); textColor: "#FFFFFF"; clickedFunc: function() { root.applyQuickTemplate("games_launchers_direct") } }
                        BasicButtonType { width: root.wideLayout ? 220 : parent.width; implicitHeight: 48; text: qsTr("Локальные медиа"); defaultColor: Qt.rgba(16/255, 185/255, 129/255, 0.18); hoveredColor: Qt.rgba(16/255, 185/255, 129/255, 0.28); pressedColor: Qt.rgba(16/255, 185/255, 129/255, 0.34); textColor: "#FFFFFF"; clickedFunc: function() { root.applyQuickTemplate("local_media_direct") } }
                    }
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 12
                    accentVisible: true
                    accentColor: "#10B981"

                    RowLayout {
                        Layout.fillWidth: true
                        LabelTextType { Layout.fillWidth: true; text: qsTr("Системные пресеты"); font.pixelSize: 17; font.weight: 700; color: FBLinkStyle.color.paleGray }
                        PremiumBadge { text: qsTr("%1 шт.").arg(root.systemProfiles.length); tone: "success" }
                    }

                    LabelTextType {
                        visible: root.systemProfiles.length === 0
                        text: qsTr("Системные пресеты пока не загружены")
                        font.pixelSize: 13
                        color: FBLinkStyle.color.mutedGray
                    }

                    Repeater {
                        model: root.systemProfiles
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

                            CaptionTextType { Layout.fillWidth: true; text: root.rulesSummary(profileData); wrapMode: Text.WordWrap; color: FBLinkStyle.color.mutedGray }

                            BasicButtonType {
                                Layout.fillWidth: true
                                implicitHeight: 44
                                enabled: root.canManageProfiles
                                text: profileData.enabled ? qsTr("Выключить") : qsTr("Включить")
                                defaultColor: profileData.enabled ? Qt.rgba(16/255, 185/255, 129/255, 0.18) : Qt.rgba(0, 200/255, 255/255, 0.16)
                                hoveredColor: profileData.enabled ? Qt.rgba(16/255, 185/255, 129/255, 0.28) : Qt.rgba(0, 200/255, 255/255, 0.26)
                                pressedColor: profileData.enabled ? Qt.rgba(16/255, 185/255, 129/255, 0.34) : Qt.rgba(0, 200/255, 255/255, 0.32)
                                textColor: "#FFFFFF"
                                clickedFunc: function() { root.toggleProfile(profileData) }
                            }
                        }
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
                            clickedFunc: root.startNewProfile
                        }
                    }

                    LabelTextType { visible: root.customProfiles.length === 0; text: qsTr("Пока нет пользовательских профилей"); font.pixelSize: 13; color: FBLinkStyle.color.mutedGray }

                    Repeater {
                        model: root.customProfiles
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
                                BasicButtonType { width: root.wideLayout ? 150 : parent.width; implicitHeight: 44; text: qsTr("Редактировать"); defaultColor: Qt.rgba(1, 1, 1, 0.08); hoveredColor: Qt.rgba(1, 1, 1, 0.12); pressedColor: Qt.rgba(1, 1, 1, 0.18); textColor: FBLinkStyle.color.paleGray; clickedFunc: function() { root.startEdit(profileData) } }
                                BasicButtonType { width: root.wideLayout ? 150 : parent.width; implicitHeight: 44; enabled: root.canManageProfiles; text: qsTr("Удалить"); defaultColor: Qt.rgba(239/255, 68/255, 68/255, 0.14); hoveredColor: Qt.rgba(239/255, 68/255, 68/255, 0.22); pressedColor: Qt.rgba(239/255, 68/255, 68/255, 0.30); textColor: "#FFFFFF"; clickedFunc: function() { root.deleteProfile(profileData) } }
                            }
                        }
                    }
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    visible: root.showEditor
                    padding: 12
                    accentVisible: true
                    accentColor: "#00C8FF"

                    LabelTextType { Layout.fillWidth: true; text: root.editingId > 0 ? qsTr("Редактирование профиля") : qsTr("Новый профиль"); font.pixelSize: 18; font.weight: 700; color: FBLinkStyle.color.paleGray; wrapMode: Text.WordWrap }

                    WarningType {
                        Layout.fillWidth: true
                        visible: root.statusMessage !== ""
                        textString: root.statusMessage
                        iconPath: root.statusIsError ? "qrc:/images/controls/alert-circle.svg" : "qrc:/images/controls/check.svg"
                        backGroundColor: root.statusIsError ? Qt.rgba(239/255, 68/255, 68/255, 0.12) : Qt.rgba(16/255, 185/255, 129/255, 0.12)
                        imageColor: root.statusIsError ? "#EF4444" : "#10B981"
                        textColor: root.statusIsError ? "#FFB4B4" : "#B6F2D2"
                    }

                    TextFieldWithHeaderType { id: nameField; Layout.fillWidth: true; headerText: qsTr("Название"); enabled: root.canManageProfiles; textField.placeholderText: qsTr("Например, AI через VPN") }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        BasicButtonType { Layout.fillWidth: true; implicitHeight: 48; enabled: root.canManageProfiles; text: qsTr("Без VPN"); defaultColor: root.editingAction === "direct" ? Qt.rgba(16/255, 185/255, 129/255, 0.18) : Qt.rgba(1, 1, 1, 0.08); hoveredColor: root.editingAction === "direct" ? Qt.rgba(16/255, 185/255, 129/255, 0.28) : Qt.rgba(1, 1, 1, 0.12); pressedColor: root.editingAction === "direct" ? Qt.rgba(16/255, 185/255, 129/255, 0.34) : Qt.rgba(1, 1, 1, 0.18); textColor: "#FFFFFF"; clickedFunc: function() { root.editingAction = "direct" } }
                        BasicButtonType { Layout.fillWidth: true; implicitHeight: 48; enabled: root.canManageProfiles; text: qsTr("Через VPN"); defaultColor: root.editingAction === "proxy" ? Qt.rgba(0, 200/255, 255/255, 0.16) : Qt.rgba(1, 1, 1, 0.08); hoveredColor: root.editingAction === "proxy" ? Qt.rgba(0, 200/255, 255/255, 0.26) : Qt.rgba(1, 1, 1, 0.12); pressedColor: root.editingAction === "proxy" ? Qt.rgba(0, 200/255, 255/255, 0.32) : Qt.rgba(1, 1, 1, 0.18); textColor: "#FFFFFF"; clickedFunc: function() { root.editingAction = "proxy" } }
                    }
                    TextAreaType { id: domainsArea; Layout.fillWidth: true; Layout.preferredHeight: 84; enabled: root.canManageProfiles; placeholderText: qsTr("Домены, по одному на строку") }
                    TextAreaType { id: suffixesArea; Layout.fillWidth: true; Layout.preferredHeight: 84; enabled: root.canManageProfiles; placeholderText: qsTr("Суффиксы доменов, по одному на строку") }
                    TextAreaType { id: cidrsArea; Layout.fillWidth: true; Layout.preferredHeight: 84; enabled: root.canManageProfiles; placeholderText: qsTr("CIDR-подсети, по одной на строку") }

                    Flow {
                        Layout.fillWidth: true
                        width: parent ? parent.width : 0
                        spacing: 10
                        BasicButtonType { width: root.wideLayout ? 220 : parent.width; implicitHeight: 44; enabled: root.canManageProfiles; text: root.editingId > 0 ? qsTr("Сохранить") : qsTr("Создать профиль"); defaultColor: "#00C8FF"; hoveredColor: "#33D4FF"; pressedColor: "#0099BB"; textColor: "#FFFFFF"; clickedFunc: root.submitProfile }
                        BasicButtonType { width: root.wideLayout ? 220 : parent.width; implicitHeight: 44; text: qsTr("Отмена"); defaultColor: Qt.rgba(1, 1, 1, 0.08); hoveredColor: Qt.rgba(1, 1, 1, 0.12); pressedColor: Qt.rgba(1, 1, 1, 0.18); textColor: FBLinkStyle.color.paleGray; clickedFunc: root.resetEditor }
                    }
                }

                Item { Layout.fillWidth: true; Layout.preferredHeight: 24 + SettingsController.safeAreaBottomMargin }
            }
        }
    }
}
