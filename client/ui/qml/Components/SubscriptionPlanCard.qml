import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Controls2/TextTypes"

Rectangle {
    id: root

    property bool selected: false
    property string billingPeriod: ""
    property string priceLabel: ""
    property string subtitle: ""
    property bool showRecommendedBadge: false
    property string recommendedText: "Recommended"

    signal selectRequested

    implicitHeight: cardLayout.implicitHeight + 28
    radius: 16
    color: AmneziaStyle.color.transparent
    border.width: selected ? 2 : 1
    border.color: selected ? AmneziaStyle.color.goldenApricot : AmneziaStyle.color.charcoalGray

    ColumnLayout {
        id: cardLayout

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 14
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        spacing: 8

        RowLayout {
            Layout.fillWidth: true

            LabelTextType {
                Layout.fillWidth: true
                text: root.billingPeriod
                color: root.selected ? AmneziaStyle.color.goldenApricot : AmneziaStyle.color.paleGray
                font.pixelSize: 17
                font.weight: Font.DemiBold
                wrapMode: Text.Wrap
            }

            LabelTextType {
                text: root.priceLabel
                color: root.selected ? AmneziaStyle.color.goldenApricot : AmneziaStyle.color.paleGray
                font.pixelSize: 17
                font.weight: Font.DemiBold
            }
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root.subtitle.length > 0 || root.showRecommendedBadge

            LabelTextType {
                Layout.fillWidth: true
                text: root.subtitle
                color: AmneziaStyle.color.mutedGray
                font.pixelSize: 13
                wrapMode: Text.Wrap
            }

            Rectangle {
                visible: root.showRecommendedBadge
                Layout.alignment: Qt.AlignVCenter
                radius: 10
                color: AmneziaStyle.color.softViolet
                implicitHeight: recLabel.implicitHeight + 8
                implicitWidth: recLabel.implicitWidth + 16

                LabelTextType {
                    id: recLabel

                    anchors.centerIn: parent
                    text: root.recommendedText
                    color: AmneziaStyle.color.midnightBlack
                    font.pixelSize: 11
                    font.weight: Font.Medium
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.selectRequested()
    }
}
