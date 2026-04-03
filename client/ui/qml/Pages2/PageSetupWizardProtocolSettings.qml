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

    readonly property bool wideLayout: GC.isWideWidth(width)
    readonly property real sideMargin: GC.pageHorizontalMargin(width)
    readonly property real maxContentWidth: GC.pageMaxWidth(width)

    SortFilterProxyModel {
        id: proxyContainersModel
        sourceModel: ContainersModel
        filters: [
            ValueFilter { roleName: "isCurrentlyProcessed"; value: true }
        ]
    }

    Flickable {
        anchors.fill: parent
        contentHeight: content.implicitHeight + 28
        clip: true

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Item {
            width: parent.width
            height: content.implicitHeight + 28

            ColumnLayout {
                id: content
                width: Math.min(root.maxContentWidth, parent.width - root.sideMargin * 2)
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                spacing: 12

                BackButtonType {
                    Layout.topMargin: 16 + SettingsController.safeAreaTopMargin
                    Layout.leftMargin: 4
                }

                Repeater {
                    model: proxyContainersModel

                    delegate: GridLayout {
                        Layout.fillWidth: true
                        columns: root.wideLayout ? 2 : 1
                        columnSpacing: 18
                        rowSpacing: 18

                        PremiumPanel {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            visible: root.wideLayout
                            accentVisible: true
                            accentColor: "#EAB308"

                            PremiumBadge {
                                text: qsTr("Шаг 2")
                                tone: "accent"
                                iconSource: "qrc:/images/controls/file-cog-2.svg"
                            }

                            LabelTextType {
                                Layout.fillWidth: true
                                text: qsTr("Установка %1").arg(name)
                                font.pixelSize: root.wideLayout ? 30 : 26
                                font.weight: 700
                                color: FBLinkStyle.color.paleGray
                                wrapMode: Text.WordWrap
                            }

                            LabelTextType {
                                Layout.fillWidth: true
                                text: description
                                font.pixelSize: 14
                                color: FBLinkStyle.color.mutedGray
                                wrapMode: Text.WordWrap
                            }

                            PremiumPanel {
                                Layout.fillWidth: true
                                fillColor: Qt.rgba(1, 1, 1, 0.03)
                                outlineColor: Qt.rgba(1, 1, 1, 0.06)
                                padding: 14

                                PremiumBadge {
                                    text: qsTr("Кратко")
                                    tone: "success"
                                }

                                LabelTextType {
                                    Layout.fillWidth: true
                                    text: qsTr("Сначала выберите обязательные параметры. Детали и описание протокола доступны ниже по кнопке.")
                                    wrapMode: Text.WordWrap
                                    color: FBLinkStyle.color.lightGray
                                    font.pixelSize: 13
                                }
                            }

                            BasicButtonType {
                                id: showDetailsButton
                                Layout.fillWidth: true
                                implicitHeight: 42
                                text: qsTr("Подробнее о %1").arg(name)
                                leftImageSource: "qrc:/images/controls/info.svg"
                                defaultColor: Qt.rgba(1, 1, 1, 0.08)
                                hoveredColor: Qt.rgba(1, 1, 1, 0.12)
                                pressedColor: Qt.rgba(1, 1, 1, 0.18)
                                textColor: FBLinkStyle.color.paleGray
                                clickedFunc: function() {
                                    showDetailsDrawer.openTriggered()
                                }
                            }
                        }

                        PremiumPanel {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            accentVisible: true
                            accentColor: "#10B981"
                            padding: 12

                            PremiumBadge {
                                visible: !root.wideLayout
                                text: qsTr("Шаг 2")
                                tone: "accent"
                                iconSource: "qrc:/images/controls/file-cog-2.svg"
                            }

                            LabelTextType {
                                visible: !root.wideLayout
                                Layout.fillWidth: true
                                text: qsTr("Установка %1").arg(name)
                                font.pixelSize: 24
                                font.weight: 700
                                color: FBLinkStyle.color.paleGray
                                wrapMode: Text.WordWrap
                            }

                            LabelTextType {
                                visible: !root.wideLayout
                                Layout.fillWidth: true
                                text: description
                                font.pixelSize: 12
                                color: FBLinkStyle.color.mutedGray
                                wrapMode: Text.WordWrap
                            }

                            LabelTextType {
                                Layout.fillWidth: true
                                text: qsTr("Базовые параметры")
                                font.pixelSize: 20
                                font.weight: 700
                                color: FBLinkStyle.color.paleGray
                            }

                            LabelTextType {
                                Layout.fillWidth: true
                                visible: root.wideLayout
                                text: qsTr("Сначала только обязательные поля. Подробности протокола открываются отдельно и не перегружают первый экран.")
                                wrapMode: Text.WordWrap
                                font.pixelSize: 13
                                color: FBLinkStyle.color.mutedGray
                            }

                            ParagraphTextType {
                                id: transportProtoHeader
                                text: qsTr("Сетевой протокол")
                                visible: transportProtoSelector.visible
                            }

                            TransportProtoSelector {
                                id: transportProtoSelector
                                Layout.fillWidth: true
                                rootWidth: root.width
                            }

                            TextFieldWithHeaderType {
                                id: port
                                Layout.fillWidth: true
                                headerText: qsTr("Port")
                                textField.maximumLength: 5
                                textField.validator: IntValidator { bottom: 1; top: 65535 }
                            }

                            PremiumPanel {
                                Layout.fillWidth: true
                                visible: root.wideLayout
                                fillColor: Qt.rgba(1, 1, 1, 0.03)
                                outlineColor: Qt.rgba(1, 1, 1, 0.06)
                                padding: 14

                                LabelTextType {
                                    Layout.fillWidth: true
                                    text: qsTr("После установки вы сможете вернуться и открыть детальные параметры протокола уже из карточки сервера.")
                                    wrapMode: Text.WordWrap
                                    color: FBLinkStyle.color.lightGray
                                    font.pixelSize: 13
                                }
                            }

                            BasicButtonType {
                                id: installButton
                                Layout.fillWidth: true
                                text: qsTr("Установить")
                                defaultColor: "#EAB308"
                                hoveredColor: "#FACC15"
                                pressedColor: "#CA8A04"
                                textColor: "#FFFFFF"
                                clickedFunc: function() {
                                    if (!port.textField.acceptableInput &&
                                            ContainerProps.containerTypeToString(dockerContainer) !== "torwebsite" &&
                                            ContainerProps.containerTypeToString(dockerContainer) !== "ikev2") {
                                        port.errorText = qsTr("The port must be in the range of 1 to 65535")
                                        return
                                    }

                                    PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                                    InstallController.install(dockerContainer, port.textField.text, transportProtoSelector.currentIndex)
                                }
                            }

                            BasicButtonType {
                                visible: !root.wideLayout
                                Layout.fillWidth: true
                                implicitHeight: 42
                                text: qsTr("Подробнее о %1").arg(name)
                                leftImageSource: "qrc:/images/controls/info.svg"
                                defaultColor: Qt.rgba(1, 1, 1, 0.08)
                                hoveredColor: Qt.rgba(1, 1, 1, 0.12)
                                pressedColor: Qt.rgba(1, 1, 1, 0.18)
                                textColor: FBLinkStyle.color.paleGray
                                clickedFunc: function() {
                                    showDetailsDrawer.openTriggered()
                                }
                            }
                        }

                        DrawerType2 {
                            id: showDetailsDrawer
                            parent: root
                            anchors.fill: parent
                            expandedHeight: parent.height * 0.9
                            expandedStateContent: Item {
                                implicitHeight: showDetailsDrawer.expandedHeight

                                BackButtonType {
                                    id: showDetailsBackButton
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.topMargin: 16
                                    backButtonFunction: function() {
                                        showDetailsDrawer.closeTriggered()
                                    }
                                }

                                Flickable {
                                    anchors.top: showDetailsBackButton.bottom
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    contentHeight: drawerContent.implicitHeight + 32
                                    clip: true

                                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                    Item {
                                        width: parent.width
                                        height: drawerContent.implicitHeight + 32

                                        ColumnLayout {
                                            id: drawerContent
                                            width: Math.min(root.maxContentWidth, parent.width - root.sideMargin * 2)
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            anchors.top: parent.top
                                            anchors.topMargin: 12
                                            spacing: 16

                                            PremiumPanel {
                                                Layout.fillWidth: true
                                                accentVisible: true
                                                accentColor: "#EAB308"

                                                LabelTextType {
                                                    Layout.fillWidth: true
                                                    text: name
                                                    font.pixelSize: 24
                                                    font.weight: 700
                                                    color: FBLinkStyle.color.paleGray
                                                }

                                                ParagraphTextType {
                                                    Layout.fillWidth: true
                                                    text: detailedDescription
                                                    textFormat: Text.MarkdownText
                                                    wrapMode: Text.WordWrap
                                                }

                                                BasicButtonType {
                                                    Layout.fillWidth: true
                                                    text: qsTr("Закрыть")
                                                    clickedFunc: function() {
                                                        showDetailsDrawer.closeTriggered()
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Component.onCompleted: {
                            var defaultContainerProto = ContainerProps.defaultProtocol(dockerContainer)

                            if (ProtocolProps.defaultPort(defaultContainerProto) < 0) {
                                port.visible = false
                            } else {
                                port.textField.text = ProtocolProps.getPortForInstall(defaultContainerProto)
                            }
                            transportProtoSelector.currentIndex = ProtocolProps.defaultTransportProto(defaultContainerProto)

                            port.enabled = ProtocolProps.defaultPortChangeable(defaultContainerProto)
                            var protocolSelectorVisible = ProtocolProps.defaultTransportProtoChangeable(defaultContainerProto)
                            transportProtoSelector.visible = protocolSelectorVisible
                            transportProtoHeader.visible = protocolSelectorVisible
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
