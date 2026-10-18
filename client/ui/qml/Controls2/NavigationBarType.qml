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
    readonly property real collapseProgress: d.collapseProgress

    signal backClicked()

    implicitHeight: column.implicitHeight

    z: 1

    onBackClicked: PageController.closePage()

    Component.onCompleted: d.flickable = d.findFlickable()

    QtObject {
        id: d

        property Item flickable: null

        readonly property real toolbarTop: PageController.safeAreaTopMargin
        readonly property real titleHeight: header.visible ? header.titleItem.height : 0

        readonly property real scrolled: {
            if (!d.flickable || !root.toolbarVisible) {
                return 0
            }
            d.flickable.contentY
            d.flickable.originY
            root.y
            return Math.max(0, -root.mapToItem(d.flickable, 0, 0).y)
        }

        readonly property real collapseProgress: d.titleHeight > 0
                                                 ? Math.max(0, Math.min(1, (d.scrolled - 12 - d.titleHeight / 2) / (d.titleHeight / 2)))
                                                 : 0

        readonly property real barHeight: 48 - 4 * d.collapseProgress

        function findFlickable() {
            let item = root.parent
            while (item) {
                if (item.contentY !== undefined && item.contentItem !== undefined && item.flickableDirection !== undefined) {
                    return item
                }
                item = item.parent
            }
            return null
        }
    }

    ColumnLayout {
        id: column

        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 0

        Item {
            Layout.fillWidth: true
            Layout.topMargin: d.toolbarTop
            Layout.preferredHeight: root.toolbarVisible ? 48 : 0
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

    Rectangle {
        id: toolbar

        width: root.width
        height: d.toolbarTop + d.barHeight
        y: d.scrolled

        visible: root.toolbarVisible
        color: d.scrolled > 0 ? AmneziaStyle.color.bgBase : AmneziaStyle.color.transparent

        MouseArea {
            anchors.fill: parent

            enabled: d.scrolled > 0
            acceptedButtons: Qt.AllButtons
        }

        IconButtonType {
            id: backButton

            x: 16
            y: d.toolbarTop + (d.barHeight - height) / 2

            visible: root.backButtonVisible
            size: IconButtonType.Small
            icon.source: root.backButtonImage
            Accessible.name: qsTr("Back")

            onClicked: root.backClicked()
        }

        H3TextType {
            readonly property real sideInset: 16 + 36 + 16

            x: Math.max(sideInset, (root.width - width) / 2)
            y: d.toolbarTop + (d.barHeight - height) / 2
            width: Math.min(implicitWidth, root.width - 2 * sideInset)

            visible: opacity > 0
            opacity: d.collapseProgress

            wrapMode: Text.NoWrap
            maximumLineCount: 1
            elide: Text.ElideRight
            text: root.title
        }

        Row {
            id: actionsRow

            anchors.right: parent.right
            anchors.rightMargin: 16
            y: d.toolbarTop + (d.barHeight - height) / 2

            spacing: 16
        }
    }
}
