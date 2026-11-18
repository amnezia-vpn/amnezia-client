import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Controls2"

Item {
    id: root

    property string placeholderText: ""
    property bool moreButtonVisible: true
    property string moreButtonAccessibleName: ""
    property alias textField: searchField.textField
    readonly property alias moreButton: moreButton

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
        anchors.topMargin: 24
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

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
            id: moreButton

            Layout.preferredWidth: 56
            Layout.preferredHeight: 56
            Layout.alignment: Qt.AlignVCenter

            visible: root.moreButtonVisible

            icon.source: "qrc:/images/controls/more-vertical.svg"
            Accessible.name: root.moreButtonAccessibleName

            onClicked: root.moreClicked()
        }
    }
}
