import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    property var profiles: []
    property string statusMessage: ""
    property bool statusIsError: false
    property int editingId: -1
    property bool editingEnabled: true
    property string editingKind: "custom"

    readonly property bool canManageProfiles: FBLinkController.canManageRoutingProfiles
    readonly property bool compactLayout: width < 720

    function normalizeEntries(rawText) {
        return rawText
            .split(/[\n,;]+/)
            .map(function(entry) { return entry.trim() })
            .filter(function(entry) { return entry.length > 0 })
    }

    function listToText(list) {
        if (!list || !list.length) return ""
        return list.join("\n")
    }

    function rulesSummary(profile) {
        const domains = profile.domains ? profile.domains.length : 0
        const suffixes = profile.domain_suffixes ? profile.domain_suffixes.length : 0
        const cidrs = profile.cidrs ? profile.cidrs.length : 0
        return qsTr("Домены: %1 • Суффиксы: %2 • CIDR: %3").arg(domains).arg(suffixes).arg(cidrs)
    }

    function openSubscription() {
        PageController.goToPage(PageEnum.PageFBLinkSubscription)
    }

    function resetEditor() {
        root.editingId = -1
        root.editingKind = "custom"
        root.editingEnabled = true
        nameField.textField.text = ""
        domainsArea.textArea.text = ""
        suffixesArea.textArea.text = ""
        cidrsArea.textArea.text = ""
        importArea.textArea.text = ""
    }

    function startEdit(profile) {
        root.editingId = Number(profile.id || -1)
        root.editingKind = String(profile.kind || "custom")
        root.editingEnabled = !!profile.enabled
        nameField.textField.text = profile.name || ""
        domainsArea.textArea.text = root.listToText(profile.domains || [])
        suffixesArea.textArea.text = root.listToText(profile.domain_suffixes || [])
        cidrsArea.textArea.text = root.listToText(profile.cidrs || [])
        importArea.textArea.text = ""
    }

    function importFromText() {
        root.statusMessage = ""
        root.statusIsError = false

        const raw = importArea.textArea.text.trim()
        if (!raw) {
            root.statusMessage = qsTr("Вставьте JSON или список правил для импорта")
            root.statusIsError = true
            return
        }

        try {
            const parsed = JSON.parse(raw)
            const source = Array.isArray(parsed) ? { domains: parsed } : parsed
            if (source.name && !nameField.textField.text.trim()) {
                nameField.textField.text = String(source.name)
            }
            domainsArea.textArea.text = root.listToText(source.domains || [])
            suffixesArea.textArea.text = root.listToText(source.domain_suffixes || source.domainSuffixes || [])
            cidrsArea.textArea.text = root.listToText(source.cidrs || [])
            if (typeof source.enabled === "boolean") {
                root.editingEnabled = source.enabled
            }
            root.statusMessage = qsTr("Правила импортированы в форму. Проверьте их и сохраните профиль.")
            return
        } catch (e) {
        }

        domainsArea.textArea.text = raw
        root.statusMessage = qsTr("Текст импортирован как список доменов. При необходимости скорректируйте поля ниже.")
    }

    function submitProfile() {
        if (!root.canManageProfiles) {
            root.openSubscription()
            return
        }

        const name = nameField.textField.text.trim()
        if (!name) {
            root.statusMessage = qsTr("Введите название профиля")
            root.statusIsError = true
            return
        }

        const payload = {
            name: name,
            kind: root.editingKind,
            enabled: root.editingEnabled,
            domains: root.normalizeEntries(domainsArea.textArea.text),
            domain_suffixes: root.normalizeEntries(suffixesArea.textArea.text),
            cidrs: root.normalizeEntries(cidrsArea.textArea.text)
        }

        if (root.editingId > 0) {
            payload.id = root.editingId
        }

        root.statusMessage = ""
        root.statusIsError = false
        FBLinkController.saveRoutingProfile(payload)
    }

    function toggleProfile(profile) {
        if (!root.canManageProfiles) {
            root.openSubscription()
            return
        }

        FBLinkController.saveRoutingProfile({
            id: profile.id,
            name: profile.name,
            kind: profile.kind,
            enabled: !profile.enabled,
            domains: profile.domains || [],
            domain_suffixes: profile.domain_suffixes || [],
            cidrs: profile.cidrs || []
        })
    }

    function deleteProfile(profile) {
        if (!root.canManageProfiles) {
            root.openSubscription()
            return
        }
        FBLinkController.deleteRoutingProfile(Number(profile.id))
    }

    Connections {
        target: FBLinkController

        function onRoutingProfilesFetched(profiles) {
            root.profiles = profiles
        }

        function onRoutingProfilesError(errorMessage) {
            root.statusMessage = errorMessage
            root.statusIsError = true
        }

        function onRoutingProfileSaved() {
            root.statusMessage = qsTr("Профиль маршрутизации сохранён")
            root.statusIsError = false
            root.resetEditor()
            FBLinkController.fetchRoutingProfiles()
            PageController.showNotificationMessage(qsTr("VIP-профиль сохранён"))
        }

        function onRoutingProfileDeleted() {
            root.statusMessage = qsTr("Профиль маршрутизации удалён")
            root.statusIsError = false
            root.resetEditor()
            FBLinkController.fetchRoutingProfiles()
            PageController.showNotificationMessage(qsTr("VIP-профиль удалён"))
        }
    }

    Component.onCompleted: {
        if (FBLinkController.isLoggedIn) {
            FBLinkController.fetchRoutingProfiles()
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: content.implicitHeight + 32
        clip: true

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: content
            width: parent.width
            spacing: 0

            BackButtonType {
                Layout.topMargin: 20 + SettingsController.safeAreaTopMargin
                Layout.leftMargin: 4
            }

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12
                headerText: qsTr("VIP-конфигурации маршрутизации")
            }

            WarningType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12

                textString: root.canManageProfiles
                    ? qsTr("Системный профиль «RU without VPN» и ваши правила будут встроены в VLESS-конфиг при подключении.")
                    : qsTr("Конфигурации видны, но управлять ими можно только в VIP. После даунгрейда правила сохраняются и остаются выключенными.")
                iconPath: "qrc:/images/controls/alert-circle.svg"
                backGroundColor: root.canManageProfiles ? Qt.rgba(0, 200/255, 255/255, 0.08) : Qt.rgba(245/255, 158/255, 11/255, 0.14)
                textColor: FBLinkStyle.color.paleGray
                imageColor: root.canManageProfiles ? "#00C8FF" : "#F59E0B"
            }

            BasicButtonType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 12
                visible: !root.canManageProfiles

                text: qsTr("Открыть VIP-подписку")
                defaultColor: "#00C8FF"
                hoveredColor: "#33D4FF"
                pressedColor: "#0099BB"
                textColor: "#FFFFFF"

                clickedFunc: root.openSubscription
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 18
                implicitHeight: profilesColumn.implicitHeight + 28
                radius: 20
                color: FBLinkStyle.color.onyxBlack
                border.color: FBLinkStyle.color.slateGray
                border.width: 1

                ColumnLayout {
                    id: profilesColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            visible: !root.compactLayout

                            LabelTextType {
                                text: qsTr("Активные и сохранённые профили")
                                font.pixelSize: 18
                                font.weight: 700
                                color: FBLinkStyle.color.paleGray
                                Layout.fillWidth: true
                            }

                            BasicButtonType {
                                visible: root.canManageProfiles
                                implicitHeight: 40
                                text: qsTr("Новый профиль")
                                defaultColor: "#00C8FF"
                                hoveredColor: "#33D4FF"
                                pressedColor: "#0099BB"
                                textColor: "#FFFFFF"
                                clickedFunc: root.resetEditor
                            }
                        }

                        LabelTextType {
                            visible: root.compactLayout
                            text: qsTr("Активные и сохранённые профили")
                            font.pixelSize: 18
                            font.weight: 700
                            color: FBLinkStyle.color.paleGray
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }

                        BasicButtonType {
                            visible: root.compactLayout && root.canManageProfiles
                            Layout.fillWidth: true
                            implicitHeight: 44
                            text: qsTr("Новый профиль")
                            defaultColor: "#00C8FF"
                            hoveredColor: "#33D4FF"
                            pressedColor: "#0099BB"
                            textColor: "#FFFFFF"
                            clickedFunc: root.resetEditor
                        }
                    }

                    LabelTextType {
                        visible: root.profiles.length === 0
                        text: qsTr("Пока нет профилей маршрутизации")
                        font.pixelSize: 14
                        color: FBLinkStyle.color.mutedGray
                    }

                    Repeater {
                        model: root.profiles

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            radius: 16
                            color: Qt.rgba(255, 255, 255, 0.03)
                            border.color: Qt.rgba(255, 255, 255, 0.08)
                            border.width: 1
                            implicitHeight: cardColumn.implicitHeight + 24

                            property var profileData: modelData

                            ColumnLayout {
                                id: cardColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 16
                                spacing: 10

                                LabelTextType {
                                    text: profileData.name || qsTr("Без названия")
                                    font.pixelSize: 16
                                    font.weight: 700
                                    color: FBLinkStyle.color.paleGray
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                }

                                Item {
                                    Layout.fillWidth: true
                                    implicitHeight: chipsFlow.implicitHeight

                                    Flow {
                                        id: chipsFlow
                                        width: parent.width
                                        spacing: 8

                                        Rectangle {
                                            height: 22
                                            width: kindText.implicitWidth + 14
                                            radius: 11
                                            color: profileData.kind === "system"
                                                ? Qt.rgba(16/255, 185/255, 129/255, 0.18)
                                                : Qt.rgba(0, 200/255, 255/255, 0.16)

                                            LabelTextType {
                                                id: kindText
                                                anchors.centerIn: parent
                                                text: profileData.kind === "system" ? qsTr("СИСТЕМНЫЙ") : qsTr("ПОЛЬЗОВАТЕЛЬСКИЙ")
                                                font.pixelSize: 10
                                                font.weight: 700
                                                color: profileData.kind === "system" ? "#10B981" : "#00C8FF"
                                            }
                                        }

                                        Rectangle {
                                            height: 22
                                            width: stateText.implicitWidth + 14
                                            radius: 11
                                            color: profileData.enabled
                                                ? Qt.rgba(16/255, 185/255, 129/255, 0.18)
                                                : Qt.rgba(255, 255, 255, 0.08)

                                            LabelTextType {
                                                id: stateText
                                                anchors.centerIn: parent
                                                text: profileData.enabled ? qsTr("ВКЛЮЧЕН") : qsTr("ВЫКЛЮЧЕН")
                                                font.pixelSize: 10
                                                font.weight: 700
                                                color: profileData.enabled ? "#10B981" : FBLinkStyle.color.mutedGray
                                            }
                                        }
                                    }
                                }

                                CaptionTextType {
                                    Layout.fillWidth: true
                                    text: root.rulesSummary(profileData)
                                    color: FBLinkStyle.color.mutedGray
                                    wrapMode: Text.WordWrap
                                }

                                LabelTextType {
                                    Layout.fillWidth: true
                                    visible: (profileData.domain_suffixes || []).length > 0
                                    text: qsTr("Суффиксы: %1").arg((profileData.domain_suffixes || []).join(", "))
                                    font.pixelSize: 12
                                    color: FBLinkStyle.color.lightGray
                                    wrapMode: Text.WordWrap
                                }

                                GridLayout {
                                    Layout.fillWidth: true
                                    columns: root.compactLayout ? 1 : 3
                                    columnSpacing: 10
                                    rowSpacing: 10

                                    BasicButtonType {
                                        Layout.fillWidth: true
                                        implicitHeight: 42
                                        text: profileData.enabled ? qsTr("Выключить") : qsTr("Включить")
                                        enabled: root.canManageProfiles
                                        defaultColor: profileData.enabled ? Qt.rgba(16/255, 185/255, 129/255, 0.18) : Qt.rgba(0, 200/255, 255/255, 0.16)
                                        hoveredColor: profileData.enabled ? Qt.rgba(16/255, 185/255, 129/255, 0.28) : Qt.rgba(0, 200/255, 255/255, 0.26)
                                        pressedColor: profileData.enabled ? Qt.rgba(16/255, 185/255, 129/255, 0.36) : Qt.rgba(0, 200/255, 255/255, 0.32)
                                        textColor: "#FFFFFF"
                                        clickedFunc: function() { root.toggleProfile(profileData) }
                                    }

                                    BasicButtonType {
                                        Layout.fillWidth: true
                                        implicitHeight: 42
                                        visible: profileData.kind !== "system"
                                        text: qsTr("Редактировать")
                                        defaultColor: Qt.rgba(255, 255, 255, 0.08)
                                        hoveredColor: Qt.rgba(255, 255, 255, 0.12)
                                        pressedColor: Qt.rgba(255, 255, 255, 0.18)
                                        textColor: FBLinkStyle.color.paleGray
                                        clickedFunc: function() { root.startEdit(profileData) }
                                    }

                                    BasicButtonType {
                                        Layout.fillWidth: true
                                        implicitHeight: 42
                                        visible: profileData.kind !== "system"
                                        text: qsTr("Удалить")
                                        enabled: root.canManageProfiles
                                        defaultColor: Qt.rgba(239/255, 68/255, 68/255, 0.14)
                                        hoveredColor: Qt.rgba(239/255, 68/255, 68/255, 0.22)
                                        pressedColor: Qt.rgba(239/255, 68/255, 68/255, 0.30)
                                        textColor: "#FF6B6B"
                                        clickedFunc: function() { root.deleteProfile(profileData) }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16
                Layout.bottomMargin: 24 + SettingsController.safeAreaBottomMargin
                implicitHeight: editorColumn.implicitHeight + 28
                radius: 20
                color: FBLinkStyle.color.onyxBlack
                border.color: FBLinkStyle.color.slateGray
                border.width: 1

                ColumnLayout {
                    id: editorColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 16
                    spacing: 12

                    LabelTextType {
                        Layout.fillWidth: true
                        text: root.editingId > 0 ? qsTr("Редактирование профиля") : qsTr("Новый профиль")
                        font.pixelSize: 18
                        font.weight: 700
                        color: FBLinkStyle.color.paleGray
                        wrapMode: Text.WordWrap
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        text: qsTr("Импортируйте JSON с полями domains / domain_suffixes / cidrs или заполните правила вручную.")
                        color: FBLinkStyle.color.mutedGray
                        wrapMode: Text.WordWrap
                    }

                    TextFieldWithHeaderType {
                        id: nameField
                        Layout.fillWidth: true
                        headerText: qsTr("Название профиля")
                        enabled: root.canManageProfiles
                        textField.placeholderText: root.compactLayout
                            ? qsTr("Например, RU without VPN")
                            : qsTr("Например, RU without VPN для сервисов")
                    }

                    TextAreaType {
                        id: importArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: 116
                        enabled: root.canManageProfiles
                        placeholderText: root.compactLayout
                            ? qsTr("Вставьте JSON\nили список правил")
                            : qsTr("Вставьте JSON или список правил для быстрого импорта")
                    }

                    RowLayout {
                        visible: !root.compactLayout
                        Layout.fillWidth: true
                        spacing: 10

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 44
                            enabled: root.canManageProfiles
                            text: qsTr("Импортировать в форму")
                            defaultColor: Qt.rgba(255, 255, 255, 0.08)
                            hoveredColor: Qt.rgba(255, 255, 255, 0.12)
                            pressedColor: Qt.rgba(255, 255, 255, 0.18)
                            textColor: FBLinkStyle.color.paleGray
                            clickedFunc: root.importFromText
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 44
                            text: root.editingEnabled ? qsTr("Профиль будет включён") : qsTr("Профиль будет выключен")
                            enabled: root.canManageProfiles
                            defaultColor: root.editingEnabled ? Qt.rgba(16/255, 185/255, 129/255, 0.18) : Qt.rgba(255, 255, 255, 0.08)
                            hoveredColor: root.editingEnabled ? Qt.rgba(16/255, 185/255, 129/255, 0.28) : Qt.rgba(255, 255, 255, 0.12)
                            pressedColor: root.editingEnabled ? Qt.rgba(16/255, 185/255, 129/255, 0.34) : Qt.rgba(255, 255, 255, 0.18)
                            textColor: "#FFFFFF"
                            clickedFunc: function() { root.editingEnabled = !root.editingEnabled }
                        }
                    }

                    ColumnLayout {
                        visible: root.compactLayout
                        Layout.fillWidth: true
                        spacing: 10

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 44
                            enabled: root.canManageProfiles
                            text: qsTr("Импортировать в форму")
                            defaultColor: Qt.rgba(255, 255, 255, 0.08)
                            hoveredColor: Qt.rgba(255, 255, 255, 0.12)
                            pressedColor: Qt.rgba(255, 255, 255, 0.18)
                            textColor: FBLinkStyle.color.paleGray
                            clickedFunc: root.importFromText
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 44
                            text: root.editingEnabled ? qsTr("Профиль будет включён") : qsTr("Профиль будет выключен")
                            enabled: root.canManageProfiles
                            defaultColor: root.editingEnabled ? Qt.rgba(16/255, 185/255, 129/255, 0.18) : Qt.rgba(255, 255, 255, 0.08)
                            hoveredColor: root.editingEnabled ? Qt.rgba(16/255, 185/255, 129/255, 0.28) : Qt.rgba(255, 255, 255, 0.12)
                            pressedColor: root.editingEnabled ? Qt.rgba(16/255, 185/255, 129/255, 0.34) : Qt.rgba(255, 255, 255, 0.18)
                            textColor: "#FFFFFF"
                            clickedFunc: function() { root.editingEnabled = !root.editingEnabled }
                        }
                    }

                    TextAreaType {
                        id: domainsArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: 116
                        enabled: root.canManageProfiles
                        placeholderText: qsTr("Домены, по одному на строку\nexample.com")
                    }

                    TextAreaType {
                        id: suffixesArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: 116
                        enabled: root.canManageProfiles
                        placeholderText: qsTr("Суффиксы доменов, по одному на строку\n.ru")
                    }

                    TextAreaType {
                        id: cidrsArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: 116
                        enabled: root.canManageProfiles
                        placeholderText: qsTr("CIDR-подсети, по одной на строку\n192.168.0.0/16")
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        visible: root.statusMessage !== ""
                        color: root.statusIsError ? "#3D1515" : Qt.rgba(0, 200/255, 255/255, 0.1)
                        border.color: root.statusIsError ? "#FF6B6B" : Qt.rgba(0, 200/255, 255/255, 0.35)
                        border.width: 1
                        radius: 12
                        implicitHeight: statusLabel.implicitHeight + 18

                        LabelTextType {
                            id: statusLabel
                            anchors.centerIn: parent
                            width: parent.width - 24
                            wrapMode: Text.WordWrap
                            text: root.statusMessage
                            color: root.statusIsError ? "#FF6B6B" : "#80D8FF"
                            font.pixelSize: 12
                        }
                    }

                    RowLayout {
                        visible: !root.compactLayout
                        Layout.fillWidth: true
                        spacing: 10

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 48
                            enabled: root.canManageProfiles
                            text: root.editingId > 0 ? qsTr("Сохранить изменения") : qsTr("Создать профиль")
                            defaultColor: "#00C8FF"
                            hoveredColor: "#33D4FF"
                            pressedColor: "#0099BB"
                            textColor: "#FFFFFF"
                            clickedFunc: root.submitProfile
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 48
                            text: qsTr("Сбросить")
                            defaultColor: Qt.rgba(255, 255, 255, 0.08)
                            hoveredColor: Qt.rgba(255, 255, 255, 0.12)
                            pressedColor: Qt.rgba(255, 255, 255, 0.18)
                            textColor: FBLinkStyle.color.paleGray
                            clickedFunc: root.resetEditor
                        }
                    }

                    ColumnLayout {
                        visible: root.compactLayout
                        Layout.fillWidth: true
                        spacing: 10

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 48
                            enabled: root.canManageProfiles
                            text: root.editingId > 0 ? qsTr("Сохранить изменения") : qsTr("Создать профиль")
                            defaultColor: "#00C8FF"
                            hoveredColor: "#33D4FF"
                            pressedColor: "#0099BB"
                            textColor: "#FFFFFF"
                            clickedFunc: root.submitProfile
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 48
                            text: qsTr("Сбросить")
                            defaultColor: Qt.rgba(255, 255, 255, 0.08)
                            hoveredColor: Qt.rgba(255, 255, 255, 0.12)
                            pressedColor: Qt.rgba(255, 255, 255, 0.18)
                            textColor: FBLinkStyle.color.paleGray
                            clickedFunc: root.resetEditor
                        }
                    }
                }
            }
        }
    }
}
