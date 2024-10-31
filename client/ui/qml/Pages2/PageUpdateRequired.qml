import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    ColumnLayout {
        anchors.top: parent.top
        anchors.bottom: websiteButton.top
        anchors.left: parent.left
        anchors.right: parent.right

        Image {
            id: image
            source: "qrc:/images/zlovpn.svg"

            Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
            Layout.topMargin: 36
            Layout.preferredWidth: 300
            Layout.preferredHeight: 240
        }

        ColumnLayout {
            id: lookingForServer

            Layout.alignment: Qt.AlignHCenter
            visible: !AuthController.spikeErrored
            Header2Type {
                headerText: qsTr("App update required")
                Layout.alignment: Qt.AlignHCenter
            }

            ParagraphTextType {
                Layout.alignment: Qt.AlignHCenter
                Layout.maximumWidth: parent.width

                text: qsTr("Install the latest version from our website.")
                color: AmneziaStyle.color.mutedGray
            }
        }

        Item {
            Layout.preferredHeight: image.height
            Layout.preferredWidth: image.width
        }
    }


    BasicButtonType {
        id: websiteButton
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16

        text: qsTr("Website")
        imageSource: "qrc:/images/controls/browser.svg"

        clickedFunc: function() {
            Qt.openUrlExternally(LanguageModel.getZloVpnSiteUrl())
        }
    }

    Component.onCompleted: {
        if (AuthController.spikeReady) {
            PageController.goToPageHome()
        }
    }
}
