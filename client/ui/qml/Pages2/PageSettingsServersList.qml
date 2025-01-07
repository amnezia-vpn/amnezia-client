import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    ColumnLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
        }

        HeaderType {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16

            headerText: qsTr("Servers")
        }
    }

    ListView {
        id: servers
        objectName: "servers"

        width: parent.width
        anchors.top: header.bottom
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.right: parent.right

        height: 500

        property bool isFocusable: true

        model: ServersModel

        clip: true
        reuseItems: true

        delegate: Item {
            implicitWidth: servers.width
            implicitHeight: delegateContent.implicitHeight

            ColumnLayout {
                id: delegateContent

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                LabelWithButtonType {
                    id: server
                    Layout.fillWidth: true

                    text: name

                    descriptionText: {
                        var servicesNameString = ""
                        var servicesName = ServersModel.getAllInstalledServicesName(index)
                        for (var i = 0; i < servicesName.length; i++) {
                            servicesNameString += servicesName[i] + " · "
                        }

                        if (ServersModel.isServerFromApi(index)) {
                            return servicesNameString + serverDescription
                        } else {
                            return servicesNameString + hostName
                        }
                    }
                    rightImageSource: "qrc:/images/controls/chevron-right.svg"

                    clickedFunction: function() {
                        ServersModel.processedIndex = index
                        PageController.goToPage(PageEnum.PageSettingsServerInfo)
                    }
                }

                DividerType {}
            }
        }
    }
}
