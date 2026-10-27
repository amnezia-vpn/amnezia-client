import QtQuick

import ProtocolEnum 1.0

import "./"

PageServiceMtProtoSettingsBase {
    configModel: TelemtConfigModel
    protocol: ProtocolEnum.Telemt
    isTelemt: true

    pageTitle: qsTr("Telemt settings")
    readMoreUrl: "https://github.com/telemt/telemt"
    noInternetText: qsTr("No internet connection. Connect to the internet to change Telemt settings.")
    startedText: qsTr("Telemt started")
    stoppedText: qsTr("Telemt stopped")
    deployFirstText: qsTr("Deploy Telemt first")
    connectionLinkText: qsTr("Telemt connection link")
    deleteText: qsTr("Delete Telemt")
    enableText: qsTr("Enable Telemt")
    enableToEditText: qsTr("Enable Telemt to edit settings")
    newSecretSavedText: qsTr("New secret saved. It will be applied when Telemt is started.")
    natSwitchText: qsTr("Set public IP manually")
    natSwitchDescriptionText: qsTr("By default the proxy auto-detects its public IP. Enable to override it manually, e.g. when the server is behind NAT / Docker bridge")
    natExternalIpHeaderText: qsTr("Public IP")
    natExternalIpErrorText: qsTr("Public IP: enter a valid IPv4 address")
}
