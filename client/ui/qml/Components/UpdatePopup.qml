import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0
import "../Controls2"
import "../Controls2/TextTypes"

Popup {
    id: root
    width: Math.min(parent.width - 28, 560)
    x: (parent.width - width) / 2
    y: Math.max(24, (parent.height - implicitHeight) / 2)
    modal: true
    focus: true
    
    // If it's a critical update, prevent closing
    closePolicy: isCritical ? Popup.NoAutoClose : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)
    padding: 20

    property string latestVersion: ""
    property string releaseNotes: ""
    property bool isCritical: false

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.70)
    }

    background: Rectangle {
        clip: true
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(27/255, 30/255, 41/255, 0.99) }
            GradientStop { position: 1.0; color: Qt.rgba(19/255, 22/255, 31/255, 0.99) }
        }
        radius: 20
        border.color: Qt.rgba(16/255, 185/255, 129/255, 0.35)
        border.width: 1

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.topMargin: 10
            height: 3
            radius: 2
            color: Qt.rgba(16/255, 185/255, 129/255, 0.8)
        }
    }

    contentItem: ColumnLayout {
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            PremiumBadge {
                text: qsTr("Обновление")
                tone: "success"
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                visible: !root.isCritical
                width: 30
                height: 30
                radius: 15
                color: closeUpdateMouse.pressed
                    ? Qt.rgba(1, 1, 1, 0.20)
                    : (closeUpdateMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.10))
                border.color: Qt.rgba(1, 1, 1, 0.12)
                border.width: 1

                Text {
                    anchors.fill: parent
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: "×"
                    color: FBLinkStyle.color.paleGray
                    font.pixelSize: 18
                    font.weight: 500
                }

                MouseArea {
                    id: closeUpdateMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        LabelTextType {
            Layout.fillWidth: true
            text: qsTr("Доступна версия %1").arg(root.latestVersion)
            font.pixelSize: 24
            font.weight: 700
            color: "#FFFFFF"
            wrapMode: Text.WordWrap
        }

        CaptionTextType {
            Layout.fillWidth: true
            text: root.releaseNotes !== "" ? root.releaseNotes : qsTr("Улучшена стабильность и скорость работы.")
            color: FBLinkStyle.color.mutedGray
            wrapMode: Text.WordWrap
        }

        Item { Layout.preferredHeight: 8 }

        BasicButtonType {
            Layout.fillWidth: true
            implicitHeight: 46
            text: qsTr("Обновить сейчас")
            defaultColor: "#10B981"
            hoveredColor: "#34D399"
            pressedColor: "#059669"
            textColor: "#FFFFFF"
            clickedFunc: function() {
                UpdateController.openDownloadUrl()
                if (!root.isCritical) {
                    root.close()
                }
            }
        }

        BasicButtonType {
            visible: !root.isCritical
            Layout.fillWidth: true
            implicitHeight: 42
            text: qsTr("Позже")
            defaultColor: Qt.rgba(1, 1, 1, 0.08)
            hoveredColor: Qt.rgba(1, 1, 1, 0.12)
            pressedColor: Qt.rgba(1, 1, 1, 0.18)
            textColor: FBLinkStyle.color.paleGray
            clickedFunc: function() {
                root.close()
            }
        }
    }

    Connections {
        target: UpdateController
        function onUpdateChecked(hasUpdate, version, notes, isCritical) {
            if (hasUpdate) {
                root.latestVersion = version
                root.releaseNotes = notes
                root.isCritical = isCritical
                root.open()
            }
        }
    }
}
