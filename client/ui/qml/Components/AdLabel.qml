import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import Qt5Compat.GraphicalEffects

import Style 1.0

import "../Config"
import "../Controls2"
import "../Controls2/TextTypes"

Rectangle {
    id: root

    property real contentHeight: ad.implicitHeight + ad.anchors.topMargin + ad.anchors.bottomMargin

    border.width: 1
    border.color: AmneziaStyle.color.goldenApricot
    color: AmneziaStyle.color.transparent
    radius: 13

    visible: false
    // visible: GC.isDesktop() && ServersModel.isDefaultServerFromApi
    //          && ServersModel.isDefaultServerDefaultContainerHasSplitTunneling && SettingsController.isHomeAdLabelVisible

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor

        onClicked: function() {
            Qt.openUrlExternally(LanguageModel.getCurrentSiteUrl() + "/premium")
        }
    }

    RowLayout {
        id: ad
        anchors.fill: parent
        anchors.margins: 16

        Image {
            source: "qrc:/images/controls/amnezia.svg"
            sourceSize: Qt.size(36, 36)

            layer {
                effect: ColorOverlay {
                    color: AmneziaStyle.color.paleGray
                }
            }
        }

        CaptionTextType {
            Layout.fillWidth: true
            Layout.rightMargin: 10
            Layout.leftMargin: 10

            text: qsTr("Amnezia Premium - for access to any website")
            color: AmneziaStyle.color.pearlGray

            lineHeight: 18
            font.pixelSize: 15
        }

        ImageButtonType {
            image: "qrc:/images/controls/close.svg"
            imageColor: AmneziaStyle.color.paleGray

            onClicked: function() {
                SettingsController.disableHomeAdLabel()
            }
        }
    }
}
