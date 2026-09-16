import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2/TextTypes"

Item {
    id: root

    property var listModel: null

    implicitHeight: 56

    Rectangle {
        anchors.fill: parent

        color: AmneziaStyle.color.surfaceBase
        radius: 16

        RowLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 4

            Repeater {
                model: [
                    { label: qsTr("All"), filter: 0 },
                    { label: qsTr("Allowlist"), filter: 1 }
                ]

                delegate: Rectangle {
                    id: tab

                    required property var modelData

                    readonly property bool selected: root.listModel !== null
                                                     && root.listModel.tabFilter === tab.modelData.filter

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    radius: 12
                    color: AmneziaStyle.color.transparent

                    border.width: tab.selected ? 1 : 0
                    border.color: AmneziaStyle.color.goldenApricot

                    Behavior on border.color {
                        PropertyAnimation { duration: 200 }
                    }

                    ParagraphTextType {
                        anchors.fill: parent

                        color: tab.selected ? AmneziaStyle.color.textPrimary : AmneziaStyle.color.textTertiary

                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter

                        text: tab.modelData.label
                    }

                    HoverHandler {
                        cursorShape: Qt.PointingHandCursor
                    }

                    TapHandler {
                        gesturePolicy: TapHandler.ReleaseWithinBounds

                        onTapped: {
                            if (root.listModel !== null) {
                                root.listModel.tabFilter = tab.modelData.filter
                            }
                        }
                    }
                }
            }
        }
    }
}
