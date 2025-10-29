import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes

import Style 1.0

import "../Config"
import "../Controls2"
import "../Controls2/TextTypes"

Rectangle {
    id: root

    property real contentHeight: content.implicitHeight + content.anchors.topMargin + content.anchors.bottomMargin

    gradient: Gradient {
        orientation: Gradient.Horizontal
        GradientStop { position: 0.0; color: Qt.rgba(85/255, 86/255, 92/255, 0.13) }
        GradientStop { position: 1.0; color: Qt.rgba(28/255, 29/255, 33/255, 0.13) }
    }
    border.width: 1
    border.color: "#1C1D21"
    radius: 13

    visible: true
    // visible: GC.isDesktop() && ServersModel.isDefaultServerFromApi
    //          && ServersModel.isDefaultServerDefaultContainerHasSplitTunneling && SettingsController.isHomeAdLabelVisible

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onEntered: {
            chevronBackground.color = AmneziaStyle.color.slateGray
        }

        onExited: {
            chevronBackground.color = AmneziaStyle.color.transparent
        }

        onPressedChanged: {
            chevronBackground.color = pressed ? AmneziaStyle.color.charcoalGray : containsMouse ? AmneziaStyle.color.slateGray : AmneziaStyle.color.transparent
        }

        onClicked: function() {
            Qt.openUrlExternally(LanguageModel.getCurrentSiteUrl("premium"))
        }
    }

    RowLayout {
        id: content
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 12
        anchors.topMargin: 12
        anchors.bottomMargin: 12
        spacing: 20

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                spacing: 4

                CaptionTextType {
                    text: qsTr("Try Amnezia")
                    color: AmneziaStyle.color.paleGray
                    font.pixelSize: 14
                    font.weight: 700
                }

                CaptionTextType {
                    text: "Premium"
                    color: "#950051"
                    font.pixelSize: 14
                    font.weight: 700
                }
            }

            CaptionTextType {
                Layout.fillWidth: true
                text: qsTr("High speed and 20 countries to connect. 7 days free")
                color: AmneziaStyle.color.mutedGray
                wrapMode: Text.WordWrap
                lineHeight: 18
                lineHeightMode: Text.FixedHeight
                font.pixelSize: 14
            }
        }

        Item {
            implicitWidth: 40
            implicitHeight: 40
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                id: chevronBackground
                anchors.fill: parent
                radius: 12
                color: AmneziaStyle.color.transparent

                Behavior on color {
                    PropertyAnimation { duration: 200 }
                }
            }

            Image {
                anchors.centerIn: parent
                source: "qrc:/images/controls/chevron-right.svg"
                sourceSize: Qt.size(24, 24)
            }
        }
    }
}
