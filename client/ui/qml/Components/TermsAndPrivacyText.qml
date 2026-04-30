import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Controls2/TextTypes"

ParagraphTextType {
    id: root

    property string termsUrl: ""
    property string privacyUrl: ""

    Layout.fillWidth: true

    horizontalAlignment: Text.AlignHCenter
    textFormat: Text.RichText
    color: AmneziaStyle.color.mutedGray
    font.pixelSize: 12
    lineHeight: 1.35
    lineHeightMode: Text.ProportionalHeight

    text: qsTr("By continuing, you agree to the <a href=\"%1\" style=\"color: %3;\">Terms of Use</a> and <a href=\"%2\" style=\"color: %3;\">Privacy Policy</a>")
            .arg(root.termsUrl)
            .arg(root.privacyUrl)
            .arg(AmneziaStyle.color.goldenApricotString)

    onLinkActivated: function(link) {
        Qt.openUrlExternally(link)
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
    }
}
