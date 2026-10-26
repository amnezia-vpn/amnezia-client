import QtQuick

import "../Config"
import "../Controls2"

ListViewType {
    id: root

    property var listModel: null

    readonly property var chips: root.listModel !== null ? root.listModel.useCases : []

    visible: root.chips.length > 1

    implicitHeight: root.visible ? 34 : 0

    clip: false
    orientation: ListView.Horizontal
    spacing: 8
    leftMargin: 16
    rightMargin: 16
    boundsBehavior: Flickable.StopAtBounds

    model: root.chips

    delegate: ChipType {
        required property var modelData

        readonly property string useCaseId: modelData.useCaseId

        checked: root.listModel !== null && root.listModel.activeUseCaseId === useCaseId
        text: "%1 · %2".arg(CountryRegionNames.useCaseName(root.listModel, useCaseId)).arg(modelData.count)

        onClicked: {
            if (root.listModel !== null) {
                root.listModel.activeUseCaseId = useCaseId
            }
        }
    }
}
