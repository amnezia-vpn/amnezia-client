import QtQuick
import QtQuick.Layouts

import "."

import Style 1.0

Rectangle {
    id: root

    property var benefitsModel: null

    visible: benefitsModel && benefitsModel.rowCount() > 0

    radius: 16
    color: AmneziaStyle.color.benefitsPanelBackground
    implicitHeight: inner.implicitHeight + 24

    ColumnLayout {
        id: inner

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 20

        Repeater {
            model: benefitsModel

            delegate: BenefitRow {
                Layout.fillWidth: true
                iconSource: model.icon
                titleText: model.title
                bodyText: model.body
                link: !!model.link
            }
        }
    }
}
