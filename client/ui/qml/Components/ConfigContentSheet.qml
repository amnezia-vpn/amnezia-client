import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

SheetType {
    id: root

    property string headerText
    property string contentText

    readonly property alias textArea: contentArea

    expandedHeight: windowHeight * 0.9

    Item {
        anchors.fill: parent

        IconButtonType {
            id: backButton

            x: 16
            y: 8

            size: IconButtonType.Small
            icon.source: "qrc:/images/controls/arrow-left.svg"

            onClicked: root.close()
        }

        FlickableType {
            id: flickable

            anchors.top: backButton.bottom
            anchors.topMargin: 4
            anchors.bottom: parent.bottom

            contentHeight: content.implicitHeight

            ColumnLayout {
                id: content

                width: flickable.width

                spacing: 0

                SectionHeaderType {
                    Layout.fillWidth: true
                    Layout.topMargin: 24
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    headerText: root.headerText
                }

                TextArea {
                    id: contentArea

                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.bottomMargin: 16 + PageController.safeAreaBottomMargin

                    padding: 0
                    leftPadding: 0
                    rightPadding: 0
                    leftInset: 0
                    rightInset: 0
                    readOnly: true
                    activeFocusOnTab: false

                    color: AmneziaStyle.color.textSecondary
                    selectionColor: AmneziaStyle.color.bgStateLayerAccent
                    selectedTextColor: AmneziaStyle.color.textSecondary

                    font.pixelSize: 16
                    font.weight: 400
                    font.family: "PT Root UI VF"
                    font.letterSpacing: -0.4

                    text: root.contentText
                    wrapMode: Text.Wrap

                    background: Item {}
                }
            }
        }
    }
}
