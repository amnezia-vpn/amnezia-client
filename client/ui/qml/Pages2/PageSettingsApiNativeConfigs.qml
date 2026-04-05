import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property string configExtension: ".conf"
    property string configCaption: qsTr("Save FBLink VPN config")
    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)

    function statusText(isIssued, isWorkerExpired) {
        if (!isIssued) return qsTr("НЕ ВЫПУЩЕН")
        return isWorkerExpired ? qsTr("ТРЕБУЕТСЯ ОБНОВЛЕНИЕ") : qsTr("ГОТОВ")
    }

    function statusTone(isIssued, isWorkerExpired) {
        if (!isIssued) return "neutral"
        return isWorkerExpired ? "warning" : "success"
    }

    function rowDescription(isIssued, isWorkerExpired) {
        if (!isIssued) return qsTr("Сконфигурируйте и скачайте файл для роутера или AWG-клиента.")
        if (isWorkerExpired) return qsTr("Текущий файл устарел. Выпустите новую конфигурацию.")
        return qsTr("Файл активен. Можно перевыпустить или отозвать через меню.")
    }

    Flickable {
        anchors.fill: parent
        clip: true
        contentHeight: content.implicitHeight + 24

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Item {
            width: parent.width
            height: content.implicitHeight + 24

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
                    padding: 14
                    radius: 16
                    fillColor: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    accentVisible: true
                    accentColor: "#EAB308"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        PremiumBadge { text: qsTr("СИСТЕМНЫЕ КОНФИГИ"); tone: "warning"; compact: true }
                        PremiumBadge { text: qsTr("AWG / РОУТЕРЫ"); tone: "neutral"; compact: true }
                    }

                    LabelTextType {
                        Layout.fillWidth: true
                        text: qsTr("Конфигурации для устройств")
                        font.pixelSize: root.wideLayout ? 24 : 21
                        font.weight: 700
                        color: FBLinkStyle.color.paleGray
                        wrapMode: Text.WordWrap
                    }

                    CaptionTextType {
                        Layout.fillWidth: true
                        text: qsTr("Скачивайте готовые конфиги по странам, перевыпускайте при необходимости и отзывайте старые файлы.")
                        color: FBLinkStyle.color.mutedGray
                        wrapMode: Text.WordWrap
                    }
                }

                Repeater {
                    model: ApiCountryModel

                    delegate: PremiumPanel {
                        Layout.fillWidth: true
                        padding: 12
                        radius: 16
                        fillColor: Qt.rgba(16/255, 16/255, 16/255, 1.0)
                        outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                        accentVisible: false

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 12

                            Rectangle {
                                Layout.preferredWidth: 42
                                Layout.preferredHeight: 42
                                radius: 11
                                color: Qt.rgba(255, 255, 255, 0.04)
                                border.width: 1
                                border.color: Qt.rgba(255, 255, 255, 0.12)

                                Image {
                                    anchors.centerIn: parent
                                    width: 24
                                    height: 24
                                    fillMode: Image.PreserveAspectFit
                                    source: "qrc:/countriesFlags/images/flagKit/" + countryImageCode + ".svg"
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 5

                                LabelTextType {
                                    Layout.fillWidth: true
                                    text: countryName
                                    font.pixelSize: 17
                                    font.weight: 700
                                    color: FBLinkStyle.color.paleGray
                                    elide: Text.ElideRight
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    width: parent ? parent.width : 0
                                    spacing: 8
                                    PremiumBadge {
                                        text: root.statusText(isIssued, isWorkerExpired)
                                        tone: root.statusTone(isIssued, isWorkerExpired)
                                        compact: true
                                    }
                                    PremiumBadge {
                                        text: qsTr("AWG")
                                        tone: "neutral"
                                        compact: true
                                    }
                                }
                            }

                            BasicButtonType {
                                implicitHeight: 38
                                implicitWidth: 126
                                text: isIssued ? qsTr("Опции") : qsTr("Скачать")
                                defaultColor: isIssued ? Qt.rgba(255, 255, 255, 0.10) : Qt.rgba(234/255, 179/255, 8/255, 0.18)
                                hoveredColor: isIssued ? Qt.rgba(255, 255, 255, 0.15) : Qt.rgba(234/255, 179/255, 8/255, 0.28)
                                pressedColor: isIssued ? Qt.rgba(255, 255, 255, 0.20) : Qt.rgba(234/255, 179/255, 8/255, 0.34)
                                textColor: "#FFFFFF"
                                clickedFunc: function() {
                                    if (isIssued) {
                                        moreOptionsDrawer.countryName = countryName
                                        moreOptionsDrawer.countryCode = countryCode
                                        moreOptionsDrawer.openTriggered()
                                    } else {
                                        root.issueConfig(countryCode)
                                    }
                                }
                            }
                        }

                        CaptionTextType {
                            Layout.fillWidth: true
                            text: root.rowDescription(isIssued, isWorkerExpired)
                            color: FBLinkStyle.color.mutedGray
                            wrapMode: Text.WordWrap
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

    DrawerType2 {
        id: moreOptionsDrawer

        property string countryName
        property string countryCode

        anchors.fill: parent
        expandedHeight: parent.height * 0.45

        expandedStateContent: Item {
            implicitHeight: moreOptionsDrawer.expandedHeight

            BackButtonType {
                id: moreOptionsDrawerBackButton
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 16
                backButtonFunction: function() { moreOptionsDrawer.closeTriggered() }
            }

            ColumnLayout {
                anchors.top: moreOptionsDrawerBackButton.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 10

                PremiumPanel {
                    Layout.fillWidth: true
                    padding: 14
                    radius: 14
                    fillColor: Qt.rgba(18/255, 18/255, 18/255, 1.0)
                    outlineColor: Qt.rgba(63/255, 63/255, 70/255, 0.9)
                    accentVisible: false

                    LabelTextType {
                        Layout.fillWidth: true
                        text: qsTr("%1 — управление конфигом").arg(moreOptionsDrawer.countryName)
                        font.pixelSize: 17
                        font.weight: 700
                        color: FBLinkStyle.color.paleGray
                        wrapMode: Text.WordWrap
                    }

                    BasicButtonType {
                        Layout.fillWidth: true
                        implicitHeight: 42
                        text: qsTr("Выпустить новый конфиг")
                        defaultColor: Qt.rgba(234/255, 179/255, 8/255, 0.16)
                        hoveredColor: Qt.rgba(234/255, 179/255, 8/255, 0.26)
                        pressedColor: Qt.rgba(234/255, 179/255, 8/255, 0.34)
                        textColor: "#FFFFFF"
                        clickedFunc: function() {
                            root.showQuestion(true, moreOptionsDrawer.countryCode, moreOptionsDrawer.countryName)
                        }
                    }

                    BasicButtonType {
                        Layout.fillWidth: true
                        implicitHeight: 42
                        text: qsTr("Отозвать текущий конфиг")
                        defaultColor: Qt.rgba(239/255, 68/255, 68/255, 0.16)
                        hoveredColor: Qt.rgba(239/255, 68/255, 68/255, 0.24)
                        pressedColor: Qt.rgba(239/255, 68/255, 68/255, 0.30)
                        textColor: "#F87171"
                        clickedFunc: function() {
                            root.showQuestion(false, moreOptionsDrawer.countryCode, moreOptionsDrawer.countryName)
                        }
                    }
                }
            }
        }
    }

    function issueConfig(countryCode) {
        var fileName = ""
        if (GC.isMobile()) {
            fileName = countryCode + configExtension
        } else {
            fileName = SystemController.getFileName(configCaption,
                                                    qsTr("Config files (*" + configExtension + ")"),
                                                    StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/" + countryCode,
                                                    true,
                                                    configExtension)
        }
        if (fileName !== "") {
            PageController.showBusyIndicator(true)
            let result = ApiConfigsController.exportNativeConfig(countryCode, fileName)
            if (result) {
                ApiSettingsController.getAccountInfo(true)
            }

            PageController.showBusyIndicator(false)
            if (result) {
                PageController.showNotificationMessage(qsTr("Config file saved"))
            }
        }
    }

    function revokeConfig(countryCode) {
        PageController.showBusyIndicator(true)
        let result = ApiConfigsController.revokeNativeConfig(countryCode)
        if (result) {
            ApiSettingsController.getAccountInfo(true)
        }
        PageController.showBusyIndicator(false)

        if (result) {
            PageController.showNotificationMessage(qsTr("The config has been revoked"))
        }
    }

    function showQuestion(isConfigIssue, countryCode, countryName) {
        var headerText
        if (isConfigIssue) {
            headerText = qsTr("Generate a new %1 configuration file?").arg(countryName)
        } else {
            headerText = qsTr("Revoke the current %1 configuration file?").arg(countryName)
        }

        var descriptionText = qsTr("Your previous configuration file will no longer work, and it will not be possible to connect using it")
        var yesButtonText = isConfigIssue ? qsTr("Download") : qsTr("Continue")
        var noButtonText = qsTr("Cancel")

        var yesButtonFunction = function() {
            if (isConfigIssue) {
                issueConfig(countryCode)
            } else {
                revokeConfig(countryCode)
            }
            moreOptionsDrawer.closeTriggered()
        }
        var noButtonFunction = function() {}

        showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
    }
}
