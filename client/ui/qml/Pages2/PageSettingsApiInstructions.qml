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

    QtObject {
        id: windows

        readonly property string title: qsTr("Windows")
        readonly property string link: qsTr("documentation/instructions/connect-amnezia-premium#windows")
    }

    QtObject {
        id: macos

        readonly property string title: qsTr("macOS")
        readonly property string link: qsTr("documentation/instructions/connect-amnezia-premium#macos")
    }

    QtObject {
        id: android

        readonly property string title: qsTr("Android")
        readonly property string link: qsTr("documentation/instructions/connect-amnezia-premium#android")
    }

    QtObject {
        id: androidTv

        readonly property string title: qsTr("AndroidTV")
        readonly property string link: qsTr("documentation/instructions/android_tv_connect/")
    }

    QtObject {
        id: ios

        readonly property string title: qsTr("iOS")
        readonly property string link: qsTr("documentation/instructions/connect-amnezia-premium#ios")
    }

    QtObject {
        id: linux

        readonly property string title: qsTr("Linux")
        readonly property string link: qsTr("documentation/instructions/connect-amnezia-premium#linux")
    }

    QtObject {
        id: routers

        readonly property string title: qsTr("Routers")
        readonly property string link: qsTr("documentation/instructions/connect-amnezia-premium#routers")
    }

    property list<QtObject> instructionsModel: [
        windows,
        macos,
        android,
        androidTv,
        ios,
        linux,
        routers
    ]

    ListViewType {
        id: listView

        anchors.fill: parent

        model: instructionsModel

        header: ColumnLayout {
            width: listView.width

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("How to connect on another device")
                description: qsTr("Setup guides on the Amnezia website")
            }

            Item {
                Layout.preferredHeight: 16
            }
        }

        footer: Item {
            width: listView.width
            height: 16 + PageController.safeAreaBottomMargin
        }

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: title
                trailingIconSource: "qrc:/images/controls/external-link.svg"

                onClicked: {
                    Qt.openUrlExternally(LanguageUiController.getCurrentDocsUrl(link))
                }
            }
        }
    }
}
