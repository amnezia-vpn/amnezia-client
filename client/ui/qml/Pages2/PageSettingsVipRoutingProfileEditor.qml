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

    property string statusMessage: ""
    property bool statusIsError: false
    property int editingId: -1
    property bool editingEnabled: true
    property string editingAction: "direct"

    readonly property bool canManageProfiles: FBLinkController.canManageRoutingProfiles
    readonly property bool isRoutingLocked: ConnectionController.isConnected || ConnectionController.isConnectionInProgress
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
        if (root.isRoutingLocked) {
            const message = qsTr("Нельзя менять профили маршрутизации во время активного подключения")
            statusMessage = message
            statusIsError = true
            PageController.showNotificationMessage(message)
            return
        }
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
        if (root.isRoutingLocked) {
            root.statusMessage = qsTr("Редактирование профиля временно заблокировано до отключения VPN.")
            root.statusIsError = true
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

                Item {
                    Layout.fillWidth: true
                    Layout.topMargin: 16 + SettingsController.safeAreaTopMargin
                    implicitHeight: 28

                    RowLayout {
                        anchors.fill: parent
                        spacing: 8

                        Item {
                            Layout.preferredHeight: 28
                            Layout.preferredWidth: cancelText.implicitWidth + 6

                            LabelTextType {
                                id: cancelText
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                text: qsTr("Отмена")
                                font.pixelSize: 16
                                color: FBLinkStyle.color.mutedGray
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.goBack()
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Item {
                            Layout.preferredHeight: 28
                            Layout.preferredWidth: 28
                            enabled: root.canManageProfiles && !root.isRoutingLocked
                            opacity: enabled ? 1.0 : 0.5

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/images/controls/check.svg"
                                sourceSize: Qt.size(20, 20)
                                layer.enabled: true
                                layer.effect: ColorOverlay { color: "#10B981" }
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: root.canManageProfiles && !root.isRoutingLocked
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.submitProfile()
                            }
                        }
                    }
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 12
                    fillColor: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    accentVisible: false

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
                    fillColor: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    accentVisible: false

                    TextFieldWithHeaderType {
                        id: nameField
                        Layout.fillWidth: true
                        headerText: qsTr("Название")
                        enabled: root.canManageProfiles && !root.isRoutingLocked
                        textField.placeholderText: qsTr("Например, AI через VPN")
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        radius: 14
                        color: Qt.rgba(1, 1, 1, 0.10)
                        border.width: 1
                        border.color: Qt.rgba(255, 255, 255, 0.12)
                        enabled: root.canManageProfiles && !root.isRoutingLocked
                        opacity: enabled ? 1.0 : 0.55

                        Rectangle {
                            id: actionThumb
                            y: 3
                            width: (parent.width - 6) / 2
                            height: parent.height - 6
                            radius: 11
                            x: root.editingAction === "proxy" ? parent.width - width - 3 : 3
                            color: root.editingAction === "proxy"
                                ? Qt.rgba(234/255, 179/255, 8/255, 0.22)
                                : Qt.rgba(16/255, 185/255, 129/255, 0.22)

                            Behavior on x {
                                NumberAnimation { duration: 180; easing.type: Easing.InOutQuad }
                            }
                            Behavior on color {
                                ColorAnimation { duration: 180 }
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 3
                            spacing: 0

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                LabelTextType {
                                    anchors.centerIn: parent
                                    text: qsTr("Без VPN")
                                    font.pixelSize: 16
                                    font.weight: 600
                                    color: root.editingAction === "direct" ? "#FFFFFF" : FBLinkStyle.color.lightGray
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: root.canManageProfiles && !root.isRoutingLocked
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.editingAction = "direct"
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                LabelTextType {
                                    anchors.centerIn: parent
                                    text: qsTr("Через VPN")
                                    font.pixelSize: 16
                                    font.weight: 600
                                    color: root.editingAction === "proxy" ? "#FFFFFF" : FBLinkStyle.color.lightGray
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    enabled: root.canManageProfiles && !root.isRoutingLocked
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.editingAction = "proxy"
                                }
                            }
                        }
                    }

                    TextAreaType {
                        id: domainsArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: 84
                        enabled: root.canManageProfiles && !root.isRoutingLocked
                        placeholderText: qsTr("Домены, по одному на строку")
                    }

                    TextAreaType {
                        id: suffixesArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: 84
                        enabled: root.canManageProfiles && !root.isRoutingLocked
                        placeholderText: qsTr("Суффиксы доменов, по одному на строку")
                    }

                    TextAreaType {
                        id: cidrsArea
                        Layout.fillWidth: true
                        Layout.preferredHeight: 84
                        enabled: root.canManageProfiles && !root.isRoutingLocked
                        placeholderText: qsTr("CIDR-подсети, по одной на строку")
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
