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

    property string statusMessage: ""
    property bool statusIsError: false
    property int editingId: -1
    property bool editingEnabled: true
    property string editingAction: "direct"

    readonly property bool canManageProfiles: FBLinkController.canManageRoutingProfiles
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)
    readonly property bool isEditMode: editingId > 0

    function normalizeEntries(rawText) {
        return rawText
            .split(/[\n,;]+/)
            .map(function(entry) { return entry.trim() })
            .filter(function(entry) { return entry.length > 0 })
    }

    function listToText(list) {
        return list && list.length ? list.join("\n") : ""
    }

    function applyDraft() {
        const draft = GC.vipRoutingProfileEditorDraft || {}
        editingId = Number(draft.id || -1)
        editingEnabled = draft.enabled !== false
        editingAction = String(draft.action || "direct")
        nameField.textField.text = String(draft.name || "")
        domainsArea.textArea.text = listToText(draft.domains || [])
        suffixesArea.textArea.text = listToText(draft.domain_suffixes || [])
        cidrsArea.textArea.text = listToText(draft.cidrs || [])
    }

    function goBack() {
        statusMessage = ""
        statusIsError = false
        GC.vipRoutingProfileEditorDraft = ({})
        PageController.closePage()
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
        if (editingId > 0) {
            payload.id = editingId
        }
        FBLinkController.saveRoutingProfile(payload)
    }

    Connections {
        target: FBLinkController

        function onRoutingProfileSaved() {
            GC.vipRoutingProfilesStatusMessage = qsTr("Профиль сохранён")
            GC.vipRoutingProfilesStatusIsError = false
            GC.vipRoutingProfileEditorDraft = ({})
            PageController.closePage()
        }

        function onRoutingProfilesError(errorMessage) {
            root.statusMessage = errorMessage
            root.statusIsError = true
        }

        function onRequestError(errorMessage) {
            root.statusMessage = errorMessage
            root.statusIsError = true
        }
    }

    Component.onCompleted: {
        applyDraft()
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
                    backButtonFunction: root.goBack
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 12
                    accentVisible: true
                    accentColor: "#EAB308"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        PremiumBadge {
                            text: qsTr("VIP")
                            tone: "accent"
                            iconSource: "qrc:/images/controls/tag.svg"
                        }
                        PremiumBadge {
                            text: root.isEditMode ? qsTr("РЕДАКТИРОВАНИЕ") : qsTr("СОЗДАНИЕ")
                            tone: "success"
                            iconSource: "qrc:/images/controls/edit-3.svg"
                        }
                    }

                    LabelTextType {
                        Layout.fillWidth: true
                        text: root.isEditMode ? qsTr("Редактирование профиля") : qsTr("Новый профиль")
                        font.pixelSize: root.wideLayout ? 24 : 21
                        font.weight: 700
                        color: FBLinkStyle.color.paleGray
                        wrapMode: Text.WordWrap
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        text: qsTr("Настройте правила маршрутизации и сохраните профиль.")
                        color: FBLinkStyle.color.mutedGray
                        wrapMode: Text.WordWrap
                    }
                }

                WarningType {
                    Layout.fillWidth: true
                    visible: root.statusMessage !== ""
                    textString: root.statusMessage
                    iconPath: root.statusIsError ? "qrc:/images/controls/alert-circle.svg" : "qrc:/images/controls/check.svg"
                    backGroundColor: root.statusIsError ? Qt.rgba(239/255, 68/255, 68/255, 0.12) : Qt.rgba(16/255, 185/255, 129/255, 0.12)
                    imageColor: root.statusIsError ? "#EF4444" : "#10B981"
                    textColor: root.statusIsError ? "#FFB4B4" : "#B6F2D2"
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 12
                    accentVisible: true
                    accentColor: "#EAB308"

                    TextFieldWithHeaderType {
                        id: nameField
                        Layout.fillWidth: true
                        headerText: qsTr("Название")
                        enabled: root.canManageProfiles
                        textField.placeholderText: qsTr("Например, AI через VPN")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 48
                            enabled: root.canManageProfiles
                            text: qsTr("Без VPN")
                            defaultColor: root.editingAction === "direct" ? Qt.rgba(16/255, 185/255, 129/255, 0.18) : Qt.rgba(1, 1, 1, 0.08)
                            hoveredColor: root.editingAction === "direct" ? Qt.rgba(16/255, 185/255, 129/255, 0.28) : Qt.rgba(1, 1, 1, 0.12)
                            pressedColor: root.editingAction === "direct" ? Qt.rgba(16/255, 185/255, 129/255, 0.34) : Qt.rgba(1, 1, 1, 0.18)
                            textColor: "#FFFFFF"
                            clickedFunc: function() { root.editingAction = "direct" }
                        }

                        BasicButtonType {
                            Layout.fillWidth: true
                            implicitHeight: 48
                            enabled: root.canManageProfiles
                            text: qsTr("Через VPN")
                            defaultColor: root.editingAction === "proxy" ? Qt.rgba(234/255, 179/255, 8/255, 0.16) : Qt.rgba(1, 1, 1, 0.08)
                            hoveredColor: root.editingAction === "proxy" ? Qt.rgba(234/255, 179/255, 8/255, 0.26) : Qt.rgba(1, 1, 1, 0.12)
                            pressedColor: root.editingAction === "proxy" ? Qt.rgba(234/255, 179/255, 8/255, 0.32) : Qt.rgba(1, 1, 1, 0.18)
                            textColor: "#FFFFFF"
                            clickedFunc: function() { root.editingAction = "proxy" }
                        }
                    }

                    TextAreaType {
                        id: domainsArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: 84
                        enabled: root.canManageProfiles
                        placeholderText: qsTr("Домены, по одному на строку")
                    }

                    TextAreaType {
                        id: suffixesArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: 84
                        enabled: root.canManageProfiles
                        placeholderText: qsTr("Суффиксы доменов, по одному на строку")
                    }

                    TextAreaType {
                        id: cidrsArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: 84
                        enabled: root.canManageProfiles
                        placeholderText: qsTr("CIDR-подсети, по одной на строку")
                    }

                    Flow {
                        Layout.fillWidth: true
                        width: parent ? parent.width : 0
                        spacing: 10

                        BasicButtonType {
                            width: root.wideLayout ? 220 : parent.width
                            implicitHeight: 44
                            enabled: root.canManageProfiles
                            text: root.isEditMode ? qsTr("Сохранить") : qsTr("Создать профиль")
                            defaultColor: "#EAB308"
                            hoveredColor: "#FACC15"
                            pressedColor: "#CA8A04"
                            textColor: "#FFFFFF"
                            clickedFunc: root.submitProfile
                        }

                        BasicButtonType {
                            width: root.wideLayout ? 220 : parent.width
                            implicitHeight: 44
                            text: qsTr("Отмена")
                            defaultColor: Qt.rgba(1, 1, 1, 0.08)
                            hoveredColor: Qt.rgba(1, 1, 1, 0.12)
                            pressedColor: Qt.rgba(1, 1, 1, 0.18)
                            textColor: FBLinkStyle.color.paleGray
                            clickedFunc: root.goBack
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
