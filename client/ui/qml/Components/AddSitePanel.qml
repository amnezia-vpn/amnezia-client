import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Controls2"

Item {
    id: root

    property string placeholderText: ""
    property alias textField: searchField.textField

    signal addClicked(string text)
    signal moreClicked()

    implicitWidth: 360
    implicitHeight: content.implicitHeight + 32 + PageController.safeAreaBottomMargin

    Rectangle {
        anchors.fill: parent

        color: AmneziaStyle.color.bgBase
        opacity: 0.85
    }

    RowLayout {
        id: content

        anchors.fill: parent
        anchors.topMargin: 16
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin

        spacing: 8

        TextFieldType {
            id: searchField

            Layout.fillWidth: true

            rightButtonClickedOnEnter: true
            placeholderText: root.placeholderText
            buttonImageSource: "qrc:/images/controls/plus.svg"
            buttonJoined: true

            clickedFunc: function() {
                root.addClicked(searchField.textField.text)
                searchField.textField.text = ""
            }
        }

        IconButtonType {
            implicitWidth: 64
            implicitHeight: 64

            icon.source: "qrc:/images/controls/more-vertical.svg"

            onClicked: root.moreClicked()
        }
    }
}
