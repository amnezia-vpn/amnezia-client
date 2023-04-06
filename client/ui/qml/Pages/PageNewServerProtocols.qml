import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ContainerProps 1.0
import ProtocolProps 1.0
import PageEnum 1.0
import "./"
import "../Controls"
import "../Config"
import "InstallSettings"

PageBase {
    id: root
    page: PageEnum.NewServerProtocols
    logic: NewServerProtocolsLogic

    onActivated: {
        container_selector.selectedIndex = -1
        UiLogic.containersModel.setSelectedServerIndex(-1)
    }

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Select VPN protocols")
    }

    BlueButtonType {
        id: pushButtonConfigure
        enabled: container_selector.selectedIndex > 0
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height - 60
        width: parent.width - 40
        height: 40
        text: qsTr("Setup server")
        onClicked: {
            let cont = container_selector.selectedIndex
            let tp = ProtocolProps.transportProtoFromString(cb_port_proto.currentText)
            let port = tf_port_num.text
            NewServerProtocolsLogic.onPushButtonConfigureClicked(cont, port, tp)
        }
    }

    BlueButtonType {
        id: pb_add_container
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: caption.bottom
        anchors.topMargin: 10

        width: parent.width - 40
        height: 40
        text: qsTr("Select protocol container")
        font.pixelSize: 16
        onClicked: container_selector.visible ? container_selector.close() : container_selector.open()

    }

    SelectContainer {
        id: container_selector
        onAboutToHide: {
            pageLoader.focus = true
        }

        onContainerSelected: function(c_index){
            var containerProto =  ContainerProps.defaultProtocol(c_index)

            tf_port_num.text = ProtocolProps.defaultPort(containerProto)
            cb_port_proto.currentIndex = ProtocolProps.defaultTransportProto(containerProto)

            tf_port_num.enabled = ProtocolProps.defaultPortChangeable(containerProto)
            cb_port_proto.enabled = ProtocolProps.defaultTransportProtoChangeable(containerProto)
        }
    }

    Column {
        id: c1
        visible: container_selector.selectedIndex > 0
        width: parent.width
        anchors.top: pb_add_container.bottom
        anchors.topMargin: 10

        Caption {
            font.pixelSize: 22
            text: UiLogic.containerName(container_selector.selectedIndex)
        }

        Text {
            width: parent.width
            anchors.topMargin: 10
            padding: 10

            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 16
            color: "#181922"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap

            text: UiLogic.containerDesc(container_selector.selectedIndex)
        }
    }



    Rectangle {
        id: frame_settings
        visible: container_selector.selectedIndex > 0
        width: parent.width
        anchors.top: c1.bottom
        anchors.topMargin: 10

        border.width: 1
        border.color: "lightgray"
        anchors.bottomMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        radius: 2
        Grid {
            id: grid
            visible: container_selector.selectedIndex > 0
            anchors.fill: parent
            columns: 2
            horizontalItemAlignment: Grid.AlignHCenter
            verticalItemAlignment: Grid.AlignVCenter
            topPadding: 5
            leftPadding: 10
            spacing: 5


            LabelType {
                width: 130
                text: qsTr("Port")
            }
            TextFieldType {
                id: tf_port_num
                width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
            }
            LabelType {
                width: 130
                text: qsTr("Network Protocol")
            }
            ComboBoxType {
                id: cb_port_proto
                width: parent.width - 130 - parent.spacing - parent.leftPadding * 2
                model: [
                    qsTr("udp"),
                    qsTr("tcp"),
                ]
            }
        }
    }
}
