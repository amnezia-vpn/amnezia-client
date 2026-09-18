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

        Item {
            id: showAll

            property bool isFocusable: root.isSearchResult

            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: showAllText.implicitWidth + 16
            Layout.preferredHeight: showAllText.implicitHeight + 16

            visible: root.isSearchResult

            Keys.onEnterPressed: root.showAllRequested()
            Keys.onReturnPressed: root.showAllRequested()
            Keys.onSpacePressed: root.showAllRequested()

            Keys.onTabPressed: {
                FocusController.nextKeyTabItem()
            }

            Keys.onBacktabPressed: {
                FocusController.previousKeyTabItem()
            }

            Rectangle {
                anchors.fill: parent

                color: AmneziaStyle.color.transparent
                radius: 8

                border.width: showAll.activeFocus ? 1 : 0
                border.color: AmneziaStyle.color.paleGray
            }

            ParagraphTextType {
                id: showAllText

                anchors.centerIn: parent

                color: AmneziaStyle.color.goldenApricot
                horizontalAlignment: Text.AlignHCenter

                text: qsTr("Show all")
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.showAllRequested()
            }
        }
    }
}
