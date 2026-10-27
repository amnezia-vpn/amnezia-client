import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

Pane {
    id: root

    property var benefitsModel: null
    property int benefitsCount: 0

    visible: root.benefitsCount > 0

    padding: 16

    background: Rectangle {
        radius: 16
        color: AmneziaStyle.color.bgSurface
    }

    contentItem: ColumnLayout {
        spacing: 16

        Repeater {
            id: benefitsRepeater

            model: root.benefitsModel

            onCountChanged: root.benefitsCount = count

            delegate: BenefitRow {
                Layout.fillWidth: true

                iconSource: model.icon
                title: model.title
                subtitle: model.body
                isLink: !!model.link
            }
        }
    }
}
