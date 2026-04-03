import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    function guessCountryCode(serverName) {
        const name = (serverName || "").toLowerCase()
        if (name.indexOf("амстердам") >= 0 || name.indexOf("amsterdam") >= 0) return "NL"
        if (name.indexOf("charlotte") >= 0 || name.indexOf("шарлотт") >= 0) return "US"
        if (name.indexOf("москва") >= 0 || name.indexOf("moscow") >= 0) return "RU"
        if (name.indexOf("frankfurt") >= 0 || name.indexOf("франкфурт") >= 0) return "DE"
        if (name.indexOf("warsaw") >= 0 || name.indexOf("варшава") >= 0) return "PL"
        if (name.indexOf("istanbul") >= 0 || name.indexOf("стамбул") >= 0) return "TR"
        if (name.indexOf("london") >= 0 || name.indexOf("лондон") >= 0) return "GB"
        return ""
    }

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin

        BackButtonType {
            id: backButton
            visible: root.stackView ? root.stackView.depth > 1 : true
        }

        BaseHeaderType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            headerText: qsTr("Локации")
        }
    }

    ListViewType {
        id: servers
        objectName: "servers"

        width: parent.width
        anchors.top: header.bottom
        anchors.topMargin: 16
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right


        model: ServersModel

        delegate: Item {
            implicitWidth: servers.width
            implicitHeight: delegateContent.implicitHeight

            ColumnLayout {
                id: delegateContent

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                Rectangle {
                    id: serverCard
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    implicitHeight: 72
                    radius: 12

                    readonly property bool selected: index === ServersModel.defaultIndex
                    readonly property string resolvedCountryCode: apiServerCountryCode !== ""
                        ? apiServerCountryCode
                        : root.guessCountryCode(name)

                    color: selected
                        ? Qt.rgba(234/255, 179/255, 8/255, 0.10)
                        : (serverMouseArea.containsMouse ? Qt.rgba(38/255, 38/255, 38/255, 0.9) : "transparent")
                    border.width: selected ? 1 : 0
                    border.color: selected ? Qt.rgba(234/255, 179/255, 8/255, 0.60) : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 36
                            Layout.preferredHeight: 36
                            radius: 8
                            color: Qt.rgba(255/255, 255/255, 255/255, 0.04)
                            border.width: 1
                            border.color: Qt.rgba(255/255, 255/255, 255/255, 0.08)

                            Image {
                                anchors.centerIn: parent
                                source: serverCard.resolvedCountryCode !== ""
                                    ? "qrc:/countriesFlags/images/flagKit/" + serverCard.resolvedCountryCode + ".svg"
                                    : "qrc:/images/controls/map-pin.svg"
                                sourceSize: serverCard.resolvedCountryCode !== "" ? Qt.size(24, 18) : Qt.size(20, 20)
                                fillMode: Image.PreserveAspectFit
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            LabelTextType {
                                Layout.fillWidth: true
                                text: name
                                font.pixelSize: 17
                                font.weight: serverCard.selected ? 700 : 500
                                color: "#F4F4F5"
                                elide: Text.ElideRight
                            }

                            CaptionTextType {
                                Layout.fillWidth: true
                                text: hostName !== "" ? hostName : serverDescription
                                color: FBLinkStyle.color.mutedGray
                                elide: Text.ElideRight
                            }
                        }

                        Image {
                            Layout.alignment: Qt.AlignVCenter
                            source: serverCard.selected
                                ? "qrc:/images/controls/check.svg"
                                : "qrc:/images/controls/chevron-right.svg"
                            sourceSize: Qt.size(18, 18)
                            opacity: serverCard.selected ? 1.0 : 0.7
                        }
                    }

                    MouseArea {
                        id: serverMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (ConnectionController.isConnected) {
                                PageController.showNotificationMessage(qsTr("Нельзя менять сервер во время активного подключения"))
                                return
                            }
                            ServersModel.processedIndex = index
                            ServersModel.defaultIndex = index
                            PageController.showNotificationMessage(qsTr("Локация выбрана"))
                        }
                    }
                }

                DividerType {}
            }
        }
    }
}
