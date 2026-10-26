import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

Button {
    id: root

    property bool connected: ConnectionController.isConnected
    property bool inProgress: ConnectionController.isConnectionInProgress

    implicitWidth: 190
    implicitHeight: 190

    padding: 0
    hoverEnabled: true

    text: ConnectionController.connectionStateText

    Connections {
        target: ConnectionController

        function onPreparingConfig() {
            PageController.showNotificationMessage(qsTr("Unable to disconnect during configuration preparation"))
        }
    }

    QtObject {
        id: d

        readonly property real ringRadius: (Math.min(root.width, root.height) - 2) / 2
        readonly property color ringColor: root.connected && !root.inProgress ? AmneziaStyle.color.accentLight
                                                                               : AmneziaStyle.color.borderEmphasis
        readonly property int glowMargin: 24
    }

    background: Item {
        implicitWidth: 190
        implicitHeight: 190

        Item {
            anchors.fill: parent

            opacity: 0.1

            Item {
                id: diskSource

                anchors.fill: parent
                anchors.margins: -d.glowMargin

                Shape {
                    anchors.fill: parent

                    preferredRendererType: Shape.CurveRenderer

                    ShapePath {
                        fillColor: AmneziaStyle.color.transparent
                        strokeColor: d.ringColor
                        strokeWidth: 1

                        PathAngleArc {
                            centerX: diskSource.width / 2
                            centerY: diskSource.height / 2
                            radiusX: d.ringRadius
                            radiusY: d.ringRadius
                            startAngle: 0
                            sweepAngle: 360
                        }
                    }
                }
            }

            ShadowType {
                anchors.fill: diskSource
                z: -1

                source: diskSource
                color: AmneziaStyle.color.shadowGlow
                blur: 6
            }
        }

        Item {
            anchors.fill: parent

            RingGlowType {
                anchors.fill: parent
                anchors.margins: -d.glowMargin

                color: d.ringColor
                radius: d.ringRadius
                startAngle: 100
                sweepAngle: root.inProgress ? 160 : 360
            }

            Shape {
                id: ringSource

                anchors.fill: parent
                anchors.margins: -d.glowMargin

                preferredRendererType: Shape.CurveRenderer

                ShapePath {
                    fillColor: AmneziaStyle.color.transparent
                    strokeColor: d.ringColor
                    strokeWidth: 2
                    capStyle: ShapePath.RoundCap

                    PathAngleArc {
                        centerX: ringSource.width / 2
                        centerY: ringSource.height / 2
                        radiusX: d.ringRadius
                        radiusY: d.ringRadius
                        startAngle: 100
                        sweepAngle: root.inProgress ? 160 : 360
                    }
                }
            }

            RotationAnimator on rotation {
                running: root.inProgress
                from: 0
                to: 360
                loops: Animation.Infinite
                duration: 1000
            }
        }

        FocusRingType {
            radius: root.width / 2
            visible: root.visualFocus
        }
    }

    contentItem: ButtonTextType {
        text: root.text

        color: root.connected && !root.inProgress ? AmneziaStyle.color.textAccent : AmneziaStyle.color.textPrimary
        lineHeight: 30 + LanguageUiController.getLineHeightAppend()
        font.pixelSize: 20
        font.weight: 700

        leftPadding: 16
        rightPadding: 16

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    onClicked: {
        ConnectionController.connectButtonClicked()
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
}
