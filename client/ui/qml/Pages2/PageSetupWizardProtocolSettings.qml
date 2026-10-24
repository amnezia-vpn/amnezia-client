import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import ProtocolProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property Item settingsItem: null

    SortFilterProxyModel {
        id: proxyContainersModel
        sourceModel: ContainersModel
        filters: [
            ValueFilter {
                roleName: "isCurrentlyProcessed"
                value: true
            }
        ]
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: bottomBar.top

        contentHeight: content.implicitHeight + 16

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            Repeater {
                model: proxyContainersModel

                delegate: ColumnLayout {
                    id: delegateRoot

                    required property string name
                    required property string description
                    required property string detailedDescription
                    required property int dockerContainer
                    required property bool isTProxy

                    function install() {
                        if (isTProxy) {
                            tproxyHostname.errorText = ""
                            tproxyEmail.errorText = ""

                            var host = TProxyConfigModel.sanitizeHostnameFieldText(tproxyHostname.textField.text)
                            var email = TProxyConfigModel.sanitizeAcmeEmailFieldText(tproxyEmail.textField.text)
                            tproxyHostname.textField.text = host
                            tproxyEmail.textField.text = email

                            var hasError = false

                            if (!TProxyConfigModel.isValidHostname(host)) {
                                tproxyHostname.errorText = qsTr("Enter a lowercase DNS hostname")
                                hasError = true
                            }
                            if (!TProxyConfigModel.isValidAcmeEmail(email)) {
                                tproxyEmail.errorText = qsTr("Enter a valid email for the TLS certificate")
                                hasError = true
                            }
                            if (hasError) {
                                return
                            }

                            port.textField.text = TProxyConfigModel.defaultPort()
                            TProxyConfigModel.setHostname(host)
                            TProxyConfigModel.setAcmeEmail(email)
                            TProxyConfigModel.setPort(TProxyConfigModel.defaultPort())
                            TProxyConfigModel.setHttpPort(TProxyConfigModel.defaultHttpPort())
                        } else if (!port.textField.acceptableInput &&
                                ContainerProps.containerTypeToString(dockerContainer) !== "torwebsite" &&
                                ContainerProps.containerTypeToString(dockerContainer) !== "ikev2") {
                            port.errorText = qsTr("The port must be in the range of 1 to 65535")
                            return
                        }

                        PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                        InstallController.install(dockerContainer, port.textField.text, transportProtoSelector.currentIndex, ServersUiController.processedServerId)
                    }

                    Layout.fillWidth: true

                    spacing: 0

                    NavigationBarType {
                        Layout.fillWidth: true

                        title: qsTr("Installing %1").arg(delegateRoot.name)
                        description: delegateRoot.description
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        spacing: 12

                        ButtonType {
                            id: showDetailsButton

                            Layout.alignment: Qt.AlignLeft

                            variant: ButtonType.Text
                            size: ButtonType.Medium

                            text: qsTr("More detailed")

                            onClicked: {
                                showDetailsDrawer.open()
                            }
                        }

                        BodyTextType {
                            id: transportProtoHeader

                            Layout.fillWidth: true

                            text: qsTr("Network protocol")
                            color: AmneziaStyle.color.textTertiary
                        }

                        TransportProtoSelector {
                            id: transportProtoSelector

                            Layout.fillWidth: true
                        }

                        TextFieldType {
                            id: port

                            Layout.fillWidth: true

                            visible: !delegateRoot.isTProxy

                            headerText: qsTr("Port")
                            textField.maximumLength: 5
                            textField.validator: IntValidator { bottom: 1; top: 65535 }
                        }

                        TextFieldType {
                            id: tproxyHostname

                            Layout.fillWidth: true

                            visible: delegateRoot.isTProxy

                            headerText: qsTr("Hostname")
                            textField.placeholderText: qsTr("proxy.example.com")
                            textField.onTextChanged: {
                                var cur = tproxyHostname.textField.text
                                var clean = TProxyConfigModel.sanitizeHostnameFieldText(cur)
                                if (clean !== cur) {
                                    textField.text = clean
                                    textField.cursorPosition = clean.length
                                    return
                                }
                                if (TProxyConfigModel.isHostnameTypingIncomplete(cur)) {
                                    tproxyHostname.errorText = ""
                                    return
                                }
                                if (!TProxyConfigModel.isValidHostname(clean)) {
                                    tproxyHostname.errorText = qsTr("Use lowercase letters, digits, dots and hyphens")
                                    return
                                }
                                tproxyHostname.errorText = ""
                            }
                        }

                        TextFieldType {
                            id: tproxyEmail

                            Layout.fillWidth: true

                            visible: delegateRoot.isTProxy

                            headerText: qsTr("ACME email")
                            textField.placeholderText: qsTr("you@example.com")
                            textField.onTextChanged: {
                                var cur = tproxyEmail.textField.text
                                var clean = TProxyConfigModel.sanitizeAcmeEmailFieldText(cur)
                                if (clean !== cur) {
                                    textField.text = clean
                                    textField.cursorPosition = clean.length
                                    return
                                }
                                if (TProxyConfigModel.isAcmeEmailTypingIncomplete(cur)) {
                                    tproxyEmail.errorText = ""
                                    return
                                }
                                if (!TProxyConfigModel.isValidAcmeEmail(clean)) {
                                    tproxyEmail.errorText = qsTr("Enter a valid email for the TLS certificate")
                                    return
                                }
                                tproxyEmail.errorText = ""
                            }
                        }

                        CaptionTextType {
                            Layout.fillWidth: true

                            visible: delegateRoot.isTProxy

                            text: qsTr("Needs a DNS A record and free, internet-reachable ports 443 (HTTPS) and 80 (ACME). If either port is busy or blocked, the proxy will not work. Classic Telegram MTProxy links will not work.")
                            color: AmneziaStyle.color.textAccent
                        }
                    }

                    SheetType {
                        id: showDetailsDrawer

                        parent: root

                        expandedHeight: windowHeight * 0.9

                        Loader {
                            anchors.fill: parent

                            active: showDetailsDrawer.visible

                            sourceComponent: Item {
                                Item {
                                    id: showDetailsNavigation

                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.right: parent.right

                                    height: 48

                                    IconButtonType {
                                        x: 4
                                        y: 0
                                        icon.source: "qrc:/images/controls/arrow-left.svg"
                                        Accessible.name: qsTr("Back")

                                        onClicked: showDetailsDrawer.close()
                                    }
                                }

                                FlickableType {
                                    id: showDetailsFlickable

                                    anchors.top: showDetailsNavigation.bottom
                                    anchors.bottom: showDetailsCloseBar.top

                                    contentHeight: showDetailsContent.implicitHeight + 28

                                    ColumnLayout {
                                        id: showDetailsContent

                                        anchors.top: parent.top
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.topMargin: 12
                                        anchors.leftMargin: 16
                                        anchors.rightMargin: 16

                                        spacing: 8

                                        SectionHeaderType {
                                            Layout.fillWidth: true

                                            headerText: delegateRoot.name
                                        }

                                        BodyTextType {
                                            Layout.fillWidth: true

                                            text: delegateRoot.detailedDescription
                                            textFormat: Text.MarkdownText
                                        }
                                    }
                                }

                                BottomBarType {
                                    id: showDetailsCloseBar

                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom

                                    color: AmneziaStyle.color.transparent

                                    ButtonType {
                                        Layout.fillWidth: true

                                        text: qsTr("Close")

                                        onClicked: showDetailsDrawer.close()
                                    }
                                }
                            }
                        }
                    }

                    Component.onCompleted: {
                        root.settingsItem = delegateRoot

                        var defaultContainerProto = ContainerProps.defaultProtocol(dockerContainer)

                        if (InstallController.defaultPort(defaultContainerProto) < 0) {
                            port.visible = false
                        } else {
                            port.textField.text = InstallController.getPortForInstall(defaultContainerProto)
                        }
                        transportProtoSelector.currentIndex = InstallController.defaultTransportProto(defaultContainerProto)

                        port.enabled = InstallController.defaultPortChangeable(defaultContainerProto)
                        var protocolSelectorVisible = InstallController.defaultTransportProtoChangeable(defaultContainerProto)
                        transportProtoSelector.visible = protocolSelectorVisible
                        transportProtoHeader.visible = protocolSelectorVisible
                    }
                }
            }
        }
    }

    BottomBarType {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        ButtonType {
            id: installButton

            Layout.fillWidth: true

            enabled: root.settingsItem !== null

            text: qsTr("Install")

            onClicked: {
                if (root.settingsItem) {
                    root.settingsItem.install()
                }
            }
        }
    }
}
