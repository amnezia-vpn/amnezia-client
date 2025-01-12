import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    FlickableType {
        id: fl
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        contentHeight: content.height + continueButton.implicitHeight + continueButton.anchors.bottomMargin + continueButton.anchors.topMargin

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            BackButtonType {
                id: backButton
                Layout.topMargin: 20
            }

            HeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 32

                headerText: ApiServicesModel.getSelectedServiceData("name")
                descriptionText: ApiServicesModel.getSelectedServiceData("serviceDescription")
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/map-pin.svg"
                leftText: qsTr("For the region")
                rightText: ApiServicesModel.getSelectedServiceData("region")
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/tag.svg"
                leftText: qsTr("Price")
                rightText: ApiServicesModel.getSelectedServiceData("price")
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/history.svg"
                leftText: qsTr("Work period")
                rightText: ApiServicesModel.getSelectedServiceData("timeLimit")

                visible: rightText !== ""
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/gauge.svg"
                leftText: qsTr("Speed")
                rightText: ApiServicesModel.getSelectedServiceData("speed")
            }

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: "qrc:/images/controls/info.svg"
                leftText: qsTr("Features")
                rightText: ""
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }
                textFormat: Text.RichText
                text: {
                    var text = ApiServicesModel.getSelectedServiceData("features")
                    return text.replace("%1", LanguageModel.getCurrentSiteUrl())
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }
        }
    }

    BasicButtonType {
        id: continueButton

        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        anchors.topMargin: 32
        anchors.rightMargin: 16
        anchors.leftMargin: 16
        anchors.bottomMargin: 32

        text: qsTr("Connect")

        clickedFunc: function() {
            var endpoint = ApiServicesModel.getStoreEndpoint()
            if (endpoint !== undefined && endpoint !== "") {
                Qt.openUrlExternally(endpoint)
                PageController.closePage()
                PageController.closePage()
            } else {
                PageController.showBusyIndicator(true)
                InstallController.installServiceFromApi()
                PageController.showBusyIndicator(false)
            }
        }
    }
}
