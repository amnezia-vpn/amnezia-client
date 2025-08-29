import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    Connections {
        target: InstallController

        function onUpdateContainerFinished() {
            PageController.showNotificationMessage(qsTr("Settings updated successfully"))
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20
        
        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Tor website settings")
            }
        }

        model: 1 // fake model to force the ListView to be created without a model

        delegate: ColumnLayout { // TODO(CyAn84): add DelegateChooser after migrate to 6.9
            width: listView.width

            LabelWithButtonType {
                id: websiteName

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.bottomMargin: 24

                text: qsTr("Website address")
                descriptionText: {
                    var containerIndex = ContainersModel.getProcessedContainerIndex()
                    var config = ContainersModel.getContainerConfig(containerIndex)
                    return config[ContainerProps.containerTypeToString(containerIndex)]["site"]
                }

                descriptionOnTop: true
                textColor: AmneziaStyle.color.goldenApricot

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }
        }

        footer: ColumnLayout {
            width: listView.width

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                onLinkActivated: Qt.openUrlExternally(link)
                textFormat: Text.RichText
                text: qsTr("Use <a href=\"https://www.torproject.org/download/\" style=\"color: #FBB26A;\">Tor Browser</a> to open this URL.")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("After creating your onion site, it takes a few minutes for the Tor network to make it available for use.")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("When configuring WordPress set the this onion address as domain.")
            }
        }
    }
}
