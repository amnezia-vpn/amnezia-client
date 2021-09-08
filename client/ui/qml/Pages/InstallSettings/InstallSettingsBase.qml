import QtQuick 2.12
import QtQuick.Controls 2.12
import "./"
import "../../Controls"
import "../../Config"

Rectangle {
    signal containerChecked(bool checked)
    property bool initiallyChecked: false
    property string containerDescription
    default property alias itemSettings: container.data

    x: 5
    y: 5
    width: parent.width - 20
    anchors.horizontalCenter: parent.horizontalCenter

    height: frame_settings.visible ? 140 : 72
    border.width: 1
    border.color: "lightgray"
    radius: 2
    Rectangle {
        id: frame_settings
        height: 77
        width: parent.width
        border.width: 1
        border.color: "lightgray"
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        visible: false
        radius: 2
        Grid {
            id: container
            anchors.fill: parent
            columns: 2
            horizontalItemAlignment: Grid.AlignHCenter
            verticalItemAlignment: Grid.AlignVCenter
            topPadding: 5
            leftPadding: 10
            spacing: 5
        }
    }
    Row {
        anchors.top: parent.top
        anchors.topMargin: 5
        leftPadding: 15
        rightPadding: 5
        height: 55
        width: parent.width
        CheckBoxType {
            text: containerDescription
            height: parent.height
            width: parent.width - 50
            checked: initiallyChecked
            onCheckedChanged: containerChecked(checked)
        }
        ImageButtonType {
            width: 35
            height: 35
            anchors.verticalCenter: parent.verticalCenter
            icon.source: "qrc:/images/settings.png"
            checkable: true
            checked: initiallyChecked
            onCheckedChanged: {
                //NewServerProtocolsLogic.pushButtonSettingsCloakChecked = checked
                if (checked) {
                    frame_settings.visible = true
                } else {
                    frame_settings.visible = false
                }
            }
        }
    }
}
