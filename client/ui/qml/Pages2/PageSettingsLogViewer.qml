import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtCore

import PageEnum 1.0
import Style 1.0

import "../Controls2"
import "../Config"
import "../Components"
import "../Controls2/TextTypes"

PageType {
    id: root

    property bool isLoading: false

    property bool exportPending: false

    readonly property bool isAppStream: LogsController.viewerStream === "app"
    readonly property bool hasText: !root.isLoading && logText.length > 0

    function reload() {
        root.isLoading = true
        LogsController.load(LogsController.viewerStream)
    }

    function isScrolledToBottom() {
        return logFlickable.contentY >= logFlickable.contentHeight - logFlickable.height - 4
    }

    function scrollToBottom() {
        logFlickable.contentY = Math.max(0, logFlickable.contentHeight - logFlickable.height)
    }

    function exportCurrentStream() {
        var stream = LogsController.viewerStream

        if (Qt.platform.os === "android") {
            root.exportPending = true
            LogsController.shareStream(stream)
            return
        }

        var fileName = LogsController.defaultFileName(stream)
        if (!GC.isMobile()) {
            fileName = SystemController.getFileName(qsTr("Save"),
                                                    qsTr("Logs files (*.log)"),
                                                    StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/" + fileName,
                                                    true,
                                                    ".log")
        }
        if (fileName !== "") {
            root.exportPending = true
            LogsController.exportStream(stream, fileName)
        }
    }

    Component.onCompleted: {
        root.reload()
        if (root.isAppStream) {
            LogsController.startLiveTail()
        }
    }

    Component.onDestruction: {
        LogsController.stopLiveTail()
    }

    Connections {
        target: LogsController

        function onStreamLoaded(stream, text) {
            if (stream !== LogsController.viewerStream) {
                return
            }
            logText.text = text
            root.isLoading = false
            Qt.callLater(root.scrollToBottom)
        }

        function onAppended(text) {
            if (!root.isAppStream || root.isLoading) {
                return
            }
            var followBottom = root.isScrolledToBottom()
            logText.insert(logText.length, text)
            if (followBottom) {
                Qt.callLater(root.scrollToBottom)
            }
        }

        function onReloadRequired() {
            root.reload()
        }

        function onBusyChanged() {
            PageController.showBusyIndicator(LogsController.busy)
        }

        function onExportFinished(success, cancelled) {
            if (!root.exportPending) {
                return
            }
            root.exportPending = false
            if (!success) {
                if (!cancelled) {
                    PageController.showNotificationMessage(qsTr("Failed to save logs. Try again later"))
                }
            } else if (!GC.isMobile()) {
                PageController.showNotificationMessage(qsTr("Logs file saved"))
            }
        }
    }

    NavigationBarType {
        id: navigationBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        title: root.isAppStream ? qsTr("Application") : qsTr("Tunnel")

        IconButtonType {
            id: refreshButton

            size: IconButtonType.Small

            icon.source: "qrc:/images/controls/refresh-cw.svg"
            enabled: !root.isLoading

            onClicked: {
                root.reload()
            }
        }

        IconButtonType {
            id: copyButton

            size: IconButtonType.Small

            icon.source: "qrc:/images/controls/copy.svg"
            enabled: root.hasText

            onClicked: {
                LogsController.copyLoaded()
                PageController.showNotificationMessage(qsTr("Copied"))
            }
        }

        IconButtonType {
            id: exportButton

            size: IconButtonType.Small

            icon.source: GC.isMobile() ? "qrc:/images/controls/share-2.svg" : "qrc:/images/controls/save.svg"
            enabled: root.hasText

            onClicked: {
                root.exportCurrentStream()
            }
        }
    }

    Flickable {
        id: logFlickable

        anchors.top: navigationBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 1

        clip: true
        visible: root.hasText

        contentWidth: width
        contentHeight: logCard.y + logCard.height + 16 + PageController.safeAreaBottomMargin
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBarType {}

        Rectangle {
            id: logCard

            x: 16
            y: 16
            width: logFlickable.width - 31
            height: logText.height + 32

            radius: 16
            color: AmneziaStyle.color.bgSurface

            TextEdit {
                id: logText

                x: 16
                y: 16
                width: logCard.width - 32

                readOnly: true
                textFormat: TextEdit.PlainText
                selectByMouse: true
                persistentSelection: true
                wrapMode: TextEdit.Wrap

                color: AmneziaStyle.color.textPrimary
                selectionColor: AmneziaStyle.color.bgStateLayerAccent
                selectedTextColor: AmneziaStyle.color.textPrimary

                font.family: Qt.platform.os === "ios" || Qt.platform.os === "osx" ? "Menlo" : "monospace"
                font.pixelSize: 12
            }
        }
    }

    SpinnerType {
        anchors.centerIn: logFlickable
        size: SpinnerType.Large
        running: root.isLoading
        visible: root.isLoading
    }

    BodyTextType {
        color: AmneziaStyle.color.textPrimary
        anchors.centerIn: logFlickable

        visible: !root.isLoading && logText.length === 0

        text: qsTr("No logs yet")
    }
}
