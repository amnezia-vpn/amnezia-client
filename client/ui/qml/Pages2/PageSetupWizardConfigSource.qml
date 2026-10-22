import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QtCore

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root


    property bool isRestoringBackup: false

    Connections {
        target: ImportController

        function onQrDecodingFinished() {
            if (Qt.platform.os === "ios") {
                PageController.closePage()
            }
            PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
        }
    }

    ListViewType {
        id: listView

        anchors.fill: parent

        header: ColumnLayout {
            width: listView.width

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Connection")
                description: qsTr("Insert the key, add a configuration file or scan the QR-code")
                backButtonVisible: root.stackView !== null && root.stackView.depth > 1

                IconButtonType {
                    id: moreButton

                    visible: SettingsController.getInstallationUuid() !== "" || PageController.isStartPageVisible()
                    size: IconButtonType.Small
                    icon.source: "qrc:/images/controls/more-vertical.svg"

                    onClicked: moreActionsDrawer.open()
                }
            }

            SheetType {
                id: moreActionsDrawer

                parent: root

                expandedHeight: moreSheetContent.implicitHeight + 16 + PageController.safeAreaBottomMargin

                ColumnLayout {
                    id: moreSheetContent

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 0

                    SectionHeaderType {
                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16

                        headerText: qsTr("Settings")
                        descriptionText: supportUuidRow.visible ? qsTr("Support tag") : ""
                    }

                    SwitchType {
                        id: switcher
                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        text: qsTr("Enable logs")

                        visible: PageController.isStartPageVisible()
                        checked: SettingsController.isLoggingEnabled
                        onToggled: function() {
                            if (checked !== SettingsController.isLoggingEnabled) {
                                SettingsController.isLoggingEnabled = checked
                            }
                        }
                    }

                    ListItemType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        text: qsTr("Export client logs")
                        trailingIconSource: "qrc:/images/controls/chevron-right.svg"

                        visible: PageController.isStartPageVisible()

                        onClicked: {
                            var fileName = ""
                            if (GC.isMobile()) {
                                fileName = "AmneziaVPN.log"
                            } else {
                                fileName = SystemController.getFileName(qsTr("Save"),
                                                                        qsTr("Logs files (*.log)"),
                                                                        StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/AmneziaVPN",
                                                                        true,
                                                                        ".log")
                            }
                            if (fileName !== "") {
                                PageController.showBusyIndicator(true)
                                SettingsController.exportLogsFile(fileName)
                                PageController.showBusyIndicator(false)
                                PageController.showNotificationMessage(qsTr("Logs file saved"))
                            }
                        }
                    }

                    RowLayout {
                        id: supportUuidRow

                        Layout.fillWidth: true
                        Layout.topMargin: 28
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        spacing: 16

                        visible: SettingsController.getInstallationUuid() !== ""

                        BodyTextType {
                            id: supportUuidText

                            Layout.fillWidth: true

                            text: SettingsController.getInstallationUuid()
                            elide: Text.ElideRight
                            wrapMode: Text.NoWrap
                        }

                        IconButtonType {
                            id: supportUuid

                            icon.source: "qrc:/images/controls/copy.svg"
                            Accessible.name: qsTr("Support tag")

                            onClicked: {
                                GC.copyToClipBoard(supportUuidText.text)
                                PageController.showNotificationMessage(qsTr("Copied"))
                                if (!GC.isMobile()) {
                                    supportUuid.forceActiveFocus(Qt.TabFocusReason)
                                }
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                spacing: 12

                TextFieldType {
                    id: textKey

                    Layout.fillWidth: true

                    headerText: qsTr("Insert key")
                    buttonText: qsTr("Insert")

                    clickedFunc: function() {
                        textField.text = ""
                        textField.paste()
                    }
                }

                ButtonType {
                    id: continueButton

                    Layout.fillWidth: true

                    visible: textKey.textField.text !== ""

                    text: qsTr("Continue")

                    onClicked: {
                        if (ImportController.extractConfigFromData(textKey.textField.text)) {
                            ImportController.openImportedConfig()
                        }
                    }
                }
            }

            SmallTextType {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Other connection options")
            }
        }

        model: variants

        delegate: ColumnLayout {
            width: listView.width

            BasicCardType {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                visible: isVisible

                text: title
                subtitle: description

                badgeText: featuredAmneziaConnection ? qsTr("Recommended") : ""

                icon.source: imageSource

                enabled: !root.isRestoringBackup

                onClicked: { handler() }
            }
        }

        footer: ColumnLayout {
            width: listView.width

            LinkButtonType {
                id: siteLink2
                Layout.topMargin: 24
                Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin
                Layout.alignment: Qt.AlignHCenter

                size: LinkButtonType.Medium

                visible: Qt.platform.os !== "ios" && !IsMacOsNeBuild

                text: qsTr("Site Amnezia")

                rightImageSource: "qrc:/images/controls/external-link.svg"

                onClicked: {
                    Qt.openUrlExternally(LanguageUiController.getCurrentSiteUrl())
                }
            }
        }
    }

    property list<QtObject> variants: [
        amneziaVpn,
        selfHostVpn,
        backupRestore,
        fileOpen,
        qrScan,
        restorePurchases,
        siteLink
    ]
    
    QtObject {
        id: amneziaVpn

        property string title: qsTr("VPN by Amnezia")
        property string description: qsTr("The easiest way to connect to the VPN")
        property string imageSource: "qrc:/images/controls/amnezia.svg"
        property bool featuredAmneziaConnection: true
        property bool isVisible: true
        property var handler: function() {
            PageController.showBusyIndicator(true)
            var result = SubscriptionUiController.fillAvailableServices()
            PageController.showBusyIndicator(false)
            if (result) {
                PageController.goToPage(PageEnum.PageSetupWizardApiServicesList)
            }
        }
    }

    QtObject {
        id: selfHostVpn

        property bool featuredAmneziaConnection: false
        property string title: qsTr("Self-hosted VPN")
        property string description: qsTr("Configure Amnezia VPN on your own server")
        property string imageSource: "qrc:/images/controls/server.svg"
        property bool isVisible: true
        property var handler: function() {
            PageController.goToPage(PageEnum.PageSetupWizardCredentials)
        }
    }

    QtObject {
        id: backupRestore

        property bool featuredAmneziaConnection: false
        property string title: qsTr("Restore from backup")
        property string description: qsTr("")
        property string imageSource: "qrc:/images/controls/archive-restore.svg"
        property bool isVisible: PageController.isStartPageVisible()
        property var handler: function() {
            if (root.isRestoringBackup) {
                return
            }
            var filePath = SystemController.getFileName(qsTr("Open backup file"),
                                                        qsTr("Backup files (*.backup)"))
            if (filePath !== "") {
                root.isRestoringBackup = true
                PageController.showBusyIndicator(true)
                Qt.callLater(function() {
                    SettingsController.restoreAppConfig(filePath)
                    PageController.showBusyIndicator(false)
                    root.isRestoringBackup = false
                })
            }
        }
    }

    QtObject {
        id: fileOpen

        property bool featuredAmneziaConnection: false
        property string title: qsTr("File with connection settings")
        property string description: qsTr("")
        property string imageSource: "qrc:/images/controls/folder-search-2.svg"
        property bool isVisible: true
        property var handler: function() {
            var nameFilter = "Config files (*.vpn *.ovpn *.conf *.json)"
            var fileName = SystemController.getFileName(qsTr("Open config file"), nameFilter)
            if (fileName !== "") {
                if (ImportController.extractConfigFromFile(fileName)) {
                    ImportController.openImportedConfig()
                }
            }
        }
    }

    QtObject {
        id: qrScan

        property bool featuredAmneziaConnection: false
        property string title: qsTr("QR code")
        property string description: qsTr("")
        property string imageSource: "qrc:/images/controls/scan-line.svg"
        property bool isVisible: SettingsController.isCameraPresent()
        property var handler: function() {
            ImportController.startDecodingQr()
            if (Qt.platform.os === "ios") {
                PageController.goToPage(PageEnum.PageSetupWizardQrReader)
            }
        }
    }

    QtObject {
        id: restorePurchases

        property bool featuredAmneziaConnection: false
        property string title: qsTr("Restore purchases")
        property string description: qsTr("")
        property string imageSource: "qrc:/images/controls/refresh-cw.svg"
        property bool isVisible: Qt.platform.os === "ios" || IsMacOsNeBuild || IsPlayBuild
        property var handler: function() {
            PageController.showBusyIndicator(true)
            SubscriptionUiController.restoreServiceFromStore()
            PageController.showBusyIndicator(false)
        }
    }

    QtObject {
        id: siteLink

        property bool featuredAmneziaConnection: false
        property string title: qsTr("I have nothing")
        property string description: qsTr("")
        property string imageSource: "qrc:/images/controls/help-circle.svg"
        property bool isVisible: PageController.isStartPageVisible() && Qt.platform.os !== "ios" && !IsMacOsNeBuild
        property var handler: function() {
            Qt.openUrlExternally(LanguageUiController.getCurrentSiteUrl())
        }
    }
}
