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
    implicitHeight: content.implicitHeight + 24 + PageController.safeAreaBottomMargin

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

        Rectangle {
            Layout.preferredWidth: 44
            Layout.preferredHeight: 44
            Layout.alignment: Qt.AlignVCenter

            visible: root.moreButtonVisible

            radius: 16
            color: AmneziaStyle.color.bgSurfaceAlt

            IconButtonType {
                id: moreButton

                anchors.fill: parent

                icon.source: "qrc:/images/controls/more-vertical.svg"
                icon.width: 20
                icon.height: 20
                Accessible.name: root.moreButtonAccessibleName

                onClicked: root.moreClicked()
            }
        }
    }
}
