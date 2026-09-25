import QtQuick

import Style 1.0

import "../Config"
import "../Controls2"
import "../Controls2/TextTypes"

ListViewType {
    id: root

    property var listModel: null

    readonly property var chips: root.listModel !== null ? root.listModel.useCases : []

    visible: root.chips.length > 1
    isFocusable: root.visible

    implicitHeight: root.visible ? 36 : 0

    orientation: ListView.Horizontal
    spacing: 8
    leftMargin: 16
    rightMargin: 16
    boundsBehavior: Flickable.StopAtBounds
    interactive: contentWidth > width

    model: root.chips

    delegate: Item {
        id: delegateRoot

        required property var modelData

        implicitWidth: chip.implicitWidth
        implicitHeight: 36

        Rectangle {
            id: chip

            readonly property string useCaseId: delegateRoot.modelData.useCaseId
            readonly property bool active: root.listModel !== null
                                           && root.listModel.activeUseCaseId === chip.useCaseId

            property bool isFocusable: true

            anchors.fill: parent
            implicitWidth: label.implicitWidth + 32
            radius: height / 2

            color: chipHover.hovered ? AmneziaStyle.color.surfaceHovered
                                     : AmneziaStyle.color.surfaceBase
            border.width: chip.activeFocus ? 2 : (chip.active ? 1 : 0)
            border.color: chip.active ? AmneziaStyle.color.goldenApricot
                                      : AmneziaStyle.color.paleGray

            function select() {
                if (root.listModel !== null) {
                    root.listModel.activeUseCaseId = chip.useCaseId
                }
            }

            ParagraphTextType {
                id: label

                anchors.centerIn: parent

                color: chip.active ? AmneziaStyle.color.textPrimary : AmneziaStyle.color.paleGray
                wrapMode: Text.NoWrap
                lineHeightMode: Text.ProportionalHeight
                lineHeight: 1.0
                font.letterSpacing: -0.4

                text: "%1 · %2".arg(CountryRegionNames.useCaseName(root.listModel, chip.useCaseId))
                               .arg(delegateRoot.modelData.count)
            }

            HoverHandler {
                id: chipHover
                cursorShape: Qt.PointingHandCursor
            }

            TapHandler {
                gesturePolicy: TapHandler.ReleaseWithinBounds
                onTapped: chip.select()
            }

            Keys.onEnterPressed: chip.select()
            Keys.onReturnPressed: chip.select()
            Keys.onSpacePressed: chip.select()

            Keys.onTabPressed: FocusController.nextKeyTabItem()
            Keys.onBacktabPressed: FocusController.previousKeyTabItem()
            Keys.onLeftPressed: FocusController.nextKeyLeftItem()
            Keys.onRightPressed: FocusController.nextKeyRightItem()
            Keys.onUpPressed: FocusController.nextKeyUpItem()
            Keys.onDownPressed: FocusController.nextKeyDownItem()
        }
    }
}
