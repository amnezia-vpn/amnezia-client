import QtQuick
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    property string title
    property string description
    property bool backButtonVisible: d.page === null || d.page.stackView === null || d.page.stackView.depth > 1
    property string backButtonImage: "qrc:/images/controls/arrow-left.svg"

    default property alias actions: actionsRow.data

    readonly property real barHeight: d.toolbarTop + 56

    signal backClicked()

    implicitHeight: column.implicitHeight

    z: 2

    onBackClicked: PageController.closePage()

    Component.onCompleted: {
        d.flickable = d.findFlickable()
        d.page = d.findPage()
    }

    QtObject {
        id: d

        property Item flickable: null
        property PageType page: null

        readonly property real toolbarTop: PageController.safeAreaTopMargin
        readonly property real sideWidth: Math.max(48, actionsRow.width)

        readonly property real scrolled: {
            if (!d.flickable) {
                return 0
            }
            d.flickable.contentY
            d.flickable.originY
            root.y
            return Math.max(0, -root.mapToItem(d.flickable, 0, 0).y)
        }

        function findPage() {
            let item = root.parent
            while (item) {
                if (item instanceof PageType) {
                    return item
                }
                item = item.parent
            }
            return null
        }

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
            Layout.preferredHeight: 56
        }

        BodyTextType {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            visible: root.description !== ""

            text: root.description
            color: AmneziaStyle.color.textSecondary
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: root.description !== "" ? 24 : 16
        }
    }

    Rectangle {
        id: toolbar

        width: root.width
        height: root.barHeight
        y: d.scrolled

        color: d.scrolled > 0 ? AmneziaStyle.color.bgBase : AmneziaStyle.color.transparent

        MouseArea {
            anchors.fill: parent

            enabled: d.scrolled > 0
            acceptedButtons: Qt.AllButtons
        }

        IconButtonType {
            id: backButton

            x: 4
            y: d.toolbarTop + 4

            visible: root.backButtonVisible
            icon.source: root.backButtonImage
            Accessible.name: qsTr("Back")

            onClicked: root.backClicked()
        }

        H3TextType {
            x: 4 + d.sideWidth + 8
            y: d.toolbarTop + (56 - height) / 2
            width: root.width - 2 * (4 + d.sideWidth + 8)

            font.weight: 600
            font.letterSpacing: 0

            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.NoWrap
            maximumLineCount: 1
            elide: Text.ElideRight
            text: root.title
        }

        Row {
            id: actionsRow

            anchors.right: parent.right
            anchors.rightMargin: 4
            y: d.toolbarTop + 4
        }
    }
}
