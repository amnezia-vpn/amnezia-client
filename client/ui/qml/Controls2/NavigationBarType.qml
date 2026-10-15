import QtQuick
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    property string title
    property string description
    property bool backButtonVisible: true
    property bool toolbarVisible: true
    property string backButtonImage: "qrc:/images/controls/arrow-left.svg"

    default property alias actions: actionsRow.data

    readonly property alias header: header

    signal backClicked()

    implicitHeight: column.implicitHeight

    onBackClicked: PageController.closePage()

    ColumnLayout {
        id: column

        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.topMargin: PageController.safeAreaTopMargin
            Layout.preferredHeight: root.toolbarVisible ? 48 : 0

            IconButtonType {
                id: backButton

                x: 16
                y: 4

                visible: root.toolbarVisible && root.backButtonVisible
                size: IconButtonType.Small
                icon.source: root.backButtonImage
                Accessible.name: qsTr("Back")

                onClicked: root.backClicked()
            }

            Row {
                id: actionsRow

                anchors.right: parent.right
                anchors.rightMargin: 16
                y: 4

                visible: root.toolbarVisible
                spacing: 16
            }
        }

        HeaderType {
            id: header

            Layout.fillWidth: true
            Layout.topMargin: 12
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 16

            visible: root.title !== ""

            headerText: root.title
            descriptionText: root.description
        }
    }
}
