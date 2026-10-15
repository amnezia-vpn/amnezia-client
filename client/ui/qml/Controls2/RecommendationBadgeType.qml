import QtQuick

import Style 1.0

import "TextTypes"

Rectangle {
    id: root

    property alias text: label.text

    implicitWidth: label.implicitWidth + 16
    implicitHeight: label.implicitHeight + 8

    radius: 8
    color: AmneziaStyle.color.promoRecommendation

    CaptionTextType {
        id: label

        anchors.centerIn: parent

        color: AmneziaStyle.color.textInverted
        wrapMode: Text.NoWrap
    }
}
