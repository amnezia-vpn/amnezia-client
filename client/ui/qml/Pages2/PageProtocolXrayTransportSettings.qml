import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    // Temporary local state — will be replaced by model roles
    property int selectedTransport: 0   // 0=RAW, 1=XHTTP, 2=mKCP

    // XHTTP fields
    property string xhttpMode: "Auto"
    property string xhttpHost: "www.googletagmanager.com"
    property string xhttpPath: ""
    property string xhttpHeadersTemplate: "HTTP"
    property string xhttpUplinkMethod: "POST"
    property bool   xhttpDisableGrpc: true
    property bool   xhttpDisableSse: true
    // Session & Sequence
    property string sessionPlacement: "Path"
    property string sessionKey: "Path"
    property string seqPlacement: "Path"
    property string seqKey: ""
    property string uplinkDataPlacement: "Body"
    property string uplinkDataKey: ""
    // Traffic Shaping
    property string uplinkChunkSize: "0"
    property string scMaxBufferedPosts: ""
    // mKCP fields
    property string mkcpTti: ""
    property string mkcpUplinkCapacity: ""
    property string mkcpDownlinkCapacity: ""
    property string mkcpReadBufferSize: ""
    property string mkcpWriteBufferSize: ""
    property bool   mkcpCongestion: true

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    FlickableType {
        id: flickable
        anchors.top: backButton.bottom
        anchors.bottom: saveButton.top
        anchors.left: parent.left
        anchors.right: parent.right
        contentHeight: mainColumn.implicitHeight

        ColumnLayout {
            id: mainColumn
            width: flickable.width
            spacing: 0

            // ── Header ────────────────────────────────────────────────
            Header2TextType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                text: qsTr("Transport")
            }

            // ── Radio: RAW (TCP) ──────────────────────────────────────
            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("RAW (TCP)")
                checked: root.selectedTransport === 0
                onClicked: root.selectedTransport = 0
            }

            DividerType {
            }

            // ── Radio: XHTTP ──────────────────────────────────────────
            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("XHTTP")
                descriptionText: qsTr("Advanced users")
                checked: root.selectedTransport === 1
                onClicked: root.selectedTransport = 1
            }

            DividerType {
            }

            // ── Radio: mKCP ───────────────────────────────────────────
            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("mKCP")
                checked: root.selectedTransport === 2
                onClicked: root.selectedTransport = 2
            }

            DividerType {
            }

            // ══════════════════════════════════════════════════════════
            // mKCP Settings (visible when mKCP selected)
            // ══════════════════════════════════════════════════════════
            ColumnLayout {
                visible: root.selectedTransport === 2
                Layout.fillWidth: true
                spacing: 0

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 24
                    Layout.bottomMargin: 8
                    text: qsTr("mKCP Settings")
                    color: AmneziaStyle.color.mutedGray
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("TTI")
                    textField.text: root.mkcpTti
                    textField.onEditingFinished: root.mkcpTti = textField.text
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("uplinkCapacity")
                    textField.text: root.mkcpUplinkCapacity
                    textField.onEditingFinished: root.mkcpUplinkCapacity = textField.text
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("downlinkCapacity")
                    textField.text: root.mkcpDownlinkCapacity
                    textField.onEditingFinished: root.mkcpDownlinkCapacity = textField.text
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("readBufferSize")
                    textField.text: root.mkcpReadBufferSize
                    textField.onEditingFinished: root.mkcpReadBufferSize = textField.text
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("writeBufferSize")
                    textField.text: root.mkcpWriteBufferSize
                    textField.onEditingFinished: root.mkcpWriteBufferSize = textField.text
                }

                SwitcherType {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    Layout.topMargin: 8
                    text: qsTr("Congestion")
                    checked: root.mkcpCongestion
                    onToggled: root.mkcpCongestion = checked
                }
            }

            // ══════════════════════════════════════════════════════════
            // XHTTP Settings (visible when XHTTP selected)
            // ══════════════════════════════════════════════════════════
            ColumnLayout {
                visible: root.selectedTransport === 1
                Layout.fillWidth: true
                spacing: 0

                // Mode dropdown
                DropDownType {
                    id: modeDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: root.xhttpMode
                    descriptionText: qsTr("Mode")
                    headerText: qsTr("Mode")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        id: modeListView
                        rootWidth: root.width
                        model: ListModel {
                            ListElement {
                                name: "Auto"
                            }
                            ListElement {
                                name: "Packet-up"
                            }
                            ListElement {
                                name: "Stream-up"
                            }
                            ListElement {
                                name: "Stream-one"
                            }
                        }
                        clickedFunction: function () {
                            root.xhttpMode = selectedText
                            modeDropDown.text = selectedText
                            modeDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === root.xhttpMode) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                // HTTP Profile label
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 24
                    Layout.bottomMargin: 8
                    text: qsTr("HTTP Profile")
                    color: AmneziaStyle.color.mutedGray
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("Host")
                    textField.text: root.xhttpHost
                    textField.onEditingFinished: root.xhttpHost = textField.text
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("Path")
                    textField.text: root.xhttpPath
                    textField.onEditingFinished: root.xhttpPath = textField.text
                }

                // Headers template dropdown
                DropDownType {
                    id: headersDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: root.xhttpHeadersTemplate
                    descriptionText: qsTr("Headers template")
                    headerText: qsTr("Headers template")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            ListElement {
                                name: "HTTP"
                            }
                            ListElement {
                                name: "None"
                            }
                        }
                        clickedFunction: function () {
                            root.xhttpHeadersTemplate = selectedText
                            headersDropDown.text = selectedText
                            headersDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === root.xhttpHeadersTemplate) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                // UplinkHTTPMethod dropdown
                DropDownType {
                    id: uplinkMethodDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: root.xhttpUplinkMethod
                    descriptionText: qsTr("UplinkHTTPMethod")
                    headerText: qsTr("UplinkHTTPMethod")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            ListElement {
                                name: "POST"
                            }
                            ListElement {
                                name: "PUT"
                            }
                            ListElement {
                                name: "PATCH"
                            }
                        }
                        clickedFunction: function () {
                            root.xhttpUplinkMethod = selectedText
                            uplinkMethodDropDown.text = selectedText
                            uplinkMethodDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === root.xhttpUplinkMethod) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                // Disable gRPC Header
                SwitcherType {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    Layout.topMargin: 16
                    text: qsTr("Disable gRPC Header")
                    descriptionText: qsTr("noGRPCHeader")
                    checked: root.xhttpDisableGrpc
                    onToggled: root.xhttpDisableGrpc = checked
                }

                DividerType {
                }

                // Disable SSE Header
                SwitcherType {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    text: qsTr("Disable SSE Header")
                    descriptionText: qsTr("noSSEHeader")
                    checked: root.xhttpDisableSse
                    onToggled: root.xhttpDisableSse = checked
                }

                DividerType {
                }

                // ── Session & Sequence ────────────────────────────────
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 24
                    Layout.bottomMargin: 8
                    text: qsTr("Session & Sequence")
                    color: AmneziaStyle.color.mutedGray
                }

                DropDownType {
                    id: sessionPlacementDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: root.sessionPlacement
                    descriptionText: qsTr("SessionPlacement")
                    headerText: qsTr("SessionPlacement")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            ListElement {
                                name: "Path"
                            }
                            ListElement {
                                name: "Header"
                            }
                            ListElement {
                                name: "Cookie"
                            }
                            ListElement {
                                name: "None"
                            }
                        }
                        clickedFunction: function () {
                            root.sessionPlacement = selectedText
                            sessionPlacementDropDown.text = selectedText
                            sessionPlacementDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === root.sessionPlacement) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                DropDownType {
                    id: sessionKeyDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: root.sessionKey
                    descriptionText: qsTr("SessionKey")
                    headerText: qsTr("SessionKey")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            ListElement {
                                name: "Path"
                            }
                            ListElement {
                                name: "Header"
                            }
                            ListElement {
                                name: "None"
                            }
                        }
                        clickedFunction: function () {
                            root.sessionKey = selectedText
                            sessionKeyDropDown.text = selectedText
                            sessionKeyDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === root.sessionKey) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                DropDownType {
                    id: seqPlacementDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: root.seqPlacement
                    descriptionText: qsTr("SeqPlacement")
                    headerText: qsTr("SeqPlacement")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            ListElement {
                                name: "Path"
                            }
                            ListElement {
                                name: "Header"
                            }
                            ListElement {
                                name: "Cookie"
                            }
                            ListElement {
                                name: "None"
                            }
                        }
                        clickedFunction: function () {
                            root.seqPlacement = selectedText
                            seqPlacementDropDown.text = selectedText
                            seqPlacementDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === root.seqPlacement) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("SeqKey")
                    textField.text: root.seqKey
                    textField.onEditingFinished: root.seqKey = textField.text
                }

                DropDownType {
                    id: uplinkDataPlacementDropDown
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: root.uplinkDataPlacement
                    descriptionText: qsTr("UplinkDataPlacement")
                    headerText: qsTr("UplinkDataPlacement")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            ListElement {
                                name: "Body"
                            }
                            ListElement {
                                name: "Query"
                            }
                        }
                        clickedFunction: function () {
                            root.uplinkDataPlacement = selectedText
                            uplinkDataPlacementDropDown.text = selectedText
                            uplinkDataPlacementDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === root.uplinkDataPlacement) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("UplinkDataKey")
                    textField.text: root.uplinkDataKey
                    textField.onEditingFinished: root.uplinkDataKey = textField.text
                }

                // ── Traffic Shaping ───────────────────────────────────
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 24
                    Layout.bottomMargin: 8
                    text: qsTr("Traffic Shaping")
                    color: AmneziaStyle.color.mutedGray
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("UplinkChunkSize")
                    textField.text: root.uplinkChunkSize
                    textField.validator: IntValidator {
                        bottom: 0
                    }
                    textField.onEditingFinished: root.uplinkChunkSize = textField.text
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("scMaxBufferedPosts")
                    textField.text: root.scMaxBufferedPosts
                    textField.onEditingFinished: root.scMaxBufferedPosts = textField.text
                }

                // scMaxEachPostBytes → nav row
                LabelWithButtonType {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    text: qsTr("scMaxEachPostBytes")
                    descriptionText: qsTr("1—100")
                    rightImageSource: "qrc:/images/controls/chevron-right.svg"
                    clickedFunction: function () { /* navigate */
                    }
                }
                DividerType {
                }

                // scMinPostsIntervalMs → nav row
                LabelWithButtonType {
                    Layout.fillWidth: true
                    text: qsTr("scMinPostsIntervalMs")
                    descriptionText: qsTr("100—600ms")
                    rightImageSource: "qrc:/images/controls/chevron-right.svg"
                    clickedFunction: function () { /* navigate */
                    }
                }
                DividerType {
                }

                // scStreamUpServerSecs → nav row
                LabelWithButtonType {
                    Layout.fillWidth: true
                    text: qsTr("scStreamUpServerSecs")
                    descriptionText: qsTr("1—100")
                    rightImageSource: "qrc:/images/controls/chevron-right.svg"
                    clickedFunction: function () { /* navigate */
                    }
                }
                DividerType {
                }

                // ── Padding and multiplexing ──────────────────────────
                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 24
                    Layout.bottomMargin: 8
                    text: qsTr("Padding and multiplexing")
                    color: AmneziaStyle.color.mutedGray
                }

                LabelWithButtonType {
                    Layout.fillWidth: true
                    text: qsTr("xPadding")
                    rightImageSource: "qrc:/images/controls/chevron-right.svg"
                    clickedFunction: function () {
                        PageController.goToPage(PageEnum.PageProtocolXrayXPaddingSettings)
                    }
                }
                DividerType {
                }

                LabelWithButtonType {
                    Layout.fillWidth: true
                    text: qsTr("XMux")
                    descriptionText: qsTr("On")
                    rightImageSource: "qrc:/images/controls/chevron-right.svg"
                    clickedFunction: function () {
                        PageController.goToPage(PageEnum.PageProtocolXrayXmuxSettings)
                    }
                }
                DividerType {
                }
            }

            Item {
                Layout.preferredHeight: 16
            }
        }
    }

    // ── Save button ───────────────────────────────────────────────────
    BasicButtonType {
        id: saveButton
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin
        anchors.leftMargin: 16
        anchors.rightMargin: 16

        text: qsTr("Save")
        onClicked: {
            forceActiveFocus()
            // XrayConfigModel.setTransport(...)
        }
        Keys.onEnterPressed: clicked()
        Keys.onReturnPressed: clicked()
    }
}
