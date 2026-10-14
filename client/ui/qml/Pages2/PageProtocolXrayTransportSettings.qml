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

    ListViewType {
        id: listView

        anchors.top: parent.top
        anchors.bottom: saveButton.visible ? saveButton.top : parent.bottom
        anchors.bottomMargin: saveButton.visible ? 16 : 0
        anchors.left: parent.left
        anchors.right: parent.right

        enabled: ServersUiController.isProcessedServerHasWriteAccess()
        model: XrayConfigModel

        header: NavigationBarType {
            width: listView.width

            title: qsTr("Transport")
        }

        delegate: ColumnLayout {
            width: listView.width
            spacing: 0

            RadioItemType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("RAW (TCP)")
                checked: transport === "raw"
                onToggled: if (checked && transport !== "raw") transport = "raw"
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("XHTTP")
                subtitle: qsTr("Advanced users")
                checked: transport === "xhttp"
                onToggled: if (checked && transport !== "xhttp") transport = "xhttp"
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("mKCP")
                checked: transport === "mkcp"
                onToggled: if (checked && transport !== "mkcp") transport = "mkcp"
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: transport === "mkcp"
                spacing: 12

                SmallTextType {
                    Layout.fillWidth: true
                    text: qsTr("mKCP Settings")
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("TTI")
                    helperText: qsTr("Transmission time interval (ms). Valid range: 10–100.")
                    placeholderText: XrayConfigModel.mkcpDefaultTti()
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

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("uplinkCapacity")
                    helperText: qsTr("Uplink capacity (MB/s). Maximum: 2147483647.")
                    placeholderText: XrayConfigModel.mkcpDefaultUplinkCapacity()
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

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("downlinkCapacity")
                    helperText: qsTr("Downlink capacity (MB/s). Maximum: 2147483647.")
                    placeholderText: XrayConfigModel.mkcpDefaultDownlinkCapacity()
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

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("mtu")
                    helperText: qsTr("Maximum packet size in bytes. Range: 576–1460.")
                    placeholderText: XrayConfigModel.mkcpDefaultMtu()
                    textField.text: mkcpMtu
                    textField.maximumLength: 4
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== mkcpMtu)
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 576, 1460)
                        if (v !== mkcpMtu) mkcpMtu = v
                        else if (textField.text !== v) textField.text = v
                        if (mkcpMaxSendingWindow !== "") {
                            var floorNow = parseInt(v !== "" ? v : XrayConfigModel.mkcpDefaultMtu(), 10)
                            if (parseInt(mkcpMaxSendingWindow, 10) < floorNow) {
                                mkcpMaxSendingWindow = String(floorNow)
                            }
                        }
                        root.editDirty = false
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("cwndMultiplier")
                    helperText: qsTr("Congestion window multiplier. Minimum: 1.")
                    placeholderText: XrayConfigModel.mkcpDefaultCwndMultiplier()
                    textField.text: mkcpCwndMultiplier
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== mkcpCwndMultiplier)
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 1, 2147483647)
                        if (v !== mkcpCwndMultiplier) mkcpCwndMultiplier = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("maxSendingWindow")
                    helperText: qsTr("Send window in bytes. Must not be smaller than mtu. Leave empty to let Xray decide.")
                    textField.text: mkcpMaxSendingWindow
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== mkcpMaxSendingWindow)
                    textField.onEditingFinished: {
                        var mtuFloor = parseInt(mkcpMtu !== "" ? mkcpMtu : XrayConfigModel.mkcpDefaultMtu(), 10)
                        var v = root.clampInt(textField.text, mtuFloor, 2147483647)
                        if (v !== mkcpMaxSendingWindow) mkcpMaxSendingWindow = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: transport === "xhttp"
                spacing: 12

                DropDownType {
                    id: modeDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: xhttpMode
                    descriptionText: qsTr("Mode")
                    headerText: qsTr("Mode")
                    listView: RadioListType {
                        currentValue: xhttpMode
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpModeOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            xhttpMode = selectedText
                            modeDropDown.text = selectedText
                            modeDropDown.close()
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

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("HTTP Profile")
                }

                TextFieldType {
                    id: hostField
                    Layout.fillWidth: true
                    headerText: qsTr("Host")
                    placeholderText: XrayConfigModel.xhttpHostDefault()
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

                TextFieldType {
                    id: pathField
                    Layout.fillWidth: true
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
                    id: uplinkMethodDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: xhttpUplinkMethod
                    descriptionText: qsTr("UplinkHTTPMethod")
                    headerText: qsTr("UplinkHTTPMethod")
                    listView: RadioListType {
                        currentValue: xhttpUplinkMethod
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpUplinkMethodOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            xhttpUplinkMethod = selectedText
                            uplinkMethodDropDown.text = selectedText
                            uplinkMethodDropDown.close()
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

                SwitchRowType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("Disable gRPC Header")
                    descriptionText: qsTr("noGRPCHeader")
                    checked: xhttpDisableGrpc
                    onToggled: xhttpDisableGrpc = checked
                }

                SwitchRowType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("Disable SSE Header")
                    descriptionText: qsTr("noSSEHeader")
                    checked: xhttpDisableSse
                    onToggled: xhttpDisableSse = checked
                }

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("Session & Sequence")
                }

                DropDownType {
                    id: sessionPlacementDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: xhttpSessionPlacement
                    descriptionText: qsTr("SessionPlacement")
                    headerText: qsTr("SessionPlacement")
                    listView: RadioListType {
                        currentValue: xhttpSessionPlacement
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpSessionPlacementOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            xhttpSessionPlacement = selectedText
                            sessionPlacementDropDown.text = selectedText
                            sessionPlacementDropDown.close()
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

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("SessionKey")
                    textField.text: xhttpSessionKey
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9_-]*$/ }
                    textField.onTextEdited: root.editDirty = (textField.text !== xhttpSessionKey)
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== xhttpSessionKey) xhttpSessionKey = v
                        else if (textField.text !== v) textField.text = v
                        root.editDirty = false
                    }
                }

                DropDownType {
                    id: seqPlacementDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: xhttpSeqPlacement
                    descriptionText: qsTr("SeqPlacement")
                    headerText: qsTr("SeqPlacement")
                    listView: RadioListType {
                        currentValue: xhttpSeqPlacement
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpSeqPlacementOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            xhttpSeqPlacement = selectedText
                            seqPlacementDropDown.text = selectedText
                            seqPlacementDropDown.close()
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

                TextFieldType {
                    Layout.fillWidth: true
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
                    text: xhttpUplinkDataPlacement
                    descriptionText: qsTr("Where the uploaded data is carried in the request")
                    headerText: qsTr("UplinkDataPlacement")
                    listView: RadioListType {
                        currentValue: xhttpUplinkDataPlacement
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpUplinkDataPlacementOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            xhttpUplinkDataPlacement = selectedText
                            uplinkDataPlacementDropDown.text = selectedText
                            uplinkDataPlacementDropDown.close()
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

                TextFieldType {
                    Layout.fillWidth: true
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

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("Traffic Shaping")
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("UplinkChunkSize")
                    helperText: qsTr("Uplink chunk size in bytes. Maximum: 2147483647. 0 = off.")
                    placeholderText: XrayConfigModel.xhttpUplinkChunkSizeDefault()
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

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("scMaxBufferedPosts")
                    helperText: qsTr("Max buffered POSTs. Range: 0–2147483647.")
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

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("scMaxEachPostBytes")
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    minValue: xhttpScMaxEachPostBytesMin
                    maxValue: xhttpScMaxEachPostBytesMax
                    minPlaceholder: XrayConfigModel.scMaxEachPostBytesMinDefault()
                    maxPlaceholder: XrayConfigModel.scMaxEachPostBytesMaxDefault()
                    onMinChanged: function(val) { xhttpScMaxEachPostBytesMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xhttpScMaxEachPostBytesMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
                }

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("scStreamUpServerSecs")
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    minValue: xhttpScStreamUpServerSecsMin
                    maxValue: xhttpScStreamUpServerSecsMax
                    minPlaceholder: XrayConfigModel.scStreamUpServerSecsMinDefault()
                    maxPlaceholder: XrayConfigModel.scStreamUpServerSecsMaxDefault()
                    onMinChanged: function(val) { xhttpScStreamUpServerSecsMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xhttpScStreamUpServerSecsMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
                }

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("scMinPostsIntervalMs")
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    minValue: xhttpScMinPostsIntervalMsMin
                    maxValue: xhttpScMinPostsIntervalMsMax
                    minPlaceholder: XrayConfigModel.scMinPostsIntervalMsMinDefault()
                    maxPlaceholder: XrayConfigModel.scMinPostsIntervalMsMaxDefault()
                    onMinChanged: function(val) { xhttpScMinPostsIntervalMsMin = val; root.editDirty = false }
                    onMaxChanged: function(val) { xhttpScMinPostsIntervalMsMax = val; root.editDirty = false }
                    onEdited: root.editDirty = true
                }

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("Padding and multiplexing")
                }

                ColumnLayout {
                    Layout.fillWidth: true

                    spacing: 0

                    ListItemType {
                        Layout.fillWidth: true
                        text: qsTr("xPadding")
                        trailingIconSource: "qrc:/images/controls/chevron-right.svg"
                        onClicked: {
                            PageController.goToPage(PageEnum.PageProtocolXrayXPaddingSettings)
                        }
                    }

                    ListItemType {
                        Layout.fillWidth: true
                        text: qsTr("XMux")
                        subtitle: xmuxEnabled ? qsTr("On") : qsTr("Off")
                        trailingIconSource: "qrc:/images/controls/chevron-right.svg"
                        onClicked: {
                            PageController.goToPage(PageEnum.PageProtocolXrayXmuxSettings)
                        }
                    }
                }
            }

            Item {
                Layout.preferredHeight: saveButton.visible ? 0 : 16 + PageController.safeAreaBottomMargin
            }
        }
    }

    ButtonType {
        id: saveButton

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 16 + PageController.safeAreaBottomMargin

        visible: listView.enabled && (XrayConfigModel.hasUnsavedChanges || root.editDirty)
        enabled: visible
        text: qsTr("Save")
        onClicked: {
            saveButton.forceActiveFocus()
            var errs = XrayConfigModel.validationErrors()
            if (errs.length > 0) {
                PageController.showErrorMessage(errs.join("\n"))
                return
            }
            var headerText = qsTr("Save settings?")
            var descriptionText = XrayConfigModel.saveDescription("")
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
