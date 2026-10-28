import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

ColumnLayout {
    id: root

    property bool expanded: false
    property bool pressed: false
    property bool outdatedWarningVisible: false
    property bool protocolSelectionVisible: false
    property bool protocolSelectable: false
    property string protocolText

    signal expandRequested()
    signal serverInfoRequested()
    signal protocolSelectionRequested()


    spacing: 0

    RowLayout {
        Layout.topMargin: 16
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter

        spacing: 10

        H1TextType {
            Layout.maximumWidth: root.width - 32 - (chevron.visible ? chevron.width + 10 : 0)

            maximumLineCount: 1
            wrapMode: Text.NoWrap
            elide: Qt.ElideRight

            text: ServersUiController.defaultServerName
            horizontalAlignment: Qt.AlignHCenter
        }

        IconButtonType {
            id: chevron

            visible: !root.expanded

            hoverEnabled: false
            down: pressed || root.pressed
            size: IconButtonType.Small
            icon.source: "qrc:/images/controls/chevron-down.svg"

            onClicked: root.expandRequested()
        }
    }

    HomeInfoButton {
        Layout.fillWidth: true
        Layout.maximumWidth: implicitWidth
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: root.expanded ? 8 : 6
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.bottomMargin: (root.outdatedWarningVisible || root.protocolSelectionVisible) ? 8 : 16

        enabled: ServersUiController.defaultServerImagePathCollapsed !== ""

        largeText: true
        maximumLineCount: 2
        text: ServersUiController.defaultServerDescriptionCollapsed
        leftImageSource: ServersUiController.defaultServerImagePathCollapsed
        leftImageColor: AmneziaStyle.color.transparent
        leftImageWidth: 20
        leftImageHeight: 15
        rightImageSource: enabled ? (root.expanded ? "qrc:/images/controls/chevron-down.svg" : "qrc:/images/controls/chevron-right.svg") : ""

        onClicked: root.serverInfoRequested()
    }

    NotificationType {
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.bottomMargin: 16

        visible: root.outdatedWarningVisible

        variant: NotificationType.Warning
        text: qsTr("AmneziaWG 2.0 is outdated and no longer supported. Continued use requires a fresh installation of the AmneziaWG 3.1 container.")
    }

    HomeInfoButton {
        Layout.fillWidth: true
        Layout.maximumWidth: implicitWidth
        Layout.alignment: Qt.AlignHCenter
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.bottomMargin: 16

        visible: root.protocolSelectionVisible
        enabled: root.protocolSelectable

        largeText: true
        text: root.protocolText
        leftImageSource: "qrc:/images/controls/arrow-left-right.svg"
        leftImageColor: AmneziaStyle.color.iconSecondary
        rightImageSource: enabled ? "qrc:/images/controls/chevron-down.svg" : ""

        onClicked: root.protocolSelectionRequested()
    }
}
