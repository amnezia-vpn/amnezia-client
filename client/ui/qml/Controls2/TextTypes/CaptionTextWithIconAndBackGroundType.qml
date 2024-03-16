import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    property string textColor: "#D7D8DB"
    property string backGroundColor: "#1C1D21"
    property string textString

    property string iconPath
    property real iconWidth: 16
    property real iconHeight: 16

    property real iconRightMargin: 6
    property real iconLeftMargin: LanguageModel.getCurrentLanguageIndex() === 3 ? 25: 15

    property real textRightMargin: 30

    color: backGroundColor
    radius: 8
    implicitHeight: supportingText.height + 27

    RowLayout {
      width: parent.width
      anchors.centerIn: parent
      layoutDirection: Qt.RightToLeft

      CaptionTextType {
          id: supportingText
          Layout.fillWidth: true
          Layout.rightMargin: textRightMargin

          font.pixelSize: 14
          text: textString
          color: textColor
      }

      Item {
          Layout.leftMargin: iconLeftMargin
          Layout.bottomMargin: supportingText.top
          Layout.rightMargin: iconRightMargin

          width: iconWidth
          height: iconHeight

          Image {
              width: iconWidth
              height: iconHeight
              source: iconPath
          }
      }
    }
}
