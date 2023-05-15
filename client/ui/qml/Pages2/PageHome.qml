import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

Item {
    id: root

    property string defaultColor: "#1C1D21"

    property string borderColor: "#2C2D30"

    property string currentServerName: serversMenuContent.currentItem.delegateData.desc
    property string currentServerDescription: serversMenuContent.currentItem.delegateData.address

    ConnectButton {
        anchors.centerIn: parent
    }

    Rectangle {
        id: buttonBackground
        anchors.fill: buttonContent
        anchors.bottomMargin: -radius

        radius: 16
        color: defaultColor
        border.color: borderColor
        border.width: 1

        Rectangle {
            width: parent.width
            height: 1
            y: parent.height - height - parent.radius

            color: borderColor
        }
    }

    ColumnLayout {
        id: buttonContent
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        RowLayout {
            Layout.topMargin: 24
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            Header1TextType {
                text: currentServerName
            }

            Image {
                Layout.preferredWidth: 18
                Layout.preferredHeight: 18

                source: "qrc:/images/controls/chevron-down.svg"
            }
        }

        LabelTextType {
            Layout.bottomMargin: 44
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

            text: currentServerDescription
        }
    }

    MouseArea {
        anchors.fill: buttonBackground
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onClicked: {
            menu.visible = true
        }
    }

    Drawer {
        id: menu

        edge: Qt.BottomEdge
        width: parent.width
        height: parent.height * 0.90

        clip: true
        modal: true

        background: Rectangle {
            anchors.fill: parent
            anchors.bottomMargin: -radius
            radius: 16

            color: "#1C1D21"
            border.color: borderColor
            border.width: 1
        }

        Overlay.modal: Rectangle {
            color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
        }

        ColumnLayout {
            id: serversMenuHeader
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.left: parent.left

            Header1TextType {
                Layout.topMargin: 24
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                text: currentServerName
            }

            LabelTextType {
                Layout.bottomMargin: 24
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

                text: currentServerDescription
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                spacing: 8

                SortFilterProxyModel {
                    id: proxyContainersModel
                    sourceModel: ContainersModel
                    filters: [
                        ValueFilter {
                            roleName: "serviceType"
                            value: ProtocolEnum.Vpn
                        }
                    ]
                }

                DropDownType {
                    id: containersDropDown

                    implicitHeight: 40

                    borderWidth: 0
                    buttonImageColor: "#0E0E11"
                    buttonMaximumWidth: 150 //todo make it dynamic

                    defaultColor: "#D7D8DB"

                    textColor: "#0E0E11"
                    headerText: "Протокол подключения"
                    headerBackButtonImage: "qrc:/images/controls/arrow-left.svg"

                    menuModel: proxyContainersModel

                    ButtonGroup {
                        id: containersRadioButtonGroup
                    }

                    menuDelegate: Item {
                        implicitWidth: root.width
                        implicitHeight: containerRadioButton.implicitHeight

                        RadioButton {
                            id: containerRadioButton

                            implicitWidth: parent.width
                            implicitHeight: containerRadioButtonContent.implicitHeight

                            hoverEnabled: true

                            ButtonGroup.group: containersRadioButtonGroup

                            checked: {
                                if (modelData !== null) {
                                    return modelData.isDefault
                                }
                                return false
                            }

                            indicator: Rectangle {
                                anchors.fill: parent
                                color: containerRadioButton.hovered ? "#2C2D30" : "#1C1D21"

                                Behavior on color {
                                    PropertyAnimation { duration: 200 }
                                }
                            }

                            checkable: {
                                if (modelData !== null) {
                                    if (modelData.isInstalled) {
                                        return true
                                    }
                                }
                                return false
                            }

                            RowLayout {
                                id: containerRadioButtonContent
                                anchors.fill: parent

                                anchors.rightMargin: 16
                                anchors.leftMargin: 16

                                z: 1

                                Image {
                                    source: {
                                        if (modelData !== null) {
                                            if (modelData.isInstalled) {
                                                return "qrc:/images/controls/check.svg"
                                            }
                                        }
                                        return "qrc:/images/controls/download.svg"
                                    }
                                    visible: {
                                        if (modelData !== null) {
                                            if (modelData.isInstalled) {
                                                return containerRadioButton.checked
                                            }
                                        }
                                        return true
                                    }

                                    width: 24
                                    height: 24

                                    Layout.rightMargin: 8
                                }

                                Text {
                                    id: containerRadioButtonText

                                    text: {
                                        if (modelData !== null) {
                                            return modelData.name
                                        } else
                                            return ""
                                    }
                                    color: "#D7D8DB"
                                    font.pixelSize: 16
                                    font.weight: 400
                                    font.family: "PT Root UI VF"

                                    height: 24

                                    Layout.fillWidth: true
                                    Layout.topMargin: 20
                                    Layout.bottomMargin: 20
                                }
                            }

                            onClicked: {
                                if (checked) {
                                    modelData.isDefault = true

                                    containersDropDown.text = containerRadioButtonText.text
                                    containersDropDown.menuVisible = false
                                } else {
                                    ContainersModel.setCurrentlyInstalledContainerIndex(proxyContainersModel.mapToSource(delegateIndex))
                                    PageController.goToPage(PageEnum.PageSetupWizardProtocolSettings)
                                    containersDropDown.menuVisible = false
                                    menu.visible = false
                                }
                            }

                            MouseArea {
                                anchors.fill: containerRadioButton
                                cursorShape: Qt.PointingHandCursor
                                enabled: false
                            }
                        }

                        Component.onCompleted: {
                            if (modelData !== null && modelData.isDefault) {
                                containersDropDown.text = modelData.name
                            }
                        }
                    }
                }

                BasicButtonType {
                    id: dnsButton

                    implicitHeight: 40

                    text: "Amnezia DNS"
                }
            }

            Header2Type {
                Layout.fillWidth: true
                Layout.topMargin: 48
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                actionButtonImage: "qrc:/images/controls/plus.svg"

                headerText: "Серверы"
            }
        }

        FlickableType {
            anchors.top: serversMenuHeader.bottom
            anchors.topMargin: 16
            contentHeight: col.implicitHeight

            Column {
                id: col
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                spacing: 16

                ButtonGroup {
                    id: serversRadioButtonGroup
                }

                ListView {
                    id: serversMenuContent
                    width: parent.width
                    height: serversMenuContent.contentItem.height

                    model: ServersModel
                    currentIndex: ServersModel.getDefaultServerIndex()

                    clip: true

                    delegate: Item {
                        id: menuContentDelegate

                        property variant delegateData: model

                        implicitWidth: serversMenuContent.width
                        implicitHeight: serverRadioButton.implicitHeight

                        RadioButton {
                            id: serverRadioButton

                            implicitWidth: parent.width
                            implicitHeight: serverRadioButtonContent.implicitHeight

                            hoverEnabled: true

                            checked: index === serversMenuContent.currentIndex

                            ButtonGroup.group: serversRadioButtonGroup

                            indicator: Rectangle {
                                anchors.fill: parent
                                color: serverRadioButton.hovered ? "#2C2D30" : "#1C1D21"

                                Behavior on color {
                                    PropertyAnimation { duration: 200 }
                                }
                            }

                            RowLayout {
                                id: serverRadioButtonContent
                                anchors.fill: parent

                                anchors.rightMargin: 16
                                anchors.leftMargin: 16

                                z: 1

                                Text {
                                    id: serverRadioButtonText

                                    text: desc
                                    color: "#D7D8DB"
                                    font.pixelSize: 16
                                    font.weight: 400
                                    font.family: "PT Root UI VF"

                                    height: 24

                                    Layout.fillWidth: true
                                    Layout.topMargin: 20
                                    Layout.bottomMargin: 20
                                }

                                Image {
                                    source: "qrc:/images/controls/check.svg"
                                    visible: serverRadioButton.checked
                                    width: 24
                                    height: 24

                                    Layout.rightMargin: 8
                                }
                            }

                            onClicked: {
                                root.currentServerName = desc
                                root.currentServerDescription = address

                                ServersModel.setDefaultServerIndex(index)
                                ContainersModel.setSelectedServerIndex(index)
                            }

                            MouseArea {
                                anchors.fill: serverRadioButton
                                cursorShape: Qt.PointingHandCursor
                                enabled: false
                            }
                        }
                    }
                }
            }
        }
    }
}
