import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    property string textColor: "#D7D8DB"
    property string backGroundColor: "#1C1D21"
    property string textString

    property string iconPath
    property real iconWidth: 15
    property real iconHeight: 15

    property real iconTopMargin
    property real iconRightMargin
    property real iconBottomMargin
    property real iconLeftMargin: 15

    color: backGroundColor
    radius: 8

    RowLayout {
      width: parent.width
      anchors.centerIn: parent
      layoutDirection: Qt.RightToLeft

      CaptionTextType {
          id: supportingText
          Layout.fillWidth: true
          Layout.rightMargin: 10

          font.pixelSize: 14
          text: textString
          color: textColor
      }

      Item {  // setting Item type as a parent is a solution for changing the image size
          Layout.topMargin: iconTopMargin
          Layout.rightMargin: iconRightMargin
          Layout.bottomMargin: supportingText.top
          Layout.leftMargin: iconLeftMargin

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
