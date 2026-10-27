import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    readonly property var instructionsModel: [
        { title: qsTr("Windows"), link: qsTr("documentation/instructions/connect-amnezia-premium#windows") },
        { title: qsTr("macOS"), link: qsTr("documentation/instructions/connect-amnezia-premium#macos") },
        { title: qsTr("Android"), link: qsTr("documentation/instructions/connect-amnezia-premium#android") },
        { title: qsTr("AndroidTV"), link: qsTr("documentation/instructions/android_tv_connect/") },
        { title: qsTr("iOS"), link: qsTr("documentation/instructions/connect-amnezia-premium#ios") },
        { title: qsTr("Linux"), link: qsTr("documentation/instructions/connect-amnezia-premium#linux") },
        { title: qsTr("Routers"), link: qsTr("documentation/instructions/connect-amnezia-premium#routers") }
    ]

    FlickableType {
        id: flickable

        anchors.top: parent.top

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("How to connect on another device")
                description: qsTr("Setup guides on the Amnezia website")
            }

            Repeater {
                model: root.instructionsModel

                delegate: ListItemType {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16

                    text: modelData.title
                    trailingIconSource: "qrc:/images/controls/external-link.svg"

                    onClicked: Qt.openUrlExternally(LanguageUiController.getCurrentDocsUrl(modelData.link))
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 16 + PageController.safeAreaBottomMargin
            }
        }
    }
}
