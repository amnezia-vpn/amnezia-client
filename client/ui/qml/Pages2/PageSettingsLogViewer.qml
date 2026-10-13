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

    Rectangle {
        anchors.fill: parent
        color: AmneziaStyle.color.backgroundBase
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    RowLayout {
        id: actionsRow

        anchors.verticalCenter: backButton.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 16

        spacing: 4

        ImageButtonType {
            id: refreshButton

            implicitWidth: 36
            implicitHeight: 36
            icon.width: 20
            icon.height: 20

            image: "qrc:/images/controls/refresh-cw.svg"
            imageColor: AmneziaStyle.color.surfaceInverse
            disableImageColor: AmneziaStyle.color.charcoalGray
            enabled: !root.isLoading

            onClicked: {
                root.reload()
            }
        }

        ImageButtonType {
            id: copyButton

            implicitWidth: 36
            implicitHeight: 36
            icon.width: 20
            icon.height: 20

            image: "qrc:/images/controls/copy.svg"
            imageColor: AmneziaStyle.color.surfaceInverse
            disableImageColor: AmneziaStyle.color.charcoalGray
            enabled: root.hasText

            onClicked: {
                LogsController.copyLoaded()
                PageController.showNotificationMessage(qsTr("Copied"))
            }
        }

        ImageButtonType {
            id: exportButton

            implicitWidth: 36
            implicitHeight: 36
            icon.width: 20
            icon.height: 20

            image: GC.isMobile() ? "qrc:/images/controls/share-2.svg" : "qrc:/images/controls/save.svg"
            imageColor: AmneziaStyle.color.surfaceInverse
            disableImageColor: AmneziaStyle.color.charcoalGray
            enabled: root.hasText

            onClicked: {
                root.exportCurrentStream()
            }
        }
    }

    AppH1TextType {
        id: title

        anchors.top: backButton.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 12
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        maximumLineCount: 1
        elide: Text.ElideRight

        text: root.isAppStream ? qsTr("Application") : qsTr("Tunnel")
    }

    Flickable {
        id: logFlickable

        anchors.top: title.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 16
        anchors.rightMargin: 1

        clip: true
        visible: root.hasText

        contentWidth: width
        contentHeight: logCard.height + 16 + PageController.safeAreaBottomMargin
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBarType {}

        Rectangle {
            id: logCard

            x: 16
            width: logFlickable.width - 31
            height: logText.height + 32

            radius: 16
            color: AmneziaStyle.color.surfaceBase

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
                selectionColor: AmneziaStyle.color.richBrown
                selectedTextColor: AmneziaStyle.color.textPrimary

                font.family: Qt.platform.os === "ios" || Qt.platform.os === "osx" ? "Menlo" : "monospace"
                font.pixelSize: 12
            }
        }
    }

    BusyIndicator {
        anchors.centerIn: logFlickable
        running: root.isLoading
        visible: root.isLoading
    }

    AppTextType {
        anchors.centerIn: logFlickable

        visible: !root.isLoading && logText.length === 0

        text: qsTr("No logs yet")
    }
}
