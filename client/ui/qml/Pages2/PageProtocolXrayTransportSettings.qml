import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import ProtocolEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    property bool editDirty: false

    function clampInt(text, lo, hi) {
        if (text === "")
            return ""
        var n = parseInt(text, 10)
        if (isNaN(n))
            return ""
        if (n < lo)
            n = lo
        if (n > hi)
            n = hi
        return String(n)
    }

    BackButtonType {
        id: backButton
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin
    }

    ListViewType {
        id: listView
        anchors.top: backButton.bottom
        anchors.bottom: saveButton.visible ? saveButton.top : parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        enabled: ServersUiController.isProcessedServerHasWriteAccess()
        model: XrayConfigModel

        delegate: ColumnLayout {
            width: listView.width
            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 0
                Layout.bottomMargin: 24
                headerText: qsTr("Transport")
            }

            // ── Radio buttons ─────────────────────────────────────────
            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("RAW (TCP)")
                checked: transport === "raw"
                onToggled: if (checked && transport !== "raw") transport = "raw"
            }

            DividerType {
            }

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("XHTTP")
                descriptionText: qsTr("Advanced users")
                checked: transport === "xhttp"
                onToggled: if (checked && transport !== "xhttp") transport = "xhttp"
            }

            DividerType {
            }

            VerticalRadioButton {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("mKCP")
                checked: transport === "mkcp"
                onToggled: if (checked && transport !== "mkcp") transport = "mkcp"
            }

            DividerType {
            }

            // ══════════════════════════════════════════════════════════
            // mKCP Settings
            // ══════════════════════════════════════════════════════════
            ColumnLayout {
                visible: transport === "mkcp"
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
                    subtitleText: qsTr("Range 10–100, default %1 ms", "mKCP TTI").arg(XrayConfigModel.mkcpDefaultTti())
                    textField.text: mkcpTti
                    textField.maximumLength: 3
                    textField.validator: RegularExpressionValidator { regularExpression: /^(|\d{1,2}|100)$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== mkcpTti)
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 10, 100)
                        if (v !== mkcpTti) mkcpTti = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("uplinkCapacity")
                    subtitleText: qsTr("≥ 0, default %1 MB/s", "mKCP uplink").arg(XrayConfigModel.mkcpDefaultUplinkCapacity())
                    textField.text: mkcpUplinkCapacity
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== mkcpUplinkCapacity)
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 0, 2147483647)
                        if (v !== mkcpUplinkCapacity) mkcpUplinkCapacity = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("downlinkCapacity")
                    subtitleText: qsTr("≥ 0, default %1 MB/s", "mKCP downlink").arg(XrayConfigModel.mkcpDefaultDownlinkCapacity())
                    textField.text: mkcpDownlinkCapacity
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== mkcpDownlinkCapacity)
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 0, 2147483647)
                        if (v !== mkcpDownlinkCapacity) mkcpDownlinkCapacity = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("readBufferSize")
                    subtitleText: qsTr("≥ 1, default %1 MB").arg(XrayConfigModel.mkcpDefaultReadBufferSize())
                    textField.text: mkcpReadBufferSize
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== mkcpReadBufferSize)
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 1, 2147483647)
                        if (v !== mkcpReadBufferSize) mkcpReadBufferSize = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("writeBufferSize")
                    subtitleText: qsTr("≥ 1, default %1 MB").arg(XrayConfigModel.mkcpDefaultWriteBufferSize())
                    textField.text: mkcpWriteBufferSize
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== mkcpWriteBufferSize)
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 1, 2147483647)
                        if (v !== mkcpWriteBufferSize) mkcpWriteBufferSize = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                SwitcherType {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    Layout.topMargin: 8
                    text: qsTr("Congestion")
                    checked: mkcpCongestion
                    onToggled: mkcpCongestion = checked
                }
            }

            // ══════════════════════════════════════════════════════════
            // XHTTP Settings
            // ══════════════════════════════════════════════════════════
            ColumnLayout {
                visible: transport === "xhttp"
                Layout.fillWidth: true
                spacing: 0

                DropDownType {
                    id: modeDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    Layout.topMargin: 16
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: xhttpMode
                    descriptionText: qsTr("Mode")
                    headerText: qsTr("Mode")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpModeOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        clickedFunction: function () {
                            xhttpMode = selectedText
                            modeDropDown.text = selectedText
                            modeDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === xhttpMode) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            modeDropDown.text = xhttpMode
                        }
                    }
                }

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
                    id: hostField
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("Host")
                    textField.text: xhttpHost
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9._:,-]*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== xhttpHost)
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== xhttpHost) xhttpHost = v
                        else if (textField.text !== v) textField.text = v
                        hostField.errorText = XrayConfigModel.isValidHost(v) ? "" : qsTr("Enter a valid IP address or domain name")
                        root.editDirty = false
                    }
                }

                TextFieldWithHeaderType {
                    id: pathField
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("Path")
                    textField.text: xhttpPath
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9\-._~:\/?#\[\]@!$&'()*+,;=%]*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== xhttpPath)
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== xhttpPath) xhttpPath = v
                        else if (textField.text !== v) textField.text = v
                        pathField.errorText = XrayConfigModel.isValidPath(v) ? "" : qsTr("Path must start with \"/\"")
                        root.editDirty = false
                    }
                }

                DropDownType {
                    id: headersDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: xhttpHeadersTemplate
                    descriptionText: qsTr("Headers template")
                    headerText: qsTr("Headers template")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpHeadersTemplateOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        clickedFunction: function () {
                            xhttpHeadersTemplate = selectedText
                            headersDropDown.text = selectedText
                            headersDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === xhttpHeadersTemplate) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            headersDropDown.text = xhttpHeadersTemplate
                        }
                    }
                }

                DropDownType {
                    id: uplinkMethodDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: xhttpUplinkMethod
                    descriptionText: qsTr("UplinkHTTPMethod")
                    headerText: qsTr("UplinkHTTPMethod")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpUplinkMethodOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        clickedFunction: function () {
                            xhttpUplinkMethod = selectedText
                            uplinkMethodDropDown.text = selectedText
                            uplinkMethodDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === xhttpUplinkMethod) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            uplinkMethodDropDown.text = xhttpUplinkMethod
                        }
                    }
                }

                SwitcherType {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    Layout.topMargin: 16
                    text: qsTr("Disable gRPC Header")
                    descriptionText: qsTr("noGRPCHeader")
                    checked: xhttpDisableGrpc
                    onToggled: xhttpDisableGrpc = checked
                }

                DividerType {
                }

                SwitcherType {
                    Layout.fillWidth: true
                    Layout.margins: 16
                    text: qsTr("Disable SSE Header")
                    descriptionText: qsTr("noSSEHeader")
                    checked: xhttpDisableSse
                    onToggled: xhttpDisableSse = checked
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
                    fitContent: true
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: xhttpSessionPlacement
                    descriptionText: qsTr("SessionPlacement")
                    headerText: qsTr("SessionPlacement")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpSessionPlacementOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        clickedFunction: function () {
                            xhttpSessionPlacement = selectedText
                            sessionPlacementDropDown.text = selectedText
                            sessionPlacementDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === xhttpSessionPlacement) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            sessionPlacementDropDown.text = xhttpSessionPlacement
                        }
                    }
                }

                DropDownType {
                    id: sessionKeyDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: xhttpSessionKey
                    descriptionText: qsTr("SessionKey")
                    headerText: qsTr("SessionKey")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpSessionKeyOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        clickedFunction: function () {
                            xhttpSessionKey = selectedText
                            sessionKeyDropDown.text = selectedText
                            sessionKeyDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === xhttpSessionKey) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            sessionKeyDropDown.text = xhttpSessionKey
                        }
                    }
                }

                DropDownType {
                    id: seqPlacementDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: xhttpSeqPlacement
                    descriptionText: qsTr("SeqPlacement")
                    headerText: qsTr("SeqPlacement")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpSeqPlacementOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        clickedFunction: function () {
                            xhttpSeqPlacement = selectedText
                            seqPlacementDropDown.text = selectedText
                            seqPlacementDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === xhttpSeqPlacement) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            seqPlacementDropDown.text = xhttpSeqPlacement
                        }
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("SeqKey")
                    textField.text: xhttpSeqKey
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9_-]*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== xhttpSeqKey)
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== xhttpSeqKey) xhttpSeqKey = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                DropDownType {
                    id: uplinkDataPlacementDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    text: xhttpUplinkDataPlacement
                    descriptionText: qsTr("UplinkDataPlacement")
                    headerText: qsTr("UplinkDataPlacement")
                    drawerParent: root
                    listView: ListViewWithRadioButtonType {
                        rootWidth: root.width
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpUplinkDataPlacementOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        clickedFunction: function () {
                            xhttpUplinkDataPlacement = selectedText
                            uplinkDataPlacementDropDown.text = selectedText
                            uplinkDataPlacementDropDown.closeTriggered()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === xhttpUplinkDataPlacement) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                    Connections {
                        target: XrayConfigModel

                        function onDataChanged() {
                            uplinkDataPlacementDropDown.text = xhttpUplinkDataPlacement
                        }
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("UplinkDataKey")
                    textField.text: xhttpUplinkDataKey
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9_-]*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== xhttpUplinkDataKey)
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== xhttpUplinkDataKey) xhttpUplinkDataKey = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
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
                    subtitleText: qsTr("≥ 0 (0 = off)")
                    textField.text: xhttpUplinkChunkSize
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== xhttpUplinkChunkSize)
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 0, 2147483647)
                        if (v !== xhttpUplinkChunkSize) xhttpUplinkChunkSize = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                TextFieldWithHeaderType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 8
                    headerText: qsTr("scMaxBufferedPosts")
                    subtitleText: qsTr("≥ 0")
                    textField.text: xhttpScMaxBufferedPosts
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== xhttpScMaxBufferedPosts)
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 0, 2147483647)
                        if (v !== xhttpScMaxBufferedPosts) xhttpScMaxBufferedPosts = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("scMaxEachPostBytes")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: xhttpScMaxEachPostBytesMin
                    maxValue: xhttpScMaxEachPostBytesMax
                    onMinChanged: function(val) { xhttpScMaxEachPostBytesMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xhttpScMaxEachPostBytesMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("scStreamUpServerSecs")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: xhttpScStreamUpServerSecsMin
                    maxValue: xhttpScStreamUpServerSecsMax
                    onMinChanged: function(val) { xhttpScStreamUpServerSecsMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xhttpScStreamUpServerSecsMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
                }

                CaptionTextType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 16
                    Layout.bottomMargin: 8
                    text: qsTr("scMinPostsIntervalMs")
                    color: AmneziaStyle.color.mutedGray
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    minValue: xhttpScMinPostsIntervalMsMin
                    maxValue: xhttpScMinPostsIntervalMsMax
                    onMinChanged: function(val) { xhttpScMinPostsIntervalMsMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xhttpScMinPostsIntervalMsMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
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
                    descriptionText: xmuxEnabled ? qsTr("On") : qsTr("Off")
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

    BasicButtonType {
        id: saveButton

        anchors.left: root.left
        anchors.right: root.right
        anchors.bottom: root.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin

        visible: listView.enabled && (XrayConfigModel.hasUnsavedChanges || root.editDirty)
        enabled: visible
        text: qsTr("Save")
        clickedFunc: function () {
            var errs = XrayConfigModel.validationErrors()
            if (errs.length > 0) {
                PageController.showErrorMessage(errs.join("\n"))
                return
            }
            var headerText = qsTr("Save settings?")
            var descriptionText = qsTr("All users with whom you shared a connection with will no longer be able to connect to it.")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")
            var yesButtonFunction = function () {
                if (ConnectionController.isConnected && ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId) === ServersUiController.processedContainerIndex) {
                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                    return
                }
                PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.Xray)
            }
            var noButtonFunction = function () {
                if (typeof GC !== "undefined" && !GC.isMobile()) {
                    saveButton.forceActiveFocus()
                }
            }
            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
