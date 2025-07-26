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
                Layout.topMargin: 8
                Layout.rightMargin: 16
                Layout.leftMargin: 16
                Layout.bottomMargin: 32

                headerText: ApiServicesModel.getSelectedServiceData("name")
                descriptionText: ApiServicesModel.getSelectedServiceData("serviceDescription")
            }
        }

        model: inputFields
        spacing: 0

        delegate: ColumnLayout {
            width: listView.width

            LabelWithImageType {
                Layout.fillWidth: true
                Layout.margins: 16

                imageSource: imagePath
                leftText: lText
                rightText: rText
            }
        }

        footer: ColumnLayout {
            width: listView.width

            spacing: 0

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
                    return text.replace("%1", LanguageModel.getCurrentSiteUrl("free"))
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            BasicButtonType {
                id: continueButton

                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.bottomMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Connect")

                clickedFunc: function() {
                    var endpoint = ApiServicesModel.getStoreEndpoint()
                    if (endpoint !== undefined && endpoint !== "") {
                        Qt.openUrlExternally(endpoint)
                        PageController.closePage()
                        PageController.closePage()
                    } else {
                        PageController.showBusyIndicator(true)
                        ApiConfigsController.importServiceFromGateway()
                        PageController.showBusyIndicator(false)
                    }
                }
            }
        }
    }

    property list<QtObject> inputFields: [
        region,
        price,
        timeLimit,
        speed,
        features
    ]

    QtObject {
        id: region

        readonly property string imagePath: "qrc:/images/controls/map-pin.svg"
        readonly property string lText: qsTr("For the region")
        readonly property string rText: ApiServicesModel.getSelectedServiceData("region")
        property bool isVisible: true
    }

    QtObject {
        id: price

        readonly property string imagePath: "qrc:/images/controls/tag.svg"
        readonly property string lText: qsTr("Price")
        readonly property string rText: ApiServicesModel.getSelectedServiceData("price")
        property bool isVisible: true
    }

    QtObject {
        id: timeLimit

        readonly property string imagePath: "qrc:/images/controls/history.svg"
        readonly property string lText: qsTr("Work period")
        readonly property string rText: ApiServicesModel.getSelectedServiceData("timeLimit")
        property bool isVisible: rText !== ""
    }

    QtObject {
        id: speed

        readonly property string imagePath: "qrc:/images/controls/gauge.svg"
        readonly property string lText: qsTr("Speed")
        readonly property string rText: ApiServicesModel.getSelectedServiceData("speed")
        property bool isVisible: true
    }

    QtObject {
        id: features

        readonly property string imagePath: "qrc:/images/controls/info.svg"
        readonly property string lText: qsTr("Features")
        readonly property string rText: ""
        property bool isVisible: true
    }
}
