import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

PageType {
    id: root

    Component.onCompleted: {
        PageController.disableTabBar(true)
        root.appendLog(qsTr("Removing…"), false)
    }
    Component.onDestruction: PageController.disableTabBar(false)

    property real targetProgress: 0.0
    property real displayProgress: 0.0

    onTargetProgressChanged: {
        if (root.displayProgress < root.targetProgress) {
            catchUpAnim.to = root.targetProgress
            catchUpAnim.restart()
        }
    }

    NumberAnimation {
        id: catchUpAnim
        target: root
        property: "displayProgress"
        duration: 800
        easing.type: Easing.OutCubic
    }

    Timer {
        id: driftTimer
        interval: 300
        repeat: true
        running: false
        onTriggered: {
            if (catchUpAnim.running) return
            var cap = Math.min(root.targetProgress + 0.15, 0.99)
            if (root.displayProgress < cap)
                root.displayProgress = Math.min(root.displayProgress + 0.001, cap)
        }
    }

    property int dotPhase: 1

    Timer {
        id: dotTimer
        interval: 500
        repeat: true
        running: true
        onTriggered: root.dotPhase = (root.dotPhase % 3) + 1
    }

    function appendLog(message, isError) {
        if (logModel.count > 0)
            logModel.setProperty(logModel.count - 1, "isLatest", false)
        logModel.append({ "msg": message, "isError": isError, "isLatest": true })
        root.dotPhase = 1
    }

    Connections {
        target: InstallController

        function onRemovalStepChanged(message, progress) {
            root.targetProgress = progress
            driftTimer.running = true
            root.appendLog(message, false)
        }

        function onInstallationErrorOccurred(errorCode) {
            root.appendLog(qsTr("Removal failed"), true)
        }
    }

    ListModel {
        id: logModel
    }

    SortFilterProxyModel {
        id: proxyServersModel
        sourceModel: ServersModel
        filters: [
            ValueFilter {
                roleName: "serverId"
                value: ServersUiController.processedServerId
            }
        ]
    }

    ListViewType {
        id: listView

        anchors.fill: parent

        model: proxyServersModel

        delegate: ColumnLayout {
            width: listView.width

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.topMargin: 20 + PageController.safeAreaTopMargin
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Removing services from %1").arg(name)
            }

            ProgressBarType {
                id: progressBar

                Layout.fillWidth: true
                Layout.preferredHeight: 6
                Layout.topMargin: 32
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                value: root.displayProgress
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 28
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.preferredHeight: 170

                color: AmneziaStyle.color.onyxBlack
                radius: 8
                layer.enabled: true

                HoverHandler {
                    cursorShape: Qt.ArrowCursor
                }

                WheelHandler {
                    onWheel: function(event) {
                        var ticks = -event.angleDelta.y / 120
                        var newY = logView.contentY + ticks * 36
                        logView.contentY = Math.max(0,
                            Math.min(Math.max(0, logView.contentHeight - logView.height), newY))
                        event.accepted = true
                    }
                }

                ListView {
                    id: logView

                    anchors.fill: parent
                    anchors.margins: 12

                    model: logModel
                    clip: true
                    spacing: 4
                    interactive: false

                    ScrollBar.vertical: ScrollBarType {}

                    onCountChanged: Qt.callLater(positionViewAtEnd)

                    delegate: Text {
                        width: logView.width

                        text: model.isLatest && model.msg.endsWith("…")
                              ? model.msg.slice(0, -1) + ".".repeat(root.dotPhase)
                              : model.msg
                        wrapMode: Text.WordWrap

                        font.pixelSize: 13
                        font.family: "PT Root UI VF"
                        font.bold: model.isLatest

                        color: model.isError
                               ? AmneziaStyle.color.vibrantRed
                               : model.isLatest
                                 ? AmneziaStyle.color.goldenApricot
                                 : AmneziaStyle.color.mutedGray

                        NumberAnimation on opacity {
                            from: 0
                            to: 1
                            duration: 150
                        }
                    }
                }
            }
        }
    }
}
