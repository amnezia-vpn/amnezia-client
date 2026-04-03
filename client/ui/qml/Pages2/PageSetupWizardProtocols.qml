import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
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
            ValueFilter { roleName: "serviceType"; value: ProtocolEnum.Vpn },
            ValueFilter { roleName: "isSupported"; value: true },
            ValueFilter { roleName: "isInstallationAllowed"; value: true }
        ]
        sorters: RoleSorter {
            roleName: "installPageOrder"
            sortOrder: Qt.AscendingOrder
        }
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
                spacing: 18

                BackButtonType {
                    Layout.topMargin: 20 + SettingsController.safeAreaTopMargin
                    Layout.leftMargin: 4
                }

                PremiumPanel {
                    Layout.fillWidth: true
                    accentVisible: true
                    accentColor: "#EAB308"

                    PremiumBadge {
                        text: qsTr("Шаг 1")
                        tone: "accent"
                        iconSource: "qrc:/images/controls/server.svg"
                    }

                    LabelTextType {
                        Layout.fillWidth: true
                        text: qsTr("Выберите сценарий подключения")
                        font.pixelSize: root.wideLayout ? 30 : 26
                        font.weight: 700
                        color: FBLinkStyle.color.paleGray
                        wrapMode: Text.WordWrap
                    }

                    LabelTextType {
                        Layout.fillWidth: true
                        text: qsTr("Новый setup flow оставляет только доступные на текущей платформе протоколы. На следующем шаге вы увидите только обязательные параметры, а детали останутся в раскрывающейся секции.")
                        wrapMode: Text.WordWrap
                        font.pixelSize: 14
                        color: FBLinkStyle.color.mutedGray
                    }
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.wideLayout ? 2 : 1
                    columnSpacing: 16
                    rowSpacing: 16

                    Repeater {
                        model: proxyContainersModel

                        delegate: PremiumPanel {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            accentVisible: index === 0
                            accentColor: index === 0 ? "#10B981" : "#EAB308"

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 10

                                PremiumBadge {
                                    text: index === 0 ? qsTr("Рекомендуем") : qsTr("Доступно")
                                    tone: index === 0 ? "success" : "accent"
                                    iconSource: index === 0 ? "qrc:/images/controls/shield-tick.svg" : "qrc:/images/controls/chevron-right.svg"
                                }

                                Item { Layout.fillWidth: true }
                            }

                            LabelTextType {
                                Layout.fillWidth: true
                                text: name
                                font.pixelSize: 22
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

                            BasicButtonType {
                                Layout.fillWidth: true
                                text: qsTr("Продолжить")
                                defaultColor: index === 0 ? "#10B981" : Qt.rgba(1, 1, 1, 0.08)
                                hoveredColor: index === 0 ? "#13C88E" : Qt.rgba(1, 1, 1, 0.12)
                                pressedColor: index === 0 ? "#0D9B6E" : Qt.rgba(1, 1, 1, 0.18)
                                textColor: "#FFFFFF"
                                rightImageSource: "qrc:/images/controls/chevron-right.svg"
                                clickedFunc: function() {
                                    ContainersModel.setProcessedContainerIndex(proxyContainersModel.mapToSource(index))
                                    PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                                }
                            }
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
