import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0

import "../Controls2/TextTypes"

Item {
    id: root

    property bool isSearchResult: true

    signal showAllRequested()

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 320)

        spacing: 16

        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 48
            Layout.preferredHeight: 48

            Image {
                id: searchIcon

                anchors.fill: parent
                source: "qrc:/images/controls/search.svg"
                visible: false
            }

            ColorOverlay {
                anchors.fill: searchIcon
                source: searchIcon
                color: AmneziaStyle.color.goldenApricot
            }
        }

        ParagraphTextType {
            Layout.fillWidth: true

            color: AmneziaStyle.color.textPrimary
            horizontalAlignment: Text.AlignHCenter

            text: root.isSearchResult ? qsTr("Not found. Try a different spelling")
                                      : qsTr("No locations available yet")
        }

        ParagraphTextType {
            Layout.alignment: Qt.AlignHCenter

            visible: root.isSearchResult

            color: AmneziaStyle.color.goldenApricot
            horizontalAlignment: Text.AlignHCenter

            text: qsTr("Show all")

            MouseArea {
                anchors.fill: parent
                anchors.margins: -8
                cursorShape: Qt.PointingHandCursor
                onClicked: root.showAllRequested()
            }
        }
    }
}
