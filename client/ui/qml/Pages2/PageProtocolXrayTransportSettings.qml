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

    readonly property bool packetUpEffective: {
        var m = XrayConfigModel.xhttpMode.toLowerCase()
        return m === "packet-up" || ((m === "" || m === "auto") && XrayConfigModel.security !== "reality")
    }

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

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: saveButton.visible ? saveButton.top : parent.bottom
        anchors.bottomMargin: saveButton.visible ? 24 : 0

        enabled: ServersUiController.isProcessedServerHasWriteAccess()

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("Transport")
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("RAW (TCP)")
                checked: XrayConfigModel.transport === "raw"
                onClicked: if (XrayConfigModel.transport !== "raw") XrayConfigModel.transport = "raw"
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("XHTTP")
                subtitle: qsTr("Advanced users")
                checked: XrayConfigModel.transport === "xhttp"
                onClicked: if (XrayConfigModel.transport !== "xhttp") XrayConfigModel.transport = "xhttp"
            }

            RadioItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                text: qsTr("mKCP")
                checked: XrayConfigModel.transport === "mkcp"
                onClicked: if (XrayConfigModel.transport !== "mkcp") XrayConfigModel.transport = "mkcp"
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: XrayConfigModel.transport === "mkcp"
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
                    textField.text: XrayConfigModel.mkcpTti
                    textField.maximumLength: 3
                    textField.validator: RegularExpressionValidator { regularExpression: /^(|\d{1,2}|100)$/ }
                    textField.onTextEdited: XrayConfigModel.mkcpTti = textField.text
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 10, 100)
                        if (v !== XrayConfigModel.mkcpTti) XrayConfigModel.mkcpTti = v
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("uplinkCapacity")
                    helperText: qsTr("Uplink capacity (MB/s). Maximum: 2147483647.")
                    placeholderText: XrayConfigModel.mkcpDefaultUplinkCapacity()
                    textField.text: XrayConfigModel.mkcpUplinkCapacity
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: XrayConfigModel.mkcpUplinkCapacity = textField.text
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 0, 2147483647)
                        if (v !== XrayConfigModel.mkcpUplinkCapacity) XrayConfigModel.mkcpUplinkCapacity = v
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("downlinkCapacity")
                    helperText: qsTr("Downlink capacity (MB/s). Maximum: 2147483647.")
                    placeholderText: XrayConfigModel.mkcpDefaultDownlinkCapacity()
                    textField.text: XrayConfigModel.mkcpDownlinkCapacity
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: XrayConfigModel.mkcpDownlinkCapacity = textField.text
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 0, 2147483647)
                        if (v !== XrayConfigModel.mkcpDownlinkCapacity) XrayConfigModel.mkcpDownlinkCapacity = v
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("mtu")
                    helperText: qsTr("Maximum packet size in bytes. Range: 576–1460.")
                    placeholderText: XrayConfigModel.mkcpDefaultMtu()
                    textField.text: XrayConfigModel.mkcpMtu
                    textField.maximumLength: 4
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: XrayConfigModel.mkcpMtu = textField.text
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 576, 1460)
                        if (v !== XrayConfigModel.mkcpMtu) XrayConfigModel.mkcpMtu = v
                        if (XrayConfigModel.mkcpMaxSendingWindow !== "") {
                            var floorNow = parseInt(v !== "" ? v : XrayConfigModel.mkcpDefaultMtu(), 10)
                            if (parseInt(XrayConfigModel.mkcpMaxSendingWindow, 10) < floorNow) {
                                XrayConfigModel.mkcpMaxSendingWindow = String(floorNow)
                            }
                        }
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("cwndMultiplier")
                    helperText: qsTr("Congestion window multiplier. Minimum: 1.")
                    placeholderText: XrayConfigModel.mkcpDefaultCwndMultiplier()
                    textField.text: XrayConfigModel.mkcpCwndMultiplier
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: XrayConfigModel.mkcpCwndMultiplier = textField.text
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 1, 2147483647)
                        if (v !== XrayConfigModel.mkcpCwndMultiplier) XrayConfigModel.mkcpCwndMultiplier = v
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("maxSendingWindow")
                    helperText: qsTr("Send window in bytes. Must not be smaller than mtu. Leave empty to let Xray decide.")
                    textField.text: XrayConfigModel.mkcpMaxSendingWindow
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: XrayConfigModel.mkcpMaxSendingWindow = textField.text
                    textField.onEditingFinished: {
                        var mtuFloor = parseInt(XrayConfigModel.mkcpMtu !== "" ? XrayConfigModel.mkcpMtu : XrayConfigModel.mkcpDefaultMtu(), 10)
                        var v = root.clampInt(textField.text, mtuFloor, 2147483647)
                        if (v !== XrayConfigModel.mkcpMaxSendingWindow) XrayConfigModel.mkcpMaxSendingWindow = v
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: XrayConfigModel.transport === "xhttp"
                spacing: 12

                DropDownType {
                    id: modeDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: XrayConfigModel.xhttpMode
                    descriptionText: qsTr("Mode")
                    headerText: qsTr("Mode")
                    listView: RadioListType {
                        currentValue: XrayConfigModel.xhttpMode
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpModeOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            XrayConfigModel.xhttpMode = selectedText
                            modeDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === XrayConfigModel.xhttpMode) {
                                    selectedIndex = i;
                                    break
                                }
                            }
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
                    textField.text: XrayConfigModel.xhttpHost
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9._:,-]*$/ }
                    textField.onTextEdited: XrayConfigModel.xhttpHost = textField.text
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== XrayConfigModel.xhttpHost) XrayConfigModel.xhttpHost = v
                        hostField.errorText = XrayConfigModel.isValidHost(v) ? "" : qsTr("Enter a valid IP address or domain name")
                    }
                }

                TextFieldType {
                    id: pathField
                    Layout.fillWidth: true
                    headerText: qsTr("Path")
                    textField.text: XrayConfigModel.xhttpPath
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9\-._~:\/?#\[\]@!$&'()*+,;=%]*$/ }
                    textField.onTextEdited: XrayConfigModel.xhttpPath = textField.text
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== "" && v.charAt(0) !== "/") v = "/" + v
                        if (v !== textField.text) textField.text = v
                        if (v !== XrayConfigModel.xhttpPath) XrayConfigModel.xhttpPath = v
                    }
                }

                DropDownType {
                    id: uplinkMethodDropDown
                    fitContent: true
                    Layout.fillWidth: true
                    text: XrayConfigModel.xhttpUplinkMethod
                    descriptionText: qsTr("UplinkHTTPMethod")
                    headerText: qsTr("UplinkHTTPMethod")
                    listView: RadioListType {
                        currentValue: XrayConfigModel.xhttpUplinkMethod
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpUplinkMethodOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            XrayConfigModel.xhttpUplinkMethod = selectedText
                            uplinkMethodDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === XrayConfigModel.xhttpUplinkMethod) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                SwitchRowType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("Disable gRPC Header")
                    descriptionText: qsTr("noGRPCHeader")
                    checked: XrayConfigModel.xhttpDisableGrpc
                    onToggled: XrayConfigModel.xhttpDisableGrpc = checked
                }

                SwitchRowType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("Disable SSE Header")
                    descriptionText: qsTr("noSSEHeader")
                    checked: XrayConfigModel.xhttpDisableSse
                    onToggled: XrayConfigModel.xhttpDisableSse = checked
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
                    text: XrayConfigModel.xhttpSessionPlacement
                    descriptionText: qsTr("SessionPlacement")
                    headerText: qsTr("SessionPlacement")
                    listView: RadioListType {
                        currentValue: XrayConfigModel.xhttpSessionPlacement
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpSessionPlacementOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            XrayConfigModel.xhttpSessionPlacement = selectedText
                            sessionPlacementDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === XrayConfigModel.xhttpSessionPlacement) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("SessionKey")
                    textField.text: XrayConfigModel.xhttpSessionKey
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9_-]*$/ }
                    textField.onTextEdited: XrayConfigModel.xhttpSessionKey = textField.text
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== XrayConfigModel.xhttpSessionKey) XrayConfigModel.xhttpSessionKey = v
                    }
                }

                DropDownType {
                    id: seqPlacementDropDown
                    visible: root.packetUpEffective
                    fitContent: true
                    Layout.fillWidth: true
                    text: XrayConfigModel.xhttpSeqPlacement
                    descriptionText: qsTr("SeqPlacement")
                    headerText: qsTr("SeqPlacement")
                    listView: RadioListType {
                        currentValue: XrayConfigModel.xhttpSeqPlacement
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpSeqPlacementOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            XrayConfigModel.xhttpSeqPlacement = selectedText
                            seqPlacementDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === XrayConfigModel.xhttpSeqPlacement) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    visible: root.packetUpEffective
                    headerText: qsTr("SeqKey")
                    textField.text: XrayConfigModel.xhttpSeqKey
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9_-]*$/ }
                    textField.onTextEdited: XrayConfigModel.xhttpSeqKey = textField.text
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== XrayConfigModel.xhttpSeqKey) XrayConfigModel.xhttpSeqKey = v
                    }
                }

                DropDownType {
                    id: uplinkDataPlacementDropDown
                    visible: root.packetUpEffective
                    fitContent: true
                    Layout.fillWidth: true
                    text: XrayConfigModel.xhttpUplinkDataPlacement
                    descriptionText: qsTr("Where the uploaded data is carried in the request")
                    headerText: qsTr("UplinkDataPlacement")
                    listView: RadioListType {
                        currentValue: XrayConfigModel.xhttpUplinkDataPlacement
                        model: ListModel {
                            Component.onCompleted: {
                                var opts = XrayConfigModel.xhttpUplinkDataPlacementOptions()
                                for (var i = 0; i < opts.length; i++) {
                                    append({name: opts[i]})
                                }
                            }
                        }
                        onActivated: {
                            XrayConfigModel.xhttpUplinkDataPlacement = selectedText
                            uplinkDataPlacementDropDown.close()
                        }
                        Component.onCompleted: {
                            for (var i = 0; i < model.count; i++) {
                                if (model.get(i).name === XrayConfigModel.xhttpUplinkDataPlacement) {
                                    selectedIndex = i;
                                    break
                                }
                            }
                        }
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    visible: root.packetUpEffective
                    headerText: qsTr("UplinkDataKey")
                    textField.text: XrayConfigModel.xhttpUplinkDataKey
                    textField.validator: RegularExpressionValidator { regularExpression: /^[A-Za-z0-9_-]*$/ }
                    textField.onTextEdited: XrayConfigModel.xhttpUplinkDataKey = textField.text
                    textField.onEditingFinished: {
                        var v = textField.text.trim()
                        if (v !== XrayConfigModel.xhttpUplinkDataKey) XrayConfigModel.xhttpUplinkDataKey = v
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
                    textField.text: XrayConfigModel.xhttpUplinkChunkSize
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: XrayConfigModel.xhttpUplinkChunkSize = textField.text
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 0, 2147483647)
                        if (v !== XrayConfigModel.xhttpUplinkChunkSize) XrayConfigModel.xhttpUplinkChunkSize = v
                    }
                }

                TextFieldType {
                    Layout.fillWidth: true
                    headerText: qsTr("scMaxBufferedPosts")
                    helperText: qsTr("Max buffered POSTs. Range: 0–2147483647.")
                    textField.text: XrayConfigModel.xhttpScMaxBufferedPosts
                    textField.maximumLength: 10
                    textField.validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                    textField.onTextEdited: XrayConfigModel.xhttpScMaxBufferedPosts = textField.text
                    textField.onEditingFinished: {
                        var v = root.clampInt(textField.text, 0, 2147483647)
                        if (v !== XrayConfigModel.xhttpScMaxBufferedPosts) XrayConfigModel.xhttpScMaxBufferedPosts = v
                    }
                }

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("scMaxEachPostBytes")
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    minValue: XrayConfigModel.xhttpScMaxEachPostBytesMin
                    maxValue: XrayConfigModel.xhttpScMaxEachPostBytesMax
                    minPlaceholder: XrayConfigModel.scMaxEachPostBytesMinDefault()
                    maxPlaceholder: XrayConfigModel.scMaxEachPostBytesMaxDefault()
                    onMinChanged: function(val) { XrayConfigModel.xhttpScMaxEachPostBytesMin = val }
                    onMaxChanged: function(val) { XrayConfigModel.xhttpScMaxEachPostBytesMax = val }
                }

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("scStreamUpServerSecs")
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    minValue: XrayConfigModel.xhttpScStreamUpServerSecsMin
                    maxValue: XrayConfigModel.xhttpScStreamUpServerSecsMax
                    minPlaceholder: XrayConfigModel.scStreamUpServerSecsMinDefault()
                    maxPlaceholder: XrayConfigModel.scStreamUpServerSecsMaxDefault()
                    onMinChanged: function(val) { XrayConfigModel.xhttpScStreamUpServerSecsMin = val }
                    onMaxChanged: function(val) { XrayConfigModel.xhttpScStreamUpServerSecsMax = val }
                }

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 12
                    text: qsTr("scMinPostsIntervalMs")
                }
                MinMaxRowType {
                    Layout.fillWidth: true
                    minValue: XrayConfigModel.xhttpScMinPostsIntervalMsMin
                    maxValue: XrayConfigModel.xhttpScMinPostsIntervalMsMax
                    minPlaceholder: XrayConfigModel.scMinPostsIntervalMsMinDefault()
                    maxPlaceholder: XrayConfigModel.scMinPostsIntervalMsMaxDefault()
                    onMinChanged: function(val) { XrayConfigModel.xhttpScMinPostsIntervalMsMin = val }
                    onMaxChanged: function(val) { XrayConfigModel.xhttpScMinPostsIntervalMsMax = val }
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
                        subtitle: XrayConfigModel.xmuxEnabled ? qsTr("On") : qsTr("Off")
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
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        visible: flickable.enabled
        enabled: XrayConfigModel.hasUnsavedChanges
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
                var touchesServer = XrayConfigModel.pendingChangeTouchesServer("")
                if (touchesServer)
                    PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.Xray, touchesServer)
            }
            var noButtonFunction = function () {}
            Dialogs.askQuestion(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
