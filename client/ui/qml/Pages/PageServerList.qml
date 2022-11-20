import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12
import PageEnum 1.0
import "../Controls"
import "./"
import "../Config"

PageBase {
    id: root
    page: PageEnum.ServersList
    logic: ServerListLogic

    BackButton {
        id: back
    }
    Caption {
        id: caption
        text: qsTr("Servers")
        width: undefined
    }

    SvgButtonType {
        anchors.verticalCenter: caption.verticalCenter
        anchors.leftMargin: 10
        anchors.left: caption.right
        width: 27
        height: 27

        icon.source: "qrc:/images/svg/control_point_black_24dp.svg"
        onClicked: {
            UiLogic.goToPage(PageEnum.Start);
        }
    }

    ListView {
        id: listWidget_servers
        x: 20
        anchors.top: caption.bottom
        anchors.topMargin: 15
        width: parent.width
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        model: ServerListLogic.serverListModel
        highlightRangeMode: ListView.ApplyRange
        highlightMoveVelocity: -1
        currentIndex: ServerListLogic.currServerIdx
        spacing: 5
        clip: true
        delegate: Item {
            height: 60
            width: root.width - 40
            MouseArea {
                id: ms
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    if (GC.isMobile()) {
                        ServerListLogic.onServerListPushbuttonSettingsClicked(index)
                    }
                    mouse.accepted = false
                }
                onEntered: {
                    mouseExitAni.stop()
                    mouseEnterAni.start()
                }
                onExited: {
                    mouseEnterAni.stop()
                    mouseExitAni.start()
                }
            }
            LinearGradient {
                visible: !ms.containsMouse
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(0, height)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#FAFBFE" }
                    GradientStop { position: 1.0; color: "#ECEEFF" }
                }
            }
            LinearGradient {
                visible: ms.containsMouse
                anchors.fill: parent
                start: Qt.point(0, 0)
                end: Qt.point(0, height)
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#FAFBFE" }
                    GradientStop { position: 1.0; color: "#DCDEDF" }
                }
            }
            LabelType {
                id: label_address
                x: 20
                y: 40
                width: 141
                height: 16
                text: address
            }
            Text {
                x: 10
                y: 10
                width: 181
                height: 21
                font.family: "Lato"
                font.styleName: "normal"
                font.pixelSize: 16
                font.bold: true
                color: "#181922"
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.Wrap
                text: desc
            }
            ImageButtonType {
                x: parent.width - 30
                y: 15
                width: 30
                height: 30
                checkable: true
                icon.source: checked ? "qrc:/images/check.png"
                                     : "qrc:/images/uncheck.png"
                onClicked: {
                    ServerListLogic.onServerListPushbuttonDefaultClicked(index)
                }
                checked: is_default
                enabled: !is_default
            }
            SvgButtonType {
                id: pushButtonSetting
                x: parent.width - 70
                y: 15
                width: 30
                height: 30
                icon.source: "qrc:/images/svg/settings_black_24dp.svg"
                opacity: 0

                OpacityAnimator {
                    id: mouseEnterAni
                    target: pushButtonSetting;
                    from: 0;
                    to: 1;
                    duration: 150
                    running: false
                    easing.type: Easing.InOutQuad
                }
                OpacityAnimator {
                    id: mouseExitAni
                    target: pushButtonSetting;
                    from: 1;
                    to: 0;
                    duration: 150
                    running: false
                    easing.type: Easing.InOutQuad
                }
                MouseArea {
                    cursorShape: Qt.PointingHandCursor
                    anchors.fill: parent
                    hoverEnabled: true
                    propagateComposedEvents: true

                    onEntered: {
                        mouseExitAni.stop()
                        mouseEnterAni.start()
                    }
                    onExited: {
                        mouseEnterAni.stop()
                        mouseExitAni.start()
                    }

                    onClicked: {
                        ServerListLogic.onServerListPushbuttonSettingsClicked(index)
                    }
                }
            }
        }
    }
}
