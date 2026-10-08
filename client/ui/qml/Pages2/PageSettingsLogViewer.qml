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
    property bool exportIsShare: false

    readonly property bool isAppStream: LogsController.viewerStream === "app"

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

    function runExport(isShare, exportFunction) {
        root.exportPending = true
        root.exportIsShare = isShare
        exportFunction()
    }

    function exportCurrentStream() {
        var stream = LogsController.viewerStream

        if (LogsController.canShare) {
            var saveFunction = function() {
                root.runExport(false, function() {
                    LogsController.exportStream(stream, LogsController.defaultFileName(stream))
                })
            }
            var shareFunction = function() {
                root.runExport(true, function() {
                    LogsController.shareStream(stream)
                })
            }
            showQuestionDrawer(qsTr("Save log"),
                               qsTr("Save the log on this device or send it to another app."),
                               qsTr("Save to file…"), qsTr("Share…"),
                               saveFunction, shareFunction)
            return
        }

        var fileName = ""
        if (GC.isMobile()) {
            fileName = LogsController.defaultFileName(stream)
        } else {
            fileName = SystemController.getFileName(qsTr("Save"),
                                                    qsTr("Logs files (*.log)"),
                                                    StandardPaths.standardLocations(StandardPaths.DocumentsLocation) + "/" + LogsController.defaultFileName(stream),
                                                    true,
                                                    ".log")
        }
        if (fileName !== "") {
            root.runExport(false, function() {
                LogsController.exportStream(stream, fileName)
            })
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

        function onExportFinished(success) {
            if (!root.exportPending) {
                return
            }
            root.exportPending = false
            if (!success) {
                PageController.showNotificationMessage(qsTr("Failed to save logs"))
            } else if (!root.exportIsShare) {
                PageController.showNotificationMessage(qsTr("Logs file saved"))
            }
        }
    }

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    RowLayout {
        id: headerRow

        anchors.top: backButton.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        spacing: 4

        BaseHeaderType {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            headerText: root.isAppStream ? qsTr("Application") : qsTr("Tunnel")
            headerTextMaximumLineCount: 1
        }

        ImageButtonType {
            id: refreshButton

            implicitWidth: 40
            implicitHeight: 40

            image: "qrc:/images/controls/refresh-cw.svg"
            imageColor: AmneziaStyle.color.paleGray
            enabled: !root.isLoading

            onClicked: {
                root.reload()
            }
        }

        ImageButtonType {
            id: copyButton

            implicitWidth: 40
            implicitHeight: 40

            image: "qrc:/images/controls/copy.svg"
            imageColor: AmneziaStyle.color.paleGray
            enabled: !root.isLoading

            onClicked: {
                LogsController.copyLoaded()
                PageController.showNotificationMessage(qsTr("Copied"))
            }
        }

        ImageButtonType {
            id: exportButton

            implicitWidth: 40
            implicitHeight: 40

            image: GC.isMobile() ? "qrc:/images/controls/share-2.svg" : "qrc:/images/controls/save.svg"
            imageColor: AmneziaStyle.color.paleGray

            onClicked: {
                root.exportCurrentStream()
            }
        }
    }

    Flickable {
        id: logFlickable

        anchors.top: headerRow.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 16
        anchors.leftMargin: 16
        anchors.rightMargin: 1
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin

        clip: true
        visible: !root.isLoading && logText.length > 0

        contentWidth: width
        contentHeight: logText.height
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBarType {}

        TextEdit {
            id: logText

            width: logFlickable.width - 15

            readOnly: true
            textFormat: TextEdit.PlainText
            selectByMouse: true
            persistentSelection: true
            wrapMode: TextEdit.WrapAnywhere

            color: AmneziaStyle.color.paleGray
            selectionColor: AmneziaStyle.color.richBrown
            selectedTextColor: AmneziaStyle.color.paleGray

            font.family: Qt.platform.os === "ios" || Qt.platform.os === "osx" ? "Menlo" : "monospace"
            font.pixelSize: 11
        }
    }

    BusyIndicator {
        anchors.centerIn: logFlickable
        running: root.isLoading
        visible: root.isLoading
    }

    ParagraphTextType {
        anchors.centerIn: logFlickable

        visible: !root.isLoading && logText.length === 0

        color: AmneziaStyle.color.mutedGray
        text: qsTr("Empty")
    }
}
