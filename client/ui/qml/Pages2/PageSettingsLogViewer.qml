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
            id: exportButton

            icon.source: GC.isMobile() ? "qrc:/images/controls/share-2.svg" : "qrc:/images/controls/save.svg"
            enabled: root.hasText

            onClicked: {
                root.exportCurrentStream()
            }
        }

        IconButtonType {
            id: moreButton

            icon.source: "qrc:/images/controls/more-vertical.svg"

            onClicked: moreActionsMenu.openBelow(moreButton)
        }
    }

    PopupMenuType {
        id: moreActionsMenu

        PopupMenuItemType {
            text: qsTr("Refresh")
            enabled: !root.isLoading

            onClicked: {
                moreActionsMenu.close()
                root.reload()
            }
        }

        PopupMenuItemType {
            text: qsTr("Copy")
            enabled: root.hasText

            onClicked: {
                moreActionsMenu.close()
                LogsController.copyLoaded()
                PageController.showNotificationMessage(qsTr("Copied"))
            }
        }
    }

    Flickable {
        id: logFlickable

        anchors.top: navigationBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        clip: true
        visible: root.hasText

        contentWidth: width
        contentHeight: logCard.y + logCard.height + 24 + PageController.safeAreaBottomMargin
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBarType {}

        Rectangle {
            id: logCard

            x: 16
            width: logFlickable.width - 32
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
        anchors.top: logFlickable.top
        anchors.topMargin: 200
        anchors.horizontalCenter: logFlickable.horizontalCenter
        running: root.isLoading
        visible: root.isLoading
    }

    H3TextType {
        anchors.top: logFlickable.top
        anchors.topMargin: 200
        anchors.horizontalCenter: logFlickable.horizontalCenter

        font.weight: 400
        color: AmneziaStyle.color.textPrimary

        visible: !root.isLoading && logText.length === 0

        text: qsTr("No logs yet")
    }
}
