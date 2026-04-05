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
    readonly property bool canManageProfiles: FBLinkController.canManageRoutingProfiles
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)
    readonly property var systemProfiles: root.profiles.filter(function(profile) { return profile.kind === "system" })
    readonly property var copiedTemplateCodes: root.profiles
        .filter(function(profile) { return profile.kind !== "system" && profile.template_code && profile.template_code.length > 0 })
        .map(function(profile) { return String(profile.template_code) })

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
                : qsTr("Пресет уже добавлен")
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
        contentHeight: content.implicitHeight + 26

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Item {
            width: parent.width
            height: content.implicitHeight + 26

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
                    radius: 16
                    accentVisible: false
                    fillColor: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)

                    LabelTextType {
                        Layout.fillWidth: true
                        text: qsTr("Системные конфиги")
                        font.pixelSize: root.wideLayout ? 24 : 21
                        font.weight: 700
                        color: FBLinkStyle.color.paleGray
                        wrapMode: Text.WordWrap
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        text: qsTr("Выберите конфиг и добавьте его в «Мои профили».")
                        color: FBLinkStyle.color.mutedGray
                        wrapMode: Text.WordWrap
                    }
                }

                Repeater {
                    model: root.systemProfiles

                    delegate: Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 74
                        radius: 14
                        color: Qt.rgba(16/255, 16/255, 16/255, 1.0)
                        border.width: 1
                        border.color: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                        property var profileData: modelData

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 10
                            spacing: 10

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                LabelTextType {
                                    Layout.fillWidth: true
                                    text: profileData.name || qsTr("Без названия")
                                    font.pixelSize: 16
                                    font.weight: 700
                                    color: FBLinkStyle.color.paleGray
                                    elide: Text.ElideRight
                                }

                                CaptionTextType {
                                    Layout.fillWidth: true
                                    text: profileData.description || ""
                                    color: FBLinkStyle.color.mutedGray
                                    elide: Text.ElideRight
                                }
                            }

                            Item {
                                Layout.preferredWidth: 36
                                Layout.preferredHeight: 36

                                Image {
                                    anchors.centerIn: parent
                                    source: root.isAdded(profileData)
                                        ? "qrc:/images/controls/check.svg"
                                        : "qrc:/images/controls/plus.svg"
                                    sourceSize: Qt.size(20, 20)
                                    layer.enabled: true
                                    layer.effect: ColorOverlay {
                                        color: root.isAdded(profileData) ? "#10B981" : "#EAB308"
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onClicked: {
                                        if (root.isAdded(profileData)) {
                                            root.openMyProfiles()
                                        } else {
                                            root.addPreset(profileData)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                LabelTextType {
                    Layout.fillWidth: true
                    visible: root.systemProfiles.length === 0
                    text: qsTr("Нет доступных системных конфигов")
                    font.pixelSize: 13
                    color: FBLinkStyle.color.mutedGray
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 24 + SettingsController.safeAreaBottomMargin
                }
            }
        }
    }
}
